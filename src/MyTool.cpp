#include "MyTool.h"

int *MyTool::spPipeFd = 0;
int MyTool::snEpollFd = 0;

int MyTool::addSig(int nSignal,void(handler)(int),bool bRestart)
{
    struct sigaction stSigation;
    memset(&stSigation,'\0',sizeof(stSigation));
    stSigation.sa_handler = handler;
    if(bRestart)
    {
        stSigation.sa_flags |= SA_RESTART;
    }
    sigfillset(&stSigation.sa_mask);
    int nRet = sigaction(nSignal,&stSigation,NULL); 
    assert(nRet != -1);
    
}

int MyTool::modfd(int nFd,int nEpollFd,int nOpt,bool bOneShot)
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

void MyTool::handler(int nSignal)
{
    int nSavedErrno = errno;
    int nMsg = nSignal;
    write(spPipeFd[1],(const void*)&nMsg,1);
    //send(spPipeFd[1],(char*)&nMsg,1,0);
    errno = nSavedErrno;
}

int MyTool::setNonBlock(int nFd)
{
    int nOldOption = fcntl(nFd,F_GETFL);
    int nNewOption = nOldOption | O_NONBLOCK;
    fcntl(nFd,F_SETFL,nNewOption);
    return nOldOption;
}

int MyTool::setFdLinger(int nFd,int nLingerTime)
{
    struct linger stLinger = {1,nLingerTime};
    setsockopt(nFd,SOL_SOCKET,SO_LINGER,&stLinger,sizeof(stLinger));
}

int MyTool::removefd(int nFd,int nEpollFd)
{
    epoll_ctl(nEpollFd,EPOLL_CTL_DEL,nFd,0);
    if(nFd != -1)
    {
        close(nFd);
    }
    
}

void MyTool::addfd2Epoll(int nFd,int nEpollFd,int nOpt,bool bOneShot,bool bEt,bool bUseLinger,int nLingerTime)
{
    epoll_event stEpollEvent;
    stEpollEvent.data.fd = nFd;
    stEpollEvent.events = EPOLLIN | EPOLLRDHUP;
    if(bEt)
    {
        stEpollEvent.events |= EPOLLET;
    }
    if(bOneShot)
    {
        stEpollEvent.events |= EPOLLONESHOT;
    }
    setNonBlock(nFd);
    epoll_ctl(nEpollFd, EPOLL_CTL_ADD, nFd, &stEpollEvent);
    if(bUseLinger)
    {
        setFdLinger(nFd,nLingerTime);
    }
}
