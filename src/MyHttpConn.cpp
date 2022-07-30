#include "MyHttpConn.h"


int MyHttpConn::hc_snEpollFd = 0;
int MyHttpConn::hc_snUsedCount = 0;

const char* cpStatus200 = "OK";
const char* cpStatus403 = "Forbidden";
const char* cp403Response = "Please check your permission!";
const char* cpStatus404 = "Not Found";
const char* cp404Response = "Please check the url!";
const char* cpStatus500 = "Internal Error";
const char* cp500Response = "Please try again later";

int htRemovefd(int nFd,int nEpollFd)
{
    LOG_DEBUG("[%d] User closed!",nFd);
    epoll_ctl(nEpollFd,EPOLL_CTL_DEL,nFd,0);
    close(nFd);
}

int htSetNonBlock(int nFd)
{
    int nOldOption = fcntl(nFd,F_GETFL);
    int nNewOption = nOldOption | O_NONBLOCK;
    fcntl(nFd,F_SETFL,nNewOption);
    return nOldOption;
}

int htModfd(int nFd,int nEpollFd,int nOpt,bool bOneShot)
{
    if(nFd == -1) return -1; 
    epoll_event stEvent;
    stEvent.data.fd = nFd;
    stEvent.events = nOpt | EPOLLET| EPOLLRDHUP;
    if(bOneShot)
    {
        stEvent.events |=  EPOLLONESHOT;
    }
    epoll_ctl(nEpollFd,EPOLL_CTL_MOD,nFd,&stEvent);
    return 0;
}


MyHttpConn::MyHttpConn(int nFd):nSockfd(nFd)
{
}

MyHttpConn::~MyHttpConn()
{
}

void MyHttpConn::httpConnInit()
{
    memset(carrRecvBuf,'\0',sizeof(carrRecvBuf));
    memset(carrSendBuf,'\0',sizeof(carrSendBuf));
    memset(&stFileStat,'\0',sizeof(stFileStat));
    memset(&stIc,'\0',sizeof(stIc));
    pSqlPool = MySqlConnPool::getSqlConnPoolInstance();
    bLinger = false;
    nRowStart = 0 ,nRowEnd = 0 , nLastPosInRecv = 0 , nContent = 0 ;
    nBytesHadSend = 0, nLastPosInSend = 0, nBytes2Send = 0;
    enCheckState = CHECK_STATE_REQUESTLINE;
    enMethod = GET;
    if(cpFileAddress != nullptr)
    {
        unmap();
    }
    cpUrl = nullptr, cpVersion = nullptr, cpHost = nullptr,cpFileAddress = nullptr;
}


/*ET读*/
bool MyHttpConn::read()
{
    if(nLastPosInRecv >= RECVBUF_SIZE || nSockfd < 0)
    {
        LOG_ERROR("ReadErro!nLastPosInRecv:%d,nSockfd:%d",nLastPosInRecv,nSockfd);
        return false;
    }
    for( ; ;)
    {
        int nRead = recv(nSockfd,carrRecvBuf+nLastPosInRecv,RECVBUF_SIZE,0);
        if(nRead == -1)
        {
            if(errno == EAGAIN || errno == EWOULDBLOCK) break;
            LOG_ERROR("RecvError![%d]User,errno:%d",nSockfd,errno);
            return false;
        }
        else if(nRead == 0)
        {
            return false;
        }
        nLastPosInRecv += nRead;
    }
    return true;
}

LINE_STATUS MyHttpConn::parseLine()
{
    if(nSockfd < 0 ) return LINE_BAD;
    char* cpNextRowStart = carrRecvBuf + nRowEnd;
    char* cpNextRowEnd = strstr(cpNextRowStart,"\r\n");
    if(cpNextRowEnd == nullptr)
    {
        return LINE_CONTINUE;
    }
    *cpNextRowEnd = '\0';
    ++cpNextRowEnd;
    *cpNextRowEnd = '\0';
    ++cpNextRowEnd;

    nRowStart = nRowEnd;
    nRowEnd = cpNextRowEnd - carrRecvBuf;

    return LINE_COMPLETE;
}

