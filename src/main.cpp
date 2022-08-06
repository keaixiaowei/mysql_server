#include "pch.hpp"
#include <iostream>
#include <string>
using namespace std;
#include "Connection.hpp"
#include "ConnectionPool.hpp"

int main()
{
	clock_t begin = clock();

	ConnectionPool* cp = ConnectionPool::getConnectionPool();

	for (int i = 0; i < 5000; i++)
	{
/*
		Connection conn;

		string sql = "insert into employee(sid,name,sex,salary) values(4,'caiwei','male',11111)";

		conn.connect("127.0.0.1", 3306, "root", "123456", "myb1");

		conn.update(sql);
*/

		shared_ptr<Connection> sp = cp->getConnection();

		string sql = "insert into employee(sid,name,sex,salary) values(4,'caiwei','male',11111)";

		sp->update(sql);

	}

	clock_t end = clock();

	cout << (end - begin) << "ms" << endl;

	return 0;
}