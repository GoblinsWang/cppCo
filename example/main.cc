/***
	@author: Wangzhiming
	@date: 2022-10-29
***/
#include <iostream>
#include <sys/sysinfo.h>

#include "../include/processor.h"
#include "../include/co_api.h"
#include "../include/socket.h"
#include "../include/mutex.h"

using namespace cppCo;

// cppCo http response with one acceptor test
// 只有一个acceptor的服务
void single_acceptor_server_test()
{
	cppCo::co_go(
		[]
		{
			cppCo::Socket listener;
			if (listener.isUseful())
			{
				listener.setTcpNoDelay(true);
				listener.setReuseAddr(true);
				listener.setReusePort(true);
				if (listener.bind(8888) < 0)

				{
					return;
				}
				listener.listen();
			}
			while (1)
			{
				cppCo::Socket *conn = new cppCo::Socket(listener.accept());
				conn->setTcpNoDelay(true);
				cppCo::co_go(
					[conn]
					{
						std::vector<char> buf;
						buf.resize(2048);
						while (1)
						{
							auto readNum = conn->read((void *)&(buf[0]), buf.size());
							std::string ok = "HTTP/1.0 200 OK\r\nServer: cppCo/0.1.0\r\nContent-Type: text/html\r\n\r\n";
							if (readNum < 0)
							{
								break;
							}
							conn->send(ok.c_str(), ok.size());
							conn->send((void *)&(buf[0]), readNum);
							if (readNum < (int)buf.size())
							{
								break;
							}
						}
						cppCo::co_sleep(100); // 需要等一下，否则还没发送完毕就关闭了
						delete conn;
					});
			}
		});
}

// cppCo http response with multi acceptor test
// 每条线程一个acceptor的服务
void multi_acceptor_server_test()
{
	auto tCnt = ::get_nprocs_conf();
	for (int i = 0; i < tCnt; ++i)
	{
		cppCo::co_go(
			[]
			{
				cppCo::Socket listener;
				if (listener.isUseful())
				{
					listener.setTcpNoDelay(true);
					listener.setReuseAddr(true);
					listener.setReusePort(true);
					if (listener.bind(8888) < 0)
					{
						return;
					}
					listener.listen();
				}
				while (1)
				{
					cppCo::Socket *conn = new cppCo::Socket(listener.accept());
					conn->setTcpNoDelay(true);
					cppCo::co_go(
						[conn]
						{
							std::string hello("HTTP/1.0 200 OK\r\nServer: cppCo/0.1.0\r\nContent-Length: 72\r\nContent-Type: text/html\r\n\r\n<HTML><TITLE>hello</TITLE>\r\n<BODY><P>hello word!\r\n</BODY></HTML>\r\n");
							// std::string hello("<HTML><TITLE>hello</TITLE>\r\n<BODY><P>hello word!\r\n</BODY></HTML>\r\n");
							char buf[1024];
							if (conn->read((void *)buf, 1024) > 0)
							{
								conn->send(hello.c_str(), hello.size());
								cppCo::co_sleep(50); // 需要等一下，否则还没发送完毕就关闭了
							}
							delete conn;
						});
				}
			},
			parameter::coroutineStackSize, i);
	}
}

// 作为客户端的测试，可配合上述server测试
void client_test()
{
	cppCo::co_go(
		[]
		{
			char buf[1024];
			while (1)
			{
				cppCo::co_sleep(2000);
				cppCo::Socket s;
				s.connect("127.0.0.1", 8099);
				s.send("ping", 4);
				s.read(buf, 1024);
				std::cout << std::string(buf) << std::endl;
			}
		});
}

// 读写锁测试
void mutex_test(cppCo::RWMutex &mu)
{
	for (int i = 0; i < 10; ++i)
		if (i < 5)
		{
			cppCo::co_go(
				[&mu, i]
				{
					mu.rlock();
					std::cout << i << " : start reading" << std::endl;
					cppCo::co_sleep(50 + i);
					std::cout << i << " : finish reading" << std::endl;
					mu.runlock();
					mu.wlock();
					std::cout << i << " : start writing" << std::endl;
					cppCo::co_sleep(50);
					std::cout << i << " : finish writing" << std::endl;
					mu.wunlock();
				});
		}
		else
		{
			cppCo::co_go(
				[&mu, i]
				{
					mu.wlock();
					std::cout << i << " : start writing" << std::endl;
					cppCo::co_sleep(50);
					std::cout << i << " : finish writing" << std::endl;
					mu.wunlock();
					mu.rlock();
					std::cout << i << " : start reading" << std::endl;
					cppCo::co_sleep(50);
					std::cout << i << " : finish reading" << std::endl;
					mu.runlock();
				});
		}
}

int main()
{
	cppCo::RWMutex mu;
	// mutex_test(mu);
	// single_acceptor_server_test();
	multi_acceptor_server_test();
	// client_test();
	cppCo::sche_join();
	std::cout << "end" << std::endl;
	return 0;
}
