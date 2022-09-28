#include "MyHttpConn.h"

#define FULL_HTML_ROOT "/home/ljl/mpServer/htmlfolder/"
#define FULL_OTH_FILE_ROOT "/home/ljl/mpServer/sourcefolder/"


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
    --MyHttpConn::hc_snUsedCount;
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
    if(!stBuffer.empty()){
        stBuffer.drstory_buff();
    }
}

void MyHttpConn::httpConnInit(bool et)
{
    httpConnInit();
    bEt = et;
}

/*处理完整请求后的初始化*/
void MyHttpConn::httpConnInit()
{
    stBuffer = (std::move(Buffer((size_t)0,RECVBUF_SIZE)));
    str_body.resize(0);
    str_line.resize(0);
    memset(carrSendBuf,'\0',sizeof(carrSendBuf));
    memset(cpUploadType,'\0',sizeof(cpUploadType));
    memset(&stFileStat,'\0',sizeof(stFileStat));
    memset(&stIc,'\0',sizeof(stIc));
    pSqlPool = MySqlConnPool::getSqlConnPoolInstance();
    bLinger = false,bUpload = false;
    nContent = 0;
    nBytesHadSend = 0, nLastPosInSend = 0, nBytes2Send = 0, nTotal = 0;
    enCheckState = CHECK_STATE_REQUESTLINE;
    enMethod = GET;
    if(cpFileAddress != nullptr)
    {
        unmap();
    }
    cpFileAddress = nullptr;
}


bool MyHttpConn::read(){
    if(nSockfd < 0){
        LOG_ERROR("nSockfd:%d",nSockfd);
        return false;
    }

    do{
        char newBuff[65536];
        struct iovec iov[2];
        const size_t writeable_size = stBuffer.writeable_size();

        iov[0].iov_base = stBuffer.raw_ptr() + stBuffer.size();
        iov[0].iov_len = writeable_size;
        iov[1].iov_base = newBuff;
        iov[1].iov_len = sizeof(newBuff);

        const ssize_t read_ret = readv(nSockfd,iov,2);
        //printf("read_ret:%ld,writeable_size:%d\n",read_ret,writeable_size);
        if(read_ret < 0){
            if(errno == EAGAIN || errno == EWOULDBLOCK) return true;
            LOG_ERROR("Read Error[%d]!",errno);
            return false;
        }
        else if(read_ret == 0){
            return false;
        }
        else{
            if(read_ret <= writeable_size){
                stBuffer.add_size(read_ret);
            }else {
                size_t outlimit_size = read_ret - writeable_size;
                
                stBuffer.add_size(writeable_size);
                stBuffer.append_data(newBuff,outlimit_size);
            }
        }
    }while(bEt);

    return true;
}


/*解析请求行*/
HTTP_CODE MyHttpConn::parseRequestLine()
{
    char *cpRequestLine = const_cast<char*>(str_line.data());
    /*http方法*/
    if(strstr(cpRequestLine,"GET")){
        enMethod = GET;
    }else if(strstr(cpRequestLine,"POST")){
        enMethod = POST;
    }else{
        LOG_WARNING("UnDefined Method![%d]User",nSockfd);
        return BAD_REQUEST;
    }
    
    /*http版本*/
    char* cpTmpVersion = strstr(cpRequestLine,"HTTP/1.1");
    if(cpTmpVersion == nullptr)
    {
        LOG_WARNING("[%s]UnSupported Version![%d]User",cpRequestLine,nSockfd);
        return BAD_REQUEST;
    }
    *(--cpTmpVersion) = '\0';
    str_version = "HTTP/1.1";

    /*请求url*/
    char *purl = nullptr;
    if((purl = strstr(cpRequestLine,"//"))!=nullptr){
        if(strlen(++purl) == 1){
            str_url = "login.html";
        }else{
            str_url = purl+1;
        }
    }
    else if((purl = strstr(cpRequestLine,"/"))!=nullptr){
        if(strlen(purl) == 1){
            str_url = "login.html";
        }else{
            str_url = purl+1;
        }
    }
    else{
        LOG_ERROR("Recv a invalid rqLine");
        return BAD_REQUEST;
    }

    /*处理完请求行--->开始处理请求头*/
    enCheckState = CHECK_STATE_REQUESTHEADER;
    return INCOMPLETE_REQUEST;
}