/*解析请求行*/
HTTP_CODE MyHttpConn::parseRequestLine()
{
    char* cpRequestLine = carrRecvBuf + nRowStart;
    /*http方法*/
    char* cpMethod = strstr(cpRequestLine,"GET");
    enMethod = GET;
    if(cpMethod == nullptr)
    {
        cpMethod = strstr(cpRequestLine,"POST");
        if(cpMethod == nullptr)
        {
            LOG_WARNING("UnDefined Method![%d]User",nSockfd);
            return BAD_REQUEST;
        }
        enMethod = POST;
    }
    
    /*http版本*/
    char* cpTmpVersion = strstr(cpRequestLine,"HTTP/1.1");
    if(cpTmpVersion == nullptr)
    {
        LOG_WARNING("[%s]UnSupported Version![%d]User",cpRequestLine,nSockfd);
        return BAD_REQUEST;
    }
    --cpTmpVersion;
    *cpTmpVersion = '\0';
    cpVersion = "HTTP/1.1";

    /*请求url*/
    char* cpTmpUrl = strstr(cpRequestLine,"//");
    if(cpTmpUrl == nullptr)
    {
        cpTmpUrl = strstr(cpRequestLine,"/");
        if(cpTmpUrl == nullptr) 
        {
            return BAD_REQUEST;
        }
        if(strlen(cpTmpUrl) == 1)
        {
            cpTmpUrl = "/login.html";
        }
    }
    else
    {
        ++cpTmpUrl;
        /*默认访问登录页面*/
        if(strlen(cpTmpUrl) == 1)
        {
            cpTmpUrl = "/login.html";
        }
    }
    cpUrl = cpTmpUrl;

    /*处理完请求行--->开始处理请求头*/
    enCheckState = CHECK_STATE_REQUESTHEADER;
    return INCOMPLETE_REQUEST;
}

/*解析请求头*/
HTTP_CODE MyHttpConn::parseRequestHeader()
{
    char* cpRequestHeader = carrRecvBuf + nRowStart;
    
    char*  cpTmpInfo = nullptr;
    if(cpRequestHeader[0] == '\0')
    {
        /*空行 代表消息头处理完毕 判定是否需要处理content内容*/
        if(nContent != 0 || enMethod == POST)
        {
            enCheckState = CHECK_STATE_CONTENT;
            return INCOMPLETE_REQUEST;
        }
        /*代表该http请求已经处理完毕*/
        return GET_REQUEST;
    }
    else if((cpTmpInfo = strstr(cpRequestHeader,"Connection:")) != nullptr)
    {
        bLinger = false;
        if(strstr(cpTmpInfo,"keep-alive") != nullptr ||
            strstr(cpTmpInfo,"KEEP-ALIVE") != nullptr ||
            strstr(cpTmpInfo,"Keep-Alive") != nullptr)
        {
            bLinger = true;
        }
    }
    else if((cpTmpInfo = strstr(cpRequestHeader,"Content-Length:")) != nullptr)
    {
        cpTmpInfo += 15;
        cpTmpInfo += strspn(cpTmpInfo," \t");
        nContent = atoi(cpTmpInfo);
    }
    else if( (cpTmpInfo = strstr(cpRequestHeader,"Host:")) != nullptr )
    {
        cpTmpInfo += 5;
        cpTmpInfo += strspn(cpTmpInfo," \t");
        cpHost = cpTmpInfo;
    }
    return INCOMPLETE_REQUEST;
}

