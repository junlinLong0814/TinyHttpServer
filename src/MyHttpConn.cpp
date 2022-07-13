#include "MyHttpConn.h"

int MyHttpConn::hc_snEpollFd = 0;
int MyHttpConn::hc_snUsedCount = 0;

MyHttpConn::MyHttpConn(int nFd):nSockfd(nFd)
{
    // std::unique_ptr<MyTool> upTmpTool(new MyTool());
    // upTool = std::move(upTmpTool);
    //httpConnInit();
}
MyHttpConn::~MyHttpConn()
{
    
}

void MyHttpConn::httpConnInit()
{
    memset(carrRecvBuf,'\0',sizeof(carrRecvBuf));
    memset(carrSendBuf,'\0',sizeof(carrSendBuf));
    pSqlPool = MySqlConnPool::getSqlConnPoolInstance();
    bLinger = false;
    nRowStart = 0 , nRowEnd = 0 , nLastPosInRecv = 0 , nContent = 0;
    enCheckState = CHECK_STATE_REQUESTLINE;
    enMethod = GET;
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
    if(cpVersion == nullptr)
    {
        LOG_WARNING("UnSupported Version![%d]User",nSockfd);
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
            cpUrl = "/login.html";
        }
    }
    else
    {
        ++cpTmpUrl;
        /*默认访问登录页面*/
        if(strlen(cpTmpUrl) == 1)
        {
            cpUrl = "/login.html";
        }
    }

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
        // if(nContent != 0 || enMethod == POST)
        // {
        //     enCheckState = CHECK_STATE_CONTENT;
        //     return INCOMPLETE_REQUEST;
        // }
        /*代表该http请求已经处理完毕*/
        LOG_INFO("Method[%d],Version[%s],Url[%s]",enMethod,cpVersion,cpUrl);
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
                    return GET_REQUEST;
                }
                std::string strSqlInsert = "insert into accounts(name,passwd)values('" + strName +"','" + strPwd +"')";
                nRet = mysql_query(pTmpSqlConn,strSqlInsert.c_str());
                if(nRet != 0)
                {
                    LOG_ERROR("MySql Insert Error![%s]",strSqlInsert.c_str());
                    return INTERNAL_ERRNO;
                }
                cpUrl = "/registerSucceed.html";
                return GET_REQUEST;
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
                return GET_REQUEST;
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
    }

    return GET_REQUEST;
}

/*解析报文入口*/
HTTP_CODE MyHttpConn::processRead()
{
    LINE_STATUS enCurLineStatus = LINE_COMPLETE;
    HTTP_CODE enCurHttpCode = NO_REQUEST;

    while(((enCurLineStatus=parseLine()) == LINE_COMPLETE))
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
                    return GET_REQUEST;
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
                if(enCurHttpCode == GET_REQUEST)
                {
                    return GET_REQUEST;
                }
                else if(enCurHttpCode == NO_RESOURCE)
                {
                    return NO_RESOURCE;
                }
                break;
            }
            default:
            {
                break;
            }
        }
    }
    return NO_REQUEST;
}

bool MyHttpConn::closeConn()
{
    if( nSockfd != -1 
//        && !bLinger
        )
    {
        LOG_INFO("A Client exit!");
        //upTool->removefd(hc_snEpollFd,nSockfd);
        nSockfd = -1;
        httpConnInit();
        return true;
    }
    return false;
}

/*线程任务函数*/
void MyHttpConn::process()
{
    HTTP_CODE enRet = processRead();
    if(enRet == NO_REQUEST)
    {
       // upTool->modfd(nSockfd,hc_snEpollFd,EPOLLIN,true);
    }   

}