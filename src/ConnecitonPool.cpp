#include "ConnectionPool.hpp"

//线程安全的懒汉单例函数接口
ConnectionPool *ConnectionPool::getConnectionPool()
{
	static ConnectionPool pool;
	return &pool;
}

//从配置文件中加载配置项
bool ConnectionPool::loadConfigFile()
{
	FILE* fp = fopen("my.ini", "r");

	if (fp == NULL)
	{
		LOG("mysql.ini file is not exist!");
		return false;
	}

	char buf[1024] = { 0 };
	while (!feof(fp))//文件没有到末尾 
	{
		memset(buf, 0, sizeof(buf));

		fgets(buf, sizeof(buf), fp);

		string str = buf;
		int idx = str.find('=', 0);

		if (idx == -1)//无效的配置项
		{
			continue;
		}

		int endidx = str.find('\n', idx);
		string data = str.substr(0, idx);
		string value = str.substr(idx + 1, endidx - idx - 1);

		if (data == "ip")
		{
			_ip = value;
		}
		else if (data == "port")
		{
			_port = atoi(value.c_str());
		}
		else if (data == "username")
		{
			_username = value;
		}
		else if (data == "password")
		{
			_passwd = value;
		}
		else if (data == "dbname")
		{
			_dbname = value;
		}
		else if (data == "initSize")
		{
			_initSize = atoi(value.c_str());
		}
		else if (data == "maxSize")
		{
			_maxSize = atoi(value.c_str());
		}
		else if (data == "maxIdleTime")
		{
			_maxIdleTime = atoi(value.c_str());
		}
		else if (data == "connectionTimeOut")
		{
			_connectionTimeout = atoi(value.c_str());
		}
	}

	return true;
}


//连接池的构造
ConnectionPool::ConnectionPool()
{
	//加载配置项了
	if(!loadConfigFile())
	{
		return;
	}

	//创建初始数量的连接
	for (int i = 0; i < _initSize; i++)
	{
		Connection* p = new Connection;
		p->connect(_ip,_port, _username, _passwd, _dbname);
		_connectionCnt++;
		p->refreshAliveTime();//刷新一下开始空闲的起始时间
		_connectionQue.push(p);
	}

	//启动一个新的线程，作为连接的生产者 linux thread => pthread_create
	thread produce(bind(&ConnectionPool::produceConnectiontask, this));
	produce.detach();//守护线程，主线程结束了，这个线程就结束了 

	//启动一个新的定时线程，扫描超过maxIdleTime时间的空闲连接，进行对于的连接回收
	thread scanner(std::bind(&ConnectionPool::scannerConnectionTask, this));
	scanner.detach();//守护线程，主线程结束了，这个线程就结束了 

}

//运行在独立的线程中，专门负责生产新连接
void ConnectionPool::produceConnectiontask()
{
	for (;;)
	{
		unique_lock<mutex> lock(_queueMutex);
		while (!_connectionQue.empty())
		{
			cv.wait(lock);  //队列不空，此处生产线程进入等待状态
		}

		//连接数量没有到达上限，继续创建新的连接
		if (_connectionCnt < _maxSize)
		{
			Connection* p = new Connection;
			p->connect(_ip, _port, _username, _passwd, _dbname);
			p->refreshAliveTime();//刷新一下开始空闲的起始时间
			_connectionCnt++;
			_connectionQue.push(p);
		}
/*
		//连接数量到达上限
		if (_connectionCnt >= _maxSize)
		{
			continue;
		}
*/
		//通知消费者线程，可以消费连接了
		cv.notify_all();
	}
}

//给外部提供接口，从连接池中获取一个可用的空闲连接
shared_ptr<Connection> ConnectionPool::getConnection()
{
	unique_lock<mutex> lock(_queueMutex);

	while (_connectionQue.empty())//连接队列是空的
	{
		//原子地释放 lock ，阻塞当前线程，并将它添加到等待在* this 上的线程列表。
		//线程将在执行 notify_all() 或 notify_one() 时，或度过相对时限 rel_time 时被解除阻塞。它亦可被虚假地解除阻塞。
		//解阻塞时，无关缘由，重获得 lock 并退出 wait_for()。
		if (cv_status::timeout == cv.wait_for(lock, chrono::milliseconds(_connectionTimeout)))
		{
			if (_connectionQue.empty())
			{
				LOG("获取空闲连接超时了...获取连接失败!");
				return nullptr;
			}
		}
	}
	/*
	shared_ptr智能指针析构时，会把connection资源直接delete掉，相当于
	调用connection的析构函数，connection就被close掉了。
	这里需要自定义shared_ptr的释放资源的方式，把connection直接归还到queue当中
	*/

	shared_ptr<Connection> sp(_connectionQue.front(),
		[&](Connection* fp)
		{
			//这里是在服务器应用线程中调用的，所以一定要考虑队列的线程安全操作
			unique_lock<mutex> lock(_queueMutex);
			fp->refreshAliveTime();//刷新一下开始空闲的起始时间
			_connectionQue.push(fp);
		});

	_connectionQue.pop();
	cv.notify_all();//消费完连接以后，通知生产者线程检查一下，如果队列为空了，赶紧生产连接
	return sp;
}

//扫描超过maxIdleTime时间的空闲连接，进行对于的连接回收
void ConnectionPool::scannerConnectionTask()
{
	for (;;)
	{
		//通过sleep模拟定时效果
		this_thread::sleep_for(chrono::seconds(_maxIdleTime));

		//扫描整个队列，释放多余的连接
		unique_lock<mutex> lock(_queueMutex);
		while (_connectionCnt > _initSize)
		{
			Connection* p = _connectionQue.front();//队头的时间没超过，那后面的时间就都没超过 
			if (p->getAliveeTime() >= (_maxIdleTime * 1000))
			{
				_connectionQue.pop();
				_connectionCnt--;
				delete p;//调用~Connection()释放连接
			}
			else
			{
				break;//队头的连接没有超过_maxIdleTime，其它连接肯定没有
			}
		}
	}
}