HTTP_CODE MyHttpConn::parseContent()
{
    
    nRowStart = nRowEnd;
    char* cpTmpContent = carrRecvBuf + nRowStart;
    nRowEnd = strlen(cpTmpContent);
    /*Post*/
    if(enMethod == POST)
    {
        /*注册*/
        MYSQL* pTmpSqlConn = nullptr;
        sqlConnRAII(&pTmpSqlConn,pSqlPool);
        if(pTmpSqlConn == nullptr)
        {
            LOG_ERROR("No sqlConnect can be used!");
            return INTERNAL_ERRNO;
        }

        char* cpName =  strstr(cpTmpContent,"name=");
        cpName += 5;
        char* cpPwd = strstr(cpTmpContent,"&pwd=");
        *cpPwd = '\0';
        cpPwd += 5;
        std::string strName = cpName;
        std::string strPwd = cpPwd;

        /*查询数据库*/
        std::string strSqlSelect = "select passwd from accounts where name = '" + strName + "'" ;
        int nRet = mysql_query(pTmpSqlConn,strSqlSelect.c_str());
        if(nRet != 0)
        {
            LOG_ERROR("MySql Select Error![%s]",strSqlSelect.c_str());
            return INTERNAL_ERRNO;
        }
        MYSQL_RES* pResult = mysql_store_result(pTmpSqlConn);
        if(pResult != nullptr)
        {
            int nMatchRows = mysql_num_rows(pResult);

            if(strstr(cpUrl,"register"))
            {
                /*注册页面*/
                if(nMatchRows != 0)
                {
                    /*原用户已经注册*/
                    cpUrl = "/repeatRegister.html";
                }
                else
                {
                    std::string strSqlInsert = "insert into accounts(name,passwd)values('" + strName +"','" + strPwd +"')";
                    nRet = mysql_query(pTmpSqlConn,strSqlInsert.c_str());
                    if(nRet != 0)
                    {
                        LOG_ERROR("MySql Insert Error![%s]",strSqlInsert.c_str());
                        return INTERNAL_ERRNO;
                    }
                    cpUrl = "/registerSucceed.html";
                }
            }
            else if(strstr(cpUrl,"login"))
            {
                /*登录界面*/
                if(nMatchRows == 0)
                {
                    /*还未注册*/
                    cpUrl = "/register.html";
                }
                else 
                {
                    MYSQL_ROW nMysqlRow = mysql_fetch_row(pResult);
                    std::string strStoredPwd = nMysqlRow[0];
                    if(strStoredPwd == strPwd)
                    {
                        /*密码正常-->登录成功*/
                        cpUrl = "/index.html";
                    }
                    else 
                    {
                        /*密码错误-->提示失败*/
                        cpUrl = "/loginError.html";
                    }
                }
            }
            else
            {
                /*暂不支持访问其余页面*/
                return NO_RESOURCE;
            }
        }
    }
    else if(enMethod == GET)
    {
        /*GET请求 不对content处理*/
        /*默认发送登录页面*/
        if(cpUrl==nullptr || strlen(cpUrl) <= 0) cpUrl = "/login.html";
    }

    if(cpUrl == nullptr || strstr(cpUrl,".html") == nullptr)
    {
        cpUrl = "/login.html";
    }
    
    std::string strUrl(cpUrl);
    std::string strFullAddress = "/home/ljl/mpServer/htmlfolder";
    strFullAddress +=  strUrl;
    char* cpFullAddress = const_cast<char*>(strFullAddress.c_str());
    if(stat(cpFullAddress,&stFileStat) < 0)
    {
        return NO_RESOURCE;
    }
    if(!(stFileStat.st_mode & S_IROTH))
    {
        return FORBIDDEN_REQUEST;
    }
    int nFilefd = open(cpFullAddress,O_RDONLY);
    if(nFilefd <= 0 )
    {
        return NO_RESOURCE;
    }
    cpFileAddress = (char *)mmap(0,stFileStat.st_size,PROT_READ,MAP_PRIVATE,nFilefd,0);
    close(nFilefd);
    return FILE_REQUEST;
}


