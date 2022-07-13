#ifndef _MYSQLCONNPOOL_H_
#define _MYSQLCONNPOOL_H_

#include <stdio.h>
#include <list>
#include <mysql/mysql.h>
#include <error.h>
#include <string.h>
#include <string>
#include <iostream>
#include <mutex>
#include "MySemaphore.h"
#include "MyLog.h"

class MySqlConnPool
{
public:
	/*获取数据库连接*/
	MYSQL* getConnection();
	/*释放数据库连接*/
	bool releaseConnection(MYSQL* con);
	/*返回当前空闲连接数*/
	int getFreeConnNum();
	/*销毁所有连接*/
	void destoryPool();
	/*初始化*/
	void sqlConnPoolInit(std::string url,std::string user,std::string pwd,std::string dbName,int port,int maxConn,int close_log);

	/*返回连接池实例*/
	static MySqlConnPool* getSqlConnPoolInstance();
private:
	MySqlConnPool();
	~MySqlConnPool();
private:
	mySemaphore mySem_;					//信号量
	int maxConn_;						//最大连接数
	int currConn_;						//当前连接数
	int freeConn_;						//当前空闲连接数
	std::list<MYSQL*> connPool_;	 	//连接池
	std::mutex mutex_;					//互斥访问连接池list

	std::string url_;					//主机地址
	std::string user_;					//登录数据库用户名
	std::string pwd_;					//登录数据库密码
	std::string dbName_;				//数据库名称
	int port_;							//端口地址
	int close_log_;						//日志开关

};

class sqlConnRAII
{
public:
	sqlConnRAII(MYSQL** con,MySqlConnPool* connPool);
	~sqlConnRAII();
private:
	MYSQL* conRAII;
	MySqlConnPool* poolRAII;
};

#endif
