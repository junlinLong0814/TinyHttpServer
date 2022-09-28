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
#include <string_view>
#include "MyLog.h"
#include "MySqlConnPool.h"
#include "MyTool.h"
#include "MySemaphore.h"
#include "MyBuff.h"

#define RECVBUF_SIZE 2048
#define SENDBUF_SIZE 2048
#define FILENAME_SIZE 100
#define FILE_MODE (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH) 


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
3.请求分析请求内容
4.上传
*/
enum CHECK_STATE
{
    CHECK_STATE_REQUESTLINE=0,
    CHECK_STATE_REQUESTHEADER,
    CHECK_STATE_CONTENT,
    CHECK_STATE_UPLOAD    
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
    UPLOAD_REQUEST,
    NO_REQUEST
};


class MyHttpConn
{
public:
    MyHttpConn(int nFd = -1) ;
    ~MyHttpConn() ;

public:
    /*供线程池使用的工作函数*/
    void process(int nfd);
    void write(int nfd);
    void write(int nfd,bool& bflag);

    /*供服务器使用的初始化*/
    void httpConnInit(bool bEt);

private:
    /*供内部使用的全初始化*/
    void httpConnInit();

    /*------接收-------*/
    bool read();
    /*处理读*/
    HTTP_CODE processRead();
    /*分析请求行*/
    HTTP_CODE parseRequestLine();
    /*分析请求头*/
    HTTP_CODE parseRequestHeader();
    /*解析content*/
    HTTP_CODE parseContent();
    /*接收上传文件*/
    HTTP_CODE processUpload();

    /*-------发送----------*/
    /*处理写*/
    bool processWrite(HTTP_CODE enRet);
    /*响应行*/
    bool addStatusLine(int nStatus,const char* cpTitle);
    /*响应头*/
    bool addResponseHeader(int nWait2SendSize);
    /*正文长度*/
    bool addContentLength(int nWait2SendSize);
    /*正文类型*/
    bool addContentType(const char* cpContentStyle);
    /*keep-alive*/
    bool addLinger();
    /*/r/n*/
    bool addBlankLine();
    /*content*/
    bool addContent(const char* cpContent);
    /*多参数格式填入*/
    bool addInFormat(const char *format,...);
    


    /*关闭连接*/
    bool closeConn();
    void unmap();

public:
    static int hc_snEpollFd;
    static int hc_snUsedCount;
private:
    char carrSendBuf[SENDBUF_SIZE];

    struct stat stFileStat;
    struct iovec stIc[2];
    int nIcCount;

    int nSockfd;                                            //socket
    MySqlConnPool* pSqlPool;                                //mysql连接池

    bool bEt;                                               //是否ET读

    bool bLinger;                                           //是否keep-live
    int nContent;                                           //请求主体长度					                        //指向解析最新数据的最后一个行尾
    CHECK_STATE enCheckState;		                        //主状态机状态	
	
    METHOD enMethod;					                    //请求方法

    int nLastPosInSend;                                     //指向发送缓冲区最后的位置
    int nBytes2Send;                                        //待发送的字节数
    int nBytesHadSend;                                      //已发送的字节数
    int nTotal;                                             //每次请求发送的总字节

    char  cpUploadType[FILENAME_SIZE];                      //上传文件类型
    bool  bUpload;                                          //此次请求是否是上传请求
    
    char* cpFileAddress;

    Buffer stBuffer;
    std::string str_body;
    std::string str_line;
    std::string str_url;
    std::string str_version;
    std::string str_host;
};


#endif