/*解析报文入口*/
HTTP_CODE MyHttpConn::processRead()
{
    LINE_STATUS enCurLineStatus = LINE_COMPLETE;
    HTTP_CODE enCurHttpCode = NO_REQUEST;

    while(  (enCurLineStatus == LINE_COMPLETE && enCheckState == CHECK_STATE_CONTENT) ||
            ((enCurLineStatus=parseLine()) == LINE_COMPLETE))
    {
        switch(enCheckState)
        {
            /*请求行*/
            case CHECK_STATE_REQUESTLINE:
            {
                enCurHttpCode = parseRequestLine();
                if(enCurHttpCode == BAD_REQUEST)
                {
                    return BAD_REQUEST;
                }
                break;
            }
            /*请求头*/
            case CHECK_STATE_REQUESTHEADER:
            {
                enCurHttpCode = parseRequestHeader();
                if(GET_REQUEST == enCurHttpCode)
                {
                    enCurHttpCode = parseContent();
                    return enCurHttpCode;
                }
                else if(BAD_REQUEST == enCurHttpCode)
                {
                    return BAD_REQUEST;
                }
                break;
            }
            /*报文主体*/
            case CHECK_STATE_CONTENT:
            {
                enCurHttpCode = parseContent();
                return enCurHttpCode;
            }
            default:
            {
                break;
            }
        }
    }
    return enCurHttpCode;
}

bool MyHttpConn::closeConn()
{
    if( nSockfd != -1 
    //    && !bLinger
        )
    {
        htRemovefd(nSockfd,hc_snEpollFd);
        nSockfd = -1;
        httpConnInit();
        return true;
    }
    return false;
}

bool MyHttpConn::addStatusLine(int nStatus,const char* cpTitle)
{
    return addInFormat("%s %d %s\r\n","HTTP/1.1",nStatus,cpTitle);
}

bool MyHttpConn::addResponseHeader(int nWait2SendSize)
{
    return addContentLength(nWait2SendSize) && addLinger() && addBlankLine();
}

bool MyHttpConn::addContentLength(int nWait2SendSize)
{
    return addInFormat("Content-Length:%d\r\n",nWait2SendSize);
}

bool MyHttpConn::addContentType(const char* cpContentStyle)
{
    return addInFormat("Content-Type:%s\r\n",cpContentStyle);
}

bool MyHttpConn::addLinger()
{
    return addInFormat("Connection:%s\r\n",(bLinger?"Keep-Alive":"close"));
}

bool MyHttpConn::addBlankLine()
{
    return addInFormat("%s","\r\n");
}

bool MyHttpConn::addContent(const char* cpContent)
{
    return addInFormat("%s",cpContent);
}

bool MyHttpConn::addInFormat(const char *format,...)
{
    if(nLastPosInSend >= SENDBUF_SIZE)
    {
        return false;
    }
    int nRestSize = SENDBUF_SIZE - nLastPosInSend - 1;

    va_list argLists;
    va_start(argLists,format);
    int nLen = vsnprintf(carrSendBuf+nLastPosInSend,nRestSize,format,argLists);
    if(nLen >= nRestSize)
    {
        LOG_INFO("nLastPosInSend:%d,nRestSize:%d,nLen:%d",nLastPosInSend,nRestSize,nLen);
        va_end(argLists);
        return false;
    }
    nLastPosInSend += nLen;
    va_end(argLists);

    return true;

}

