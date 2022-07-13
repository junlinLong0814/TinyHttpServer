#ifndef _MYHTTPCONN_H
#define _MYHTTPCONN_H

#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fstream>
#include <string.h>
#include <string>
#include <iostream>
#include <sys/mman.h>
#include <stdarg.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h> 
#include "MyLog.h"
#include "MySqlConnPool.h"
#include "MyTool.h"
#include "MySemaphore.h"

#define RECVBUF_SIZE 4096
#define SENDBUF_SIZE 4096


/*HTTP请求方法*/
enum METHOD
{
	GET=0,
	POST,
	HEAD,
	PUT,
	DELETE,
	TRACE,
	OPTIONS,
	CONNECT,
	PATCH
};

/*主状态机状态
1.请求分析请求行
2.请求分析请求头
3.请求分析请求内容*/
enum CHECK_STATE
{
    CHECK_STATE_REQUESTLINE=0,
    CHECK_STATE_REQUESTHEADER,
    CHECK_STATE_CONTENT
};

/*从状态机的三种状态
1.读取到一个完整的行
2.读取到一个不完整的行
3.读取到一个错误的行（即HTTP格式错误）
 */
enum LINE_STATUS
{
    LINE_COMPLETE=0,
    LINE_CONTINUE,
    LINE_BAD
};

/*HTTP的解析结果
1.GET请求
2.请求不完整
3.请求不合规
*/
enum HTTP_CODE
{
    GET_REQUEST = 0,    //处理完一个完整的http请求
    INCOMPLETE_REQUEST,
    BAD_REQUEST,
    NO_RESOURCE,
    FORBIDDEN_REQUEST,
    FILE_REQUEST,
    INTERNAL_ERRNO,
    CLOSED_CONNECTION,
    NO_REQUEST
};



class MyHttpConn
{
public:
    MyHttpConn(int nFd = -1) ;
    ~MyHttpConn() ;

public:
    void process();
    void write();

private:
    void httpConnInit();
    /*ET模式读*/
    bool read();
    /*处理写*/
    bool processWrite();
    /*处理读*/
    HTTP_CODE processRead();
    /*解析行内容*/
    LINE_STATUS parseLine();
    /*分析请求行*/
    HTTP_CODE parseRequestLine();
    /*分析请求头*/
    HTTP_CODE parseRequestHeader();
    /*解析content*/
    HTTP_CODE parseContent();

    /*关闭连接*/
    bool closeConn();

public:
    static int hc_snEpollFd;
    static int hc_snUsedCount;
private:
    char carrRecvBuf[RECVBUF_SIZE];
    char carrSendBuf[SENDBUF_SIZE];

    //std::unique_ptr<MyTool> upTool;

    int nSockfd;                        //socket
    MySqlConnPool* pSqlPool;            //mysql连接池

    bool bLinger;                   //是否keep-live
    int nContent;
    int nRowStart;					//http报文每行行首
	int nRowEnd;					//指向解析最新数据的最后一个行尾
    int nLastPosInRecv;             //指向读取接收缓冲区的最后一个位置
    CHECK_STATE enCheckState;		    //主状态机状态	
	
    METHOD enMethod;					//请求方法
    char* cpUrl;
    char* cpVersion;
    char* cpHost;

    char* cpFileAddress;


};


#endif
