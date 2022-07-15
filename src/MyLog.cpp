#include "MyLog.h"

MyLog::MyLog() :fplogFile_(nullptr), nbufLastIdx(0)
{
}

MyLog::~MyLog()
{
	if (fplogFile_)
	{
		//如果退出时缓冲区还有内容 冲出日志文件
		if (nbufLastIdx != 0)
		{
			fputs(ctmpLogFileBuf, fplogFile_);
			fflush(fplogFile_);
		}
		fclose(fplogFile_);
		fplogFile_ = nullptr;
	}
}


void MyLog::appendBuf()
{
	int nRealThisLogSize = strlen(clogBuf_) ;

	std::lock_guard<std::mutex> locklogbuf(mlogFileBuf);
	std::lock_guard<std::mutex> lockfile(mufile_);
	int nBufLastIdx=0;
	while(nRealThisLogSize != 0)
	{
		int nThisFlushSize = std::min((int)nRealThisLogSize,(int)sizeof(ctmpLogFileBuf));
		memcpy(ctmpLogFileBuf,clogBuf_+nBufLastIdx,nThisFlushSize);
		//预防非法访址
		ctmpLogFileBuf[nThisFlushSize-1]='\0';
		fputs(ctmpLogFileBuf, fplogFile_);
		fflush(fplogFile_);
		nBufLastIdx += nThisFlushSize;
		nRealThisLogSize -= nThisFlushSize;
	}
	ctmpLogFileBuf[0]='\0';
}

bool MyLog::logInit(bool bLog)
{

	memset(cdirName_, '\0', sizeof(cdirName_));
	memset(cfileName_, '\0', sizeof(cfileName_));
	memset(clogBuf_, '\0', sizeof(clogBuf_));
	memcpy(cdirName_, "logDir/server_", 255);

	memset(ctmpLogFileBuf, '\0', sizeof(ctmpLogFileBuf));

	time_t tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	struct tm &tm = *localtime(&tt);
	int nyear = (tm.tm_year + 1900);
	int nmonth = (tm.tm_mon + 1);
	int nday = (tm.tm_mday);
	snprintf(cfileName_, 255, "%s%d_%02d_%02d%s", cdirName_, nyear, nmonth, nday, ".log");
	fplogFile_ = fopen(cfileName_, "a");

	if (!fplogFile_)
	{
		return false;
	}

	nyear_ = nyear;
	nmonth_ = nmonth;
	ntoday_ = nday;

	bLogOn = bLog;
	return true;

}

bool MyLog::newWriteLog(std::string strInfoType,
					const char* pcFileName,
					const int nLine,
					const char* pcFunctionName,
					const char* pcTime,
					const char *fmt, ...)
{
	time_t tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	struct tm &tm = *localtime(&tt);

	int ntoday = (tm.tm_mday);
	int nmonth = (tm.tm_mon + 1);
	int nyear = (tm.tm_year + 1900);

	if (ntoday != ntoday_
		|| nmonth != nmonth_
		|| nyear != nyear_)
	{
		std::lock_guard<std::mutex> lock(mufile_);
		ntoday_ = ntoday;
		nmonth_ = nmonth;
		nyear_ = nyear;
		memset(cfileName_, '\0', sizeof(cfileName_));
		snprintf(cfileName_, 255, "%s%d_%02d_%02d%s", cdirName_, nyear, nmonth, ntoday, ".log");
		fplogFile_ = fopen(cfileName_, "a");
		if (!fplogFile_)
		{
			return false;
		}
	}
	//组装log实际内容
	char s[256];
	memset(s,'\0',sizeof(s));
	va_list ap;
    int n=0;
    va_start(ap,fmt);
    n=vsnprintf(s,sizeof(s),fmt,ap);
    va_end(ap);
	//连接函数名
	std::string tmp = s;
	tmp+="\n";
	{
		std::lock_guard<std::mutex> lock(mlogBuf);
		memset(clogBuf_, '\0', sizeof(clogBuf_));
		snprintf(clogBuf_,sizeof(clogBuf_)-1, "%s %04d-%02d-%02d %s [%s/%s:%d] %s\n",strInfoType.c_str(),nyear,nmonth, ntoday, pcTime, pcFileName,pcFunctionName,nLine,tmp.c_str());
		appendBuf();
	}

    return n;
}