bool MyHttpConn::processWrite(HTTP_CODE enRet)
{
    switch (enRet)
    {
        case NO_RESOURCE:
        case BAD_REQUEST:
        {
            addStatusLine(404,cpStatus404);
            addResponseHeader(strlen(cp404Response));
            if(!addContent(cp404Response))
            {
                return false;
            }
            break;
        }
        case FILE_REQUEST:
        {
            addStatusLine(200,cpStatus200);
            if(stFileStat.st_size != 0)
            {
                addResponseHeader(stFileStat.st_size);
                stIc[0].iov_base = carrSendBuf;
                stIc[0].iov_len = nLastPosInSend;
                stIc[1].iov_base = cpFileAddress;
                stIc[1].iov_len = stFileStat.st_size;
                nIcCount = 2;
                nBytes2Send = nLastPosInSend + stFileStat.st_size;
                return true;
            }
            else
            {
                const char *cp200Response = "<html><body></body></html>";
                addResponseHeader(strlen(cp200Response));
                if(!addContent(cp200Response))
                {
                    return false;
                }
            }
            break;
        }
        case FORBIDDEN_REQUEST:
        {
            addStatusLine(403,cpStatus403);
            addResponseHeader(strlen(cp403Response));
            if(!addContent(cp403Response))
            {
                return false;
            }
            break;
        }
        case INTERNAL_ERRNO:
        {
            addStatusLine(500,cpStatus500);
            addResponseHeader(strlen(cp500Response));
            if(!addContent(cp500Response))
            {
                return false;
            }
            break;
        }
        default:
        {
            LOG_INFO("UNKNOWED RET:%d",enRet);
            return false;
        }
    }
    nBytes2Send = nLastPosInSend;
    stIc[0].iov_base = carrSendBuf;
    stIc[0].iov_len = nLastPosInSend;
    nIcCount = 1;
    return true;
}

void MyHttpConn::unmap()
{
    if(cpFileAddress != nullptr)
    {
        munmap(cpFileAddress,stFileStat.st_size);
        cpFileAddress = nullptr;
    }   
}

void MyHttpConn::write(int nfd)
{
    LOG_INFO("DeBug:write to [%d],nBytes2Send[%d],url[%s]",nSockfd,nBytes2Send,cpUrl);
}

void MyHttpConn::write(int nfd,bool& bflag)
{
    nSockfd = nfd; 
    if(nBytes2Send <= 0)
    {
        htModfd(nfd,hc_snEpollFd,EPOLLIN,true); 
        httpConnInit();
        bflag = true;
        return ;
    }

    int nRet = 0;
    for( ; ; )
    {
        nRet = writev(nfd,stIc,nIcCount);

        if(nRet == 0)
        {
            if(errno == EAGAIN)
            {
                htModfd(nfd,hc_snEpollFd,EPOLLOUT,true);
                bflag = true;
                return ;
            }
            LOG_WARNING("Response to [%d]user error[%d]!,Now Close it",nSockfd,errno);
            closeConn();
            unmap();
            bflag = false;
            return ;
        }

        nBytesHadSend += nRet;
        nBytes2Send -= nRet;

        if(nBytesHadSend >= stIc[0].iov_len)
        {
            stIc[0].iov_len = 0;
            stIc[1].iov_base = cpFileAddress + (nBytesHadSend - nLastPosInSend);
            stIc[1].iov_len = nBytes2Send;
        }
        else 
        {
            stIc[0].iov_len -= nBytesHadSend;
            stIc[0].iov_base = carrSendBuf + nBytesHadSend;
        }

        if(nBytes2Send <= 0)
        {
            LOG_DEBUG("Send 2 [%d] done,keep-alive:%d",nfd,bLinger);
            unmap();
            htModfd(nfd,hc_snEpollFd,EPOLLIN,true);
            if(bLinger)
            {
                httpConnInit();
                bflag = true;
                bLinger = true;
                return ;
            }
            else
            {
                closeConn();
                bflag = false;
                return ;
            }
        }
    }
}

/*线程任务函数*/
void MyHttpConn::process(int fd)
{
    nSockfd = fd;
    bool bReadSucceed = read();
    if(bReadSucceed)
    {
        HTTP_CODE enRet = processRead();
        if(enRet == NO_REQUEST)
        {
            htModfd(fd,hc_snEpollFd,EPOLLIN,true); 
            return ; 
        }
        bool bPrepared2Write = processWrite(enRet);
        if(!bPrepared2Write) 
        {
            closeConn();
            return ;
        }
        /*TODO:要重新设置EPOLLIN吗*/
        htModfd(fd,hc_snEpollFd,EPOLLOUT,true); 
    }
    else
    {
        closeConn();
        return ;
    }
     
}