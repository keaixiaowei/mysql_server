#ifndef CONNECTION_H
#define CONNECTION_H
/*
数据库操作代码、增删该查代码实现
*/
#include <iostream>
#include "mysql/mysql.h"
#include <string>
#include <ctime>
using namespace std;
#include "public.hpp"

// 数据库操作类
class Connection
{
public:
	// 初始化数据库连接
	Connection();

	// 释放数据库连接资源
	~Connection();

	// 连接数据库
	bool connect(string ip,
		unsigned short port,
		string user,
		string password,
		string dbname);

	// 更新操作 insert、delete、update
	bool update(string sql);

	// 查询操作 select
	MYSQL_RES* query(string sql);

		//刷新一下连接的起始的空闲时间点
	void refreshAliveTime();

	//返回存活的时间
	clock_t getAliveeTime()const;

private:
	MYSQL* _conn; // 表示和MySQL Server的一条连接
	clock_t _alivetime;//记录进入空闲状态后的起始存活时间
};

#endif // !