#include "mpServer.h"

mpServer::mpServer() : unPort(8888),nThreadNum(4),nMaxTasks(10000),strUserName("ljl"),strPwd("123456"),nSqlConnNum(10),strUrl("localhost"),strDBName("test"),nSqlPort(15)
{
    memset(pPipeFd,'\0',sizeof(pPipeFd));
    memset(arrUsers,'\0',sizeof(arrUsers));
}

void mpServer::mpServerInit()
{

}

void mpServer::mpServerInit(int nPort,
                            int nThreadNum,
                            int nMaxTasks,
                            std::string strUserName,
                            std::string strPwd,
                            int nSqlConnNum,
                            std::string strUrl,
                            std::string strDBName,
                            int nSqlPort)
{
    unPort = nPort < 0 ? 8888 : nPort;
    nThreadNum = nThreadNum < 0 ? 4 : nThreadNum;
    nMaxTasks = nMaxTasks < 0 ? 10000 : nMaxTasks;
    strUserName = strUserName;
    strPwd = strPwd;
    nSqlConnNum = nSqlConnNum;
    strUrl = strUrl;
    strDBName = strDBName;
    nSqlPort = nSqlPort < 0 ? 15 : nSqlPort;
    //printf("Server Init Succeed\n");
}

void mpServer::mplogInit()
{
    pLog = MyLog::getLogInstance();
    pLog->logInit();
}

void mpServer::mpthreadPoolInit()
{
    std::unique_ptr<MyThreadPool> pTmp(new MyThreadPool());
    pThreadPool = move(pTmp);
    pThreadPool->threadPoolInit(nThreadNum,nMaxTasks);
    LOG_INFO("[%d] Threads be created!",nThreadNum);
}

void mpServer::mpsqlPoolInit()
{
    pSqConnPool = MySqlConnPool::getSqlConnPoolInstance();
    pSqConnPool->sqlConnPoolInit(strUrl,strUserName,strPwd,strDBName,nSqlPort,nSqlConnNum,false);
    LOG_INFO("Dbs<%s>:[usr:%s,pwd:%s,count:%d]",strDBName.c_str(),strUserName.c_str(),strPwd.c_str(),nSqlConnNum);
}

void mpServer::mphttpConnInit()
{
    for(int i = 0 ; i < MAX_FD ; ++i)
    {
        arrUsers[i].httpConnInit();
    }
}

void mpServer::mpServerListen()
{
    nListenfd = socket(PF_INET,SOCK_STREAM,0);
    assert(nListenfd >= 0);

    /*优雅关闭连接*/
    struct linger stLinger = {0,0};
    setsockopt(nListenfd,SOL_SOCKET,SO_LINGER,&stLinger,sizeof(stLinger));
    /*为了kill掉server后能在timewait时重新bind地址*/
    int ntmp = 1;
    setsockopt(nListenfd,SOL_SOCKET,SO_REUSEADDR,&ntmp,sizeof(ntmp));

    bzero(&stServerAddress,sizeof(stServerAddress));
    stServerAddress.sin_family = AF_INET;
    stServerAddress.sin_port = htons(unPort);
    stServerAddress.sin_addr.s_addr = htonl(INADDR_ANY);

    int nret = bind(nListenfd,(struct sockaddr*)&stServerAddress,sizeof(stServerAddress));
    assert(nret != -1);

    nret = listen(nListenfd,SOMAXCONN);
    assert(nret != -1);

    LOG_INFO("Server Start Listening Port[%d]",unPort);
    
    nEpollFd = epoll_create(5);
    assert(nEpollFd != -1);

    nret = socketpair(PF_UNIX,SOCK_STREAM,0,pPipeFd);
    assert(nret != -1);

    stTool.setNonBlock(pPipeFd[1]);
    stTool.addfd2Epoll(pPipeFd[0],nEpollFd,EPOLLIN,false,false);
    stTool.addfd2Epoll(nListenfd,nEpollFd,EPOLLIN,false,false);

    stTool.addSig(SIGPIPE,SIG_IGN);
    //stTool.addSig(SIGTERM,stTool.handler,false);
    //stTool.addSig(SIGKILL,stTool.handler,false);
    //stTool.addSig(SIGINT,stTool.handler,false);

    MyTool::snEpollFd = nEpollFd;
    MyTool::spPipeFd = pPipeFd;
    MyHttpConn::hc_snEpollFd = nEpollFd;

}


