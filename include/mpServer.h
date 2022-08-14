#ifndef _MPSERVER_H
#define _MPSERVER_H

#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <vector>
#include <cassert>
#include <map>
#include <mutex>
#include <thread>
#include "MyTool.h"
#include "MyLog.h"
#include "MySemaphore.h"
#include "MySqlConnPool.h"
#include "MyThreadPool.h"
#include "MyHttpConn.h"

#define MAX_FD 65536
#define MAX_EVENT_NUMBER 10000 

class mpServer
{
public:
    /*服务器运行*/
    void mpServerRun();
    /*服务器初始化*/
    void mpServerInit(int nPort,
                      int nThreadNum,
                      int nMaxTasks,
                      std::string strUserName,
                      std::string strPwd,
                      int nSqlConnNum,
                      std::string strUrl,
                      std::string strDBName,
                      int nSqlPort,
                      bool bLog,
                      bool bEt);
    void mpServerInit();
    /*开始监听*/
    void mpServerListen();
    /*日志单例初始化*/
    void mplogInit();
    /*数据库连接池初始化*/
    void mpsqlPoolInit();
    /*线程池初始化*/
    void mpthreadPoolInit();
    /*httpconn初始化*/
    void mphttpConnInit();
    /*处理新到来的用户*/
    bool dealWithNewUser();
    /*处理离开的用户*/
    bool dealExitUser(int nSockfd);
    /*处理信号*/
    int dealWithSignal(bool& bStop);
    /*处理待读的fd*/
    int dealWithUserRead(int nSockfd);
    /*处理待写的fd*/
    int dealWithUserWrite(int nSockfd);
public:
    mpServer();
    ~mpServer();

private:
    /*socket需要的基础属性*/
    struct sockaddr_in stServerAddress;
    unsigned int unPort;
    int nListenfd; 

    /*Http*/
    MyHttpConn arrUsers[MAX_FD];

    /*Epoll*/
    epoll_event stEvents[MAX_EVENT_NUMBER]; 
    int nEpollFd;
    bool bEt; 
    

    /*线程池*/
    std::unique_ptr<MyThreadPool> pThreadPool;
    int nThreadNum;
    int nMaxTasks;

    /*数据库连接池*/
    MySqlConnPool *pSqConnPool;
    std::string strUrl;
    std::string strUserName;
    std::string strPwd;
    std::string strDBName;
    int nSqlPort;
    int nSqlConnNum;

    /*日志*/
    MyLog *pLog;
    bool bLogOn;

    /*工具类*/
    MyTool stTool;
    int pPipeFd[2];


};


#endif