#ifndef _LOGGING_H_
#define _LOGGING_H_

#include <chrono>
#include <string.h>
#include <sstream>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <mutex>
#include <vector>

#ifndef LOG_BUF_SIZE
#define LOG_BUF_SIZE 2048	
#endif 

class MyLog
{
public:
	static MyLog* getLogInstance()
	{
		static MyLog myLog;
		return &myLog;
	}

	/*日志初始化*/
	bool logInit(int maxBlockSize = 1);

	bool newWriteLog(std::string strInfoType,
					const char* pcFileName,
					const int nLine,
					const char* pcFunctionName,
					const char* time,
					const char *fmt, ...);

private:
	/*将一条日志语句加到缓冲区*/
	void appendBuf();

	MyLog();
	~MyLog();

private:
	char 											ctmpLogFileBuf[LOG_BUF_SIZE];	//待写入日志文件的缓冲区
	char 											cdirName_[256];					//路径名
	char 											cfileName_[256];				//Log文件名
	char 											clogBuf_[1024];					//Log日志内容
	
	std::mutex 										mufile_;						//互斥访问文件指针		
	std::mutex 										mlogBuf;						//互斥访问clogBuf
	std::mutex 										mlogFileBuf;					//互斥访问ctmpLogFileBuf	
	
	int 											ntoday_;						//记录天数
	int 											nmonth_;						//记录月份
	int 											nyear_;							//记录年份
	int 											nbufLastIdx;					//指向缓冲区结尾位置
	FILE 											*fplogFile_;					//指向日志文件指针
	
	//std::string 									slog;				
	//std::string 									slog2;
	//bool 										bcanWriteFile;								//指示线程是否可以进行文件的写
	//std::shared_ptr<std::thread>	pWriteLogThread;					//异步写入日志文件
	//std::condition_variable 			cv;												//唤醒线程写文件
};


#define LOG_DEBUG(...) {MyLog::getLogInstance()->newWriteLog("[DEBUG]:",__FILE__,__LINE__,__FUNCTION__,__TIME__,##__VA_ARGS__ );}
#define LOG_INFO(...) {MyLog::getLogInstance()->newWriteLog("[INFO]:",__FILE__,__LINE__,__FUNCTION__,__TIME__,##__VA_ARGS__ );}
#define LOG_WARNING(...) {MyLog::getLogInstance()->newWriteLog("[WARNING]:",__FILE__,__LINE__,__FUNCTION__,__TIME__,##__VA_ARGS__ );}
#define LOG_ERROR(...) {MyLog::getLogInstance()->newWriteLog("[ERROR]:",__FILE__,__LINE__,__FUNCTION__,__TIME__,##__VA_ARGS__ );}

// #define LOG_DEBUG(...) {MyLog::GetLogInstance()->newWriteLog("[Debug]:", ##__VA_ARGS__);}
// #define LOG_INFO(...) {MyLog::GetLogInstance()->newWriteLog("[Info]:", ##__VA_ARGS__);}
// #define LOG_WARNING(...) {MyLog::GetLogInstance()->newWriteLog("[Warning]:", ##__VA_ARGS__);}
// #define LOG_ERROR(...) {MyLog::GetLogInstance()->newWriteLog("[Error]:", ##__VA_ARGS__);}

#endif