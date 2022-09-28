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
    if(!st_rbuf.empty()){
        st_rbuf.drstory_buff();
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
    st_rbuf = std::move(Buffer((size_t)0,RECVBUF_SIZE));
    str_body.resize(0);
    str_line.resize(0);
    boundary.resize(0);
    filepath.resize(0);
    umap_body.clear();
    upsize= 0;

    bLinger = bUpload = false;
    nContent =  nBytesHadSend =  nLastPosInSend = nBytes2Send = 0;

    enCheckState = CHECK_STATE_REQUESTLINE;
    enMethod = GET;
    
    pSqlPool = MySqlConnPool::getSqlConnPoolInstance();

    memset(carrSendBuf,'\0',sizeof(carrSendBuf));
    memset(&stFileStat,'\0',sizeof(stFileStat));
    memset(&stIc,'\0',sizeof(stIc));
    
    
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
        const size_t writeable_size = st_rbuf.writeable_size();

        iov[0].iov_base = st_rbuf.raw_ptr() + st_rbuf.size();
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
                st_rbuf.add_size(read_ret);
            }else {
                size_t outlimit_size = read_ret - writeable_size;
                
                st_rbuf.add_size(writeable_size);
                st_rbuf.append_data(newBuff,outlimit_size);
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
           str_get_url = ("login.html");
        }else{
           str_get_url = (purl+1);
        }
    }
    else if((purl = strstr(cpRequestLine,"/"))!=nullptr){
        if(strlen(purl) == 1){
           str_get_url = ("login.html");
        }else{
           str_get_url = (purl+1);
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
    size_t st = 0, ed = 0;
    if(boundary.size() == 0){
        // 第一次接收文件
        ed = str_body.find("\r\n",0);
        boundary = str_body.substr(0, ed);

        // 取文件名
        st = str_body.find("filename=\"", ed) + strlen("filename=\"");
        ed = str_body.find("\"", st);
        filepath = static_cast<std::string>(FULL_OTH_FILE_ROOT) + str_body.substr(st, ed - st);

        // 取内容
        st = str_body.find("\r\n\r\n",ed) + strlen("\r\n\r\n");
        //printf("boundary:%s\nfilepath:%s\n",boundary.data(),filepath.data());
    }
    size_t n_boundary_idx = str_body.find(boundary, st);
    if(n_boundary_idx == std::string::npos){
        // 后续还有文件内容需要读取
        ed = str_body.size();
    }   
    else{
        // 文件全部内容读取完毕
        ed = n_boundary_idx - 2; // 文件结尾也有\r\n
    }
    std::string content = std::move(str_body.substr(st,ed - st));
    
    // 写内容
    std::ofstream ofs;
    // 如果文件分多次发送，应该采用app，同时为避免重复上传，应该用md5做校验
    ofs.open(filepath.data(), std::ios::app | std::ios::binary);
    ofs << content;
    ofs.close(); 

    return n_boundary_idx==-1?INCOMPLETE_REQUEST:FILE_REQUEST;
}

void MyHttpConn::parse_postBody(){
    if(str_body.size() < nContent){
        return ;
    }
    auto convertHex = [](char ch)->int{
        if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
        if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
        return static_cast<int>(ch);
    };
    std::string key, value;
    int num = 0;
    int n = str_body.size();
    bool need_value = false;
    //key=value&key=value
    //%:hex,+:' '
    //i:right,j:left
    int i = 0, j = 0;
    for (; i < n; i ++)
    {
        char ch = str_body[i];
        switch (ch)
        {
        case '=':
            need_value = true;
            key = str_body.substr(j, i - j);
            j = i + 1;
            break;
        case '+':
            str_body[i] = ' ';
            break;
        case '%':
            num = convertHex(str_body[i + 1] * 16 + convertHex(str_body[i + 2]));
            str_body[i + 2] = num % 10 + '0';
            str_body[i + 1] = num / 10 + '0';
            i += 2;
            break;
        case '&':
            need_value = false;
            value = str_body.substr(j, i - j);
            j = i + 1;
            umap_body[key] = value;
        default:
            break;
        }
    }
    if(need_value){
        umap_body[key] = str_body.substr(j,n-j);
    }
}

HTTP_CODE MyHttpConn::vertify_user(const std::string& user,const std::string& pwd,bool binsert){
    if(user.empty() || pwd.empty()){
        return BAD_REQUEST;
    }
    MYSQL* sql_conn = nullptr;
    sqlConnRAII(&sql_conn,pSqlPool);
    if(sql_conn == nullptr){
        LOG_ERROR("No sqlConnect can be used!");
        return INTERNAL_ERRNO;
    }

    char sql_command[256];
    snprintf(sql_command, 256, "SELECT name,passwd FROM accounts WHERE name='%s' LIMIT 1", user.c_str());
    
    if(mysql_query(sql_conn,sql_command) != 0){
        LOG_ERROR("MySql Select Error![%s]",sql_command);
        return INTERNAL_ERRNO;
    }

    MYSQL_RES* pResult = mysql_store_result(sql_conn);
    int nMatchRows = mysql_num_rows(pResult);
    
    if(binsert && nMatchRows > 0){
        /*已经被注册*/
        str_response_url = "repeatRegister.html";
    }
    else if(binsert && nMatchRows == 0){
        /*可以注册*/
        snprintf(sql_command, 256, "INSERT INTO accounts(name,passwd)VALUES('%s','%s')", user.c_str(),pwd.c_str());
        if( mysql_query(sql_conn,sql_command) ){
            LOG_ERROR("Mysql insert error[%s]",sql_command);
            return INTERNAL_ERRNO;
        }
        str_response_url = "registerSucceed.html";
    }
    else if(!binsert && nMatchRows == 0){
        str_response_url =  "loginError.html";
    }
    else if(!binsert && nMatchRows > 0){
        /*检查账号密码是否一致*/
        MYSQL_ROW sql_data = mysql_fetch_row(pResult);
        std::string stored_pwd = sql_data[1];
        if(stored_pwd != pwd){
            str_response_url =  "loginError.html";
        }else{
            str_response_url = "index.html";
        }
    }
    return GET_REQUEST;

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
        /*解析请求体中各参数*/
        parse_postBody();
        HTTP_CODE cur_ret = GET_REQUEST;
        /*注册*/
        if(str_get_url.find("register") != -1){
            cur_ret = vertify_user(umap_body["name"],umap_body["pwd"],true);
        }else if(str_get_url.find("login") != -1){
            cur_ret = vertify_user(umap_body["name"],umap_body["pwd"],false);
        }else{
            str_response_url = str_get_url;
        }

        if(cur_ret == INTERNAL_ERRNO ||cur_ret == BAD_REQUEST){
            return cur_ret;
        }
    }
    else if(enMethod == GET)
    {
        /*GET请求 不对content处理*/
        /*默认发送登录页面*/
        str_response_url = str_get_url;
        if(strstr(str_get_url.data(),"pressure")) return FILE_REQUEST;
    }

    if(str_response_url.empty())
    {
        str_response_url = "login.html";
    }

    std::string strFullAddress = FULL_HTML_ROOT;
    if(strstr(str_response_url.data(),"html") == nullptr)
    {
        strFullAddress = FULL_OTH_FILE_ROOT;
    }
    strFullAddress +=  str_response_url;
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
    while(!st_rbuf.empty())
    {
        if(enCheckState == CHECK_STATE_CONTENT){
            /*主状态机：content*/
            st_rbuf.read_all(str_body);
            if(str_body.size() < nContent){
                return INCOMPLETE_REQUEST;
            }
        }else if(enCheckState == CHECK_STATE_UPLOAD){
            st_rbuf.read_all(str_body);
            /*主状态机:upload*/
            if(str_body.size() >= std::min({(int64_t)MAX_LIMIT, nContent, nContent-upsize})){

            }else{
                return INCOMPLETE_REQUEST;
            }
        }
        else{
            str_line.resize(0);
            /*其余状态 逐行解析*/
            if(!st_rbuf.find_str(str_line,"\r\n")){
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
                upsize += str_body.size();
                enCurHttpCode = processUpload();
                str_body.resize(0)
                return enCurHttpCode;
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
    if(!str_response_url.empty() && strstr(str_response_url.data(),".pdf")){
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

    LOG_INFO("DeBug:write to [%d],nBytes2Send[%d]",nSockfd,nBytes2Send);

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