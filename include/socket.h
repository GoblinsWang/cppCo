/***
	@author: Wangzhiming
	@date: 2021-10-29
***/
#pragma once

#include "utils.h"
#include "parameter.h"

#include <arpa/inet.h>
#include <sys/types.h>
#include <string>
#include <unistd.h>
#include <sys/socket.h>

struct tcp_info;

namespace netco
{
	/*
		Socket类，创建的Socket对象默认都是非阻塞的
		职责：
			1、提供fd操作的相关API
			2、管理fd的生命周期
		其中有引用计数，若某一fd没人用了就会close
	*/
	class Socket
	{
	public:
		explicit Socket(int sockfd, std::string ip = "", int port = -1)
			: sockfd_(sockfd), pRef_(new int(1)), port_(port), ip_(std::move(ip))
		{
			if (sockfd > 0)
			{
				setNonBolckSocket();
			}
		}

		Socket()
			: sockfd_(::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP)),
			  pRef_(new int(1)), port_(-1), ip_("")
		{
		}

		Socket(const Socket &otherSock) : sockfd_(otherSock.sockfd_)
		{
			*(otherSock.pRef_) += 1;
			pRef_ = otherSock.pRef_;
			ip_ = otherSock.ip_;
			port_ = otherSock.port_;
		}

		Socket(Socket &&otherSock) : sockfd_(otherSock.sockfd_)
		{
			*(otherSock.pRef_) += 1;
			pRef_ = otherSock.pRef_;
			ip_ = std::move(otherSock.ip_);
			port_ = otherSock.port_;
		}

		Socket &operator=(const Socket &otherSock) = delete;

		~Socket();

		// 返回当前Socket的fd
		int fd() const { return sockfd_; }

		// 返回当前Socket是否可用
		bool isUseful() { return sockfd_ >= 0; }

		// 绑定ip和port到当前Socket
		int bind(int port);

		// 开始监听当前Socket
		int listen();

		// 接收一个连接，返回一个新连接的Socket
		Socket accept();

		// 从socket中读数据
		ssize_t read(void *buf, size_t count);

		// ip示例："127.0.0.1"
		void connect(const char *ip, int port);

		// 往socket中写数据
		ssize_t send(const void *buf, size_t count);

		// 获取当前套接字的目标ip
		std::string ip() { return ip_; }

		// 获取当前套接字的目标port
		int port() { return port_; }

		// 获取套接字的选项,成功则返回true，反之，返回false
		bool getSocketOpt(struct tcp_info *) const;

		// 获取套接字的选项的字符串,成功则返回true，反之，返回false
		bool getSocketOptString(char *buf, int len) const;

		// 获取套接字的选项的字符串
		std::string getSocketOptString() const;

		// 关闭套接字的写操作
		int shutdownWrite();

		// 设置是否开启Nagle算法减少需要传输的数据包，若开启延时可能会增加
		int setTcpNoDelay(bool on);

		// 设置是否地址重用
		int setReuseAddr(bool on);

		// 设置是否端口重用
		int setReusePort(bool on);

		// 设置是否使用心跳检测
		int setKeepAlive(bool on);

		// 设置socket为非阻塞的
		int setNonBolckSocket();

		// 设置socket为阻塞的
		int setBlockSocket();

		// void SetNoSigPipe();

	private:
		// 接收一个连接，返回一个新连接的Socket
		Socket accept_raw();

		// fd
		const int sockfd_;

		// 引用计数
		int *pRef_;

		// 端口号
		int port_;

		// ip
		std::string ip_;
	};

}