void mpServer::mpServerRun()
{
    bool bStopServer = false;
    
    while(!bStopServer)
    {
        int nActiveFd = epoll_wait(nEpollFd,stEvents,MAX_EVENT_NUMBER,-1);
        if(nActiveFd < 0 && errno != EINTR)
        {
            LOG_ERROR("Epoll failure");
            break;
        }
        
        for(int i = 0 ; i < nActiveFd; ++i)
        {
            int nSockfd = stEvents[i].data.fd;
            LOG_INFO("client<%d>,events:%d",nSockfd,stEvents[i].events);
            if(nSockfd == nListenfd)
            {
                /*新接收的客户*/
                dealWithNewUser();
            }
            else if(nSockfd == pPipeFd[0] && (stEvents[i].events & EPOLLIN))
            {
                /*待处理的信号*/
                if(0 != dealWithSignal(bStopServer))
                {
                    LOG_ERROR("Deal signal error[%d]!",errno);
                }
            }   
            else if(stEvents[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR) )
            {
                /*关闭该fd*/
                dealExitUser(nSockfd);
            }
            else if(stEvents[i].events & EPOLLIN)
            {
                /*用户可读事件*/
                if(0!=dealWithUserRead(nSockfd))
                {
                    LOG_WARNING("Read from fd[%d] error!",nSockfd);
                    continue;
                }
            }
            else if(stEvents[i].events & (EPOLLOUT))
            {
                /*用户可写事件*/
                if(0!=dealWithUserWrite(nSockfd))
                {
                    LOG_WARNING("Write in fd[%d] error!",nSockfd);
                    continue;
                }
            }
            
        }
    }
}

int mpServer::dealWithUserWrite(int nSockfd)
{
    if(nSockfd < 0 || nSockfd >= MAX_FD)
    {
        return -1;
    }
    pThreadPool->addTask(
            [this,nSockfd]()
            {
                bool flag = false;
                this->arrUsers[nSockfd].write(nSockfd,flag);
            });
    return 0;
}

int mpServer::dealWithUserRead(int nSockfd)
{
    if(nSockfd < 0 || nSockfd >= MAX_FD)
    {
        return -1;
    }
    pThreadPool->addTask(
        [this,nSockfd]()
        {
            this->arrUsers[nSockfd].process(nSockfd);
        });
    return 0;
}

bool mpServer::dealExitUser(int nSockfd)
{
    stTool.removefd(nSockfd,nEpollFd);
    close(nSockfd);
    --MyHttpConn::hc_snUsedCount;
}

bool mpServer::dealWithNewUser()
{
    struct sockaddr_in stClientAddress;
    socklen_t unClientAddrLength = sizeof(stClientAddress);
    bzero(&stClientAddress,(size_t)unClientAddrLength);

    int nUserFd = accept(nListenfd,(struct sockaddr*)&stClientAddress,&unClientAddrLength);
    if(nUserFd < 0)
    {
        LOG_ERROR("Accept new user failed! errno:%d",errno);
        return false;
    }
    if(MyHttpConn::hc_snUsedCount >= MAX_FD)
    {
        LOG_WARNING("Too much user!");
        close(nUserFd);
        return false;
    }
    
    stTool.addfd2Epoll(nUserFd,nEpollFd,EPOLLIN,true,true,true,0);
    ++MyHttpConn::hc_snUsedCount;
    LOG_INFO("a new client");
    return true;

}


int mpServer::dealWithSignal(bool& bStopServer)
{
    int nRet = 0 ;
    char cSignalBuf[1024];
    memset(cSignalBuf,'\0',sizeof(cSignalBuf));
    nRet = recv(pPipeFd[0],cSignalBuf,sizeof(cSignalBuf),0);
    if(nRet == -1)
    {
        return -1;
    }
    else if(nRet == 0)
    {
        return -1;
    }
    else
    {
        for(int i = 0 ; i < nRet ; ++i)
        {
            switch(cSignalBuf[i])
            {
                case SIGTERM : 
                {
                    bStopServer = true;
                    printf("SIGTERM\n");
                    break;
                }
                case SIGKILL:
                {
                    bStopServer = true;
                    printf("SIGKILL\n");
                    break;
                }
                break;
            }
        }
    }
    return 0;
}


mpServer::~mpServer()
{
    if(nEpollFd > 0) close(nEpollFd);
    if(nListenfd > 0) close(nListenfd);
    if(pPipeFd != nullptr) 
    {
        close(pPipeFd[0]);
        close(pPipeFd[1]);
    }
}



