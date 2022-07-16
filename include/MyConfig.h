#ifndef _MYCONFIGH_
#define _MYCONFIGH_

#include "mpServer.h"

class MyConfig
{
public:
    MyConfig();
    ~MyConfig() = default;

    void parseCommandLine(int argc,char* argv[]);
public:
    int nPort;
    int nThreadNum;
    int nMaxTasks;
    std::string strUserName;
    std::string strPwd;
    int nSqlConnNum;
    std::string strUrl;
    std::string strDBName;
    int nSqlPort;
    bool bLogOn;

};




#endif
