#include "MyConfig.h"

int main(int argc, char *argv[])
{
    MyConfig stConfig;
    stConfig.parseCommandLine(argc,argv);

    mpServer mpServer;

    mpServer.mpServerInit(stConfig.nPort,stConfig.nThreadNum,stConfig.nMaxTasks,
                            stConfig.strUserName,stConfig.strPwd,stConfig.nSqlConnNum,
                            stConfig.strUrl,stConfig.strDBName,stConfig.nSqlPort,stConfig.bLogOn,
                            stConfig.bEt,stConfig.bLinger);
    mpServer.mplogInit();

    mpServer.mpsqlPoolInit();

    mpServer.mpthreadPoolInit();

    mpServer.mphttpConnInit();

    mpServer.mpServerListen();

    mpServer.mpServerRun();

    return 0;
}