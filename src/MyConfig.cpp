#include "MyConfig.h"

MyConfig::MyConfig()
{
    nPort = 8888;
    nThreadNum = 4;
    nMaxTasks = 10000;
    strUserName = "ljl";
    strPwd = "123456";
    nSqlConnNum = 10;
    strUrl = "localhost";
    strDBName = "test";
    nSqlPort = 15;
    bLogOn = true;
    bEt = false;
}

void MyConfig::parseCommandLine(int argc,char* argv[])
{
    int nopt ;
    const char* cpOption = "s::p::t::l::e";
    bool bStartConfig = true;
    while(((nopt = getopt(argc,argv,cpOption)) != -1) && 
            bStartConfig)
    {
        switch(nopt)
        {
            case 'p':
            {
                nPort = atoi(optarg);
                break;
            }
            case 't':
            {
                nThreadNum = atoi(optarg);
                break;
            }
            case 'l':
            {
                bLogOn = atoi(optarg);
                break;
            }
            case 'e':
            {
                bEt = true;
            }
            default:
            {
                break;
            }
        }
    }
}