/*解析请求头*/
HTTP_CODE MyHttpConn::parseRequestHeader()
{
    char* cpRequestHeader = const_cast<char*>(str_line.c_str());
    char*  cpTmpInfo = nullptr;
    if(cpRequestHeader[0] == '\0' || strcmp(str_line.c_str(),"\r\n") == 0)
    {
        /*空行 代表消息头处理完毕 判定是否需要处理content内容*/
        if(nContent > 0){
            enCheckState = bUpload ? CHECK_STATE_UPLOAD : CHECK_STATE_CONTENT;
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
        str_host = cpTmpInfo;
    }
    else if((cpTmpInfo = strstr(cpRequestHeader,"multipart/form-data")) != nullptr )
    {
        bUpload = true;        
    }
    return INCOMPLETE_REQUEST;
}



HTTP_CODE MyHttpConn::processUpload(){
    if(str_body.size() < nContent){
        return INCOMPLETE_REQUEST;
    }
    
    std::string_view body = str_body;

    size_t st = 0, ed = 0;
    ed = body.find("\r\n");
    std::string_view boundary = body.substr(0, ed);

    // 取文件名
    st = str_body.find("filename=\"", ed) + strlen("filename=\"");
    ed = str_body.find("\"", st);
    std::string filenpath = static_cast<std::string>(FULL_OTH_FILE_ROOT) + str_body.substr(st, ed - st);
    printf("upload_path[%s]\n",filenpath.c_str());

    // 取内容
    st = body.find("\r\n\r\n", ed) + strlen("\r\n\r\n");
    ed = body.find(boundary, st) - 2; // 文件结尾也有\r\n
    std::string_view content =  body.substr(st, ed - st);
    
    // 写内容
    std::ofstream ofs;
    // 如果文件分多次发送，应该采用app，同时为避免重复上传，应该用md5做校验
    ofs.open(filenpath.data(), std::ios::ate | std::ios::binary);
    ofs << content;
    ofs.close(); 

    // size_t st = 0, ed = 0;
    // ed = body.find("\r\n");
    // std::string boundary = body.substr(0, ed);
    // printf("boundary[%s]\n",boundary.data());
    // // 解析文件信息
    // st = body.find("filename=\"", ed) + strlen("filename=\"");
    // ed = body.find("\"", st);
    // std::string filenpath = FULL_OTH_FILE_ROOT + body.substr(st, ed - st);
    // printf("filenpath[%s]\n",filenpath.data());
    // // 解析文件内容，文件内容以\r\n\r\n开始
    // st = body.find("\r\n\r\n", ed) + strlen("\r\n\r\n");
    // ed = body.find(boundary, st) - 2; // 文件结尾也有\r\n
    // std::string content =  body.substr(st, ed - st);
    // printf("content[%s]\n",content.data());
    // std::ofstream ofs;
    // // 如果文件分多次发送，应该采用app，同时为避免重复上传，应该用md5做校验
    // ofs.open(filenpath.data(), std::ios::ate);
    // ofs << content;
    // ofs.close(); 

    return FILE_REQUEST;
}

HTTP_CODE MyHttpConn::parseContent()
{
    char* cpTmpContent = nullptr;
    if(!str_body.empty()){
        cpTmpContent = const_cast<char*>(str_body.data());
    }
    
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

            if(strstr(str_url.data(),"register"))
            {
                /*注册页面*/
                if(nMatchRows != 0)
                {
                    /*原用户已经注册*/
                    str_url = "repeatRegister.html";
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
                    str_url = "registerSucceed.html";
                }
            }
            else if(strstr(str_url.data(),"login"))
            {
                /*登录界面*/
                if(nMatchRows == 0)
                {
                    /*还未注册*/
                    str_url = "register.html";
                }
                else 
                {
                    MYSQL_ROW nMysqlRow = mysql_fetch_row(pResult);
                    std::string strStoredPwd = nMysqlRow[0];
                    if(strStoredPwd == strPwd)
                    {
                        /*密码正常-->登录成功*/
                        str_url = "index.html";
                    }
                    else 
                    {
                        /*密码错误-->提示失败*/
                        str_url = "loginError.html";
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
        if(str_url.empty() ) str_url = "login.html";
        if(strstr(str_url.data(),"pressure")) return FILE_REQUEST;
    }

    if(str_url.empty()
        )
    {
        str_url = "login.html";
    }

    std::string strFullAddress = FULL_HTML_ROOT;
    if(strstr(str_url.data(),"html") == nullptr)
    {
        strFullAddress = FULL_OTH_FILE_ROOT;
    }
    strFullAddress +=  str_url;
    char* cpFullAddress = const_cast<char*>(strFullAddress.c_str());

    if(stat(cpFullAddress,&stFileStat) < 0)
    {
        LOG_ERROR("stat error, errno:%d",errno);
        return NO_RESOURCE;
    }
    if(!(stFileStat.st_mode & S_IROTH))
    {
        LOG_ERROR("S_IROTH error, errno:%d",errno);
        return FORBIDDEN_REQUEST;
    }
    int nFilefd = open(cpFullAddress,O_RDONLY);
    if(nFilefd <= 0 )
    {
        LOG_ERROR("nFilefd error, errno:%d",errno);
        return NO_RESOURCE;
    }
    cpFileAddress = (char *)mmap(0,stFileStat.st_size,PROT_READ,MAP_PRIVATE,nFilefd,0);
    close(nFilefd);
    return FILE_REQUEST;
}

/*解析报文入口*/
HTTP_CODE MyHttpConn::processRead(){
    LINE_STATUS enCurLineStatus = LINE_COMPLETE;
    HTTP_CODE enCurHttpCode = NO_REQUEST;
    while(!stBuffer.empty())
    {
        if(enCheckState == CHECK_STATE_UPLOAD ||
            enCheckState == CHECK_STATE_CONTENT){
            /*主状态机：body*/
            stBuffer.read_all(str_body);
            if(str_body.size() < nContent){
                return INCOMPLETE_REQUEST;
            }
        }
        else{
            str_line.resize(0);
            /*其余状态 逐行解析*/
            if(!stBuffer.find_str(str_line,"\r\n")){
                return INCOMPLETE_REQUEST;
            }
        }
        switch(enCheckState)
        {
            /*请求行*/
            case CHECK_STATE_REQUESTLINE:
            {
                if(parseRequestLine() == BAD_REQUEST)
                {
                    return BAD_REQUEST;
                }
                break;
            }
            /*请求头*/
            case CHECK_STATE_REQUESTHEADER:
            {
                enCurHttpCode = parseRequestHeader();
                if(enCurHttpCode == BAD_REQUEST)
                {
                    return BAD_REQUEST;
                }else if(enCurHttpCode == GET_REQUEST){
                    return parseContent(); 
                }
                break;
            }
            /*报文主体*/
            case CHECK_STATE_CONTENT:
            {
                return parseContent();
            }
            /*上传请求*/
            case CHECK_STATE_UPLOAD:
            {
                return processUpload();
            }
            default:
            {
                break;
            }
        }
    }
    return INCOMPLETE_REQUEST;

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
    std::string str = "text/html";
    if(!str_url.empty() && strstr(str_url.data(),".pdf")){
        str="application/pdf";
    }
    return addContentLength(nWait2SendSize) && addContentType(str.c_str()) &&addLinger() && addBlankLine();
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
        case INCOMPLETE_REQUEST:
        {
            if(enCheckState == CHECK_STATE_UPLOAD)
            {
                return true;
            }
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

    LOG_INFO("DeBug:write to [%d],nBytes2Send[%d],url[%s]",nSockfd,nBytes2Send,str_url.data());

    int nRet = 0;
    for( ; ; )
    {
        nRet = writev(nfd,stIc,nIcCount);
        if(nRet <= 0)
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
        nTotal += nRet;

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
            LOG_DEBUG("Send 2 [%d] done,total[%d],keep-alive:%d",nfd,nTotal,bLinger);
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
        if(enRet == NO_REQUEST || enRet == INCOMPLETE_REQUEST)
        {
            htModfd(fd,hc_snEpollFd,EPOLLIN,true); 
            return ; 
        }
        bool bPrepared2Write = processWrite(enRet);
        if(!bPrepared2Write) 
        {
            closeConn();
            return ;
        }else
        {
            /*正常请求 || 已捉到完整的文件*/
            htModfd(fd,hc_snEpollFd,EPOLLOUT,true);
        } 
    }
    else
    {
        closeConn();
        return ;
    }
     
}