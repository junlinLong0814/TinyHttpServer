#ifndef _MYTOOL_H_
#define _MYTOOL_H_

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <string.h>
#include <string>

#include <time.h>

class MyTool
{
public:
    /*非阻塞fd*/
    int setNonBlock(int nFd);
    /*在epoll上注册监听事件*/
    void addfd2Epoll(int nFd,int nEpollFd,int nOpt,bool bOneShot,bool bEt,bool bUseLinger = false,int nLingerTime = 1);
    /*添加信号*/
    int addSig(int nSignal,void(handler)(int),bool bRestart = true);
    /*设置linger关闭*/
    int setFdLinger(int nFd,int nLingerTime);
    /*从epoll移除fd*/
    int removefd(int nFd,int nEpollFd);
    /*mod fd */
    int modfd(int nFd,int nEpollFd,int nOpt = EPOLLIN,bool bOneShot = true);
    /*信号处理函数*/
    static void handler(int nSignal);
public:
    static int* spPipeFd;
    static int snEpollFd;
};

#endif