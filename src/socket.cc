/***
	@author: Wangzhiming
	@date: 2022-10-29
***/
#include "../include/socket.h"
#include "../include/scheduler.h"

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <stdio.h> // snprintf
#include <fcntl.h>
#include <string.h>
#include <sys/epoll.h>

using namespace cppCo;

Socket::~Socket()
{
	--(*pRef_);
	if (!(*pRef_) && isUseful())
	{
		::close(sockfd_);
		delete pRef_;
	}
}

bool Socket::getSocketOpt(struct tcp_info *tcpi) const
{
	socklen_t len = sizeof(*tcpi);
	memset(tcpi, 0, sizeof(*tcpi));
	return ::getsockopt(sockfd_, SOL_TCP, TCP_INFO, tcpi, &len) == 0;
}

bool Socket::getSocketOptString(char *buf, int len) const
{
	struct tcp_info tcpi;
	bool ok = getSocketOpt(&tcpi);
	if (ok)
	{
		snprintf(buf, len, "unrecovered=%u "
						   "rto=%u ato=%u snd_mss=%u rcv_mss=%u "
						   "lost=%u retrans=%u rtt=%u rttvar=%u "
						   "sshthresh=%u cwnd=%u total_retrans=%u",
				 tcpi.tcpi_retransmits, // Number of unrecovered [RTO] timeouts
				 tcpi.tcpi_rto,			// Retransmit timeout in usec
				 tcpi.tcpi_ato,			// Predicted tick of soft clock in usec
				 tcpi.tcpi_snd_mss,
				 tcpi.tcpi_rcv_mss,
				 tcpi.tcpi_lost,	// Lost packets
				 tcpi.tcpi_retrans, // Retransmitted packets out
				 tcpi.tcpi_rtt,		// Smoothed round trip time in usec
				 tcpi.tcpi_rttvar,	// Medium deviation
				 tcpi.tcpi_snd_ssthresh,
				 tcpi.tcpi_snd_cwnd,
				 tcpi.tcpi_total_retrans); // Total retransmits for entire connection
	}
	return ok;
}

std::string Socket::getSocketOptString() const
{
	char buf[1024];
	buf[0] = '\0';
	getSocketOptString(buf, sizeof buf);
	return std::string(buf);
}

int Socket::bind(int port)
{
	port_ = port;
	struct sockaddr_in serv;
	memset(&serv, 0, sizeof(struct sockaddr_in));
	serv.sin_family = AF_INET;
	serv.sin_port = htons(port);
	serv.sin_addr.s_addr = htonl(INADDR_ANY);
	int ret = ::bind(sockfd_, (struct sockaddr *)&serv, sizeof(serv));
	return ret;
}

int Socket::listen()
{
	int ret = ::listen(sockfd_, parameter::backLog);
	return ret;
}

Socket Socket::accept_raw()
{
	int connfd = -1;
	struct sockaddr_in client;
	socklen_t len = sizeof(client);
	connfd = ::accept(sockfd_, (struct sockaddr *)&client, &len);
	if (connfd < 0)
	{
		return Socket(connfd);
	}

	// accept成功保存用户ip
	struct sockaddr_in *sock = (struct sockaddr_in *)&client;
	int port = ntohs(sock->sin_port); // linux上打印方式
	struct in_addr in = sock->sin_addr;
	char ip[INET_ADDRSTRLEN]; // INET_ADDRSTRLEN这个宏系统默认定义 16
	// 成功的话此时IP地址保存在str字符串中。
	inet_ntop(AF_INET, &in, ip, sizeof(ip));

	return Socket(connfd, std::string(ip), port);
}

Socket Socket::accept()
{
	auto ret(accept_raw());
	if (ret.isUseful())
	{
		return ret;
	}
	cppCo::Scheduler::getScheduler()->getProcessor(threadIdx)->waitEvent(sockfd_, EPOLLIN | EPOLLPRI | EPOLLRDHUP | EPOLLHUP);
	auto con(accept_raw());
	if (con.isUseful())
	{
		return con;
	}
	return accept();
}

// 从socket中读数据
ssize_t Socket::read(void *buf, size_t count)
{
	auto ret = ::read(sockfd_, buf, count);
	if (ret >= 0)
	{
		return ret;
	}
	if (ret == -1 && errno == EINTR) // 被中断信号（只有阻塞状态才会出现）
	{
		return read(buf, count);
	}
	cppCo::Scheduler::getScheduler()->getProcessor(threadIdx)->waitEvent(sockfd_, EPOLLIN | EPOLLPRI | EPOLLRDHUP | EPOLLHUP);
	return ::read(sockfd_, buf, count);
}

void Socket::connect(const char *ip, int port)
{
	struct sockaddr_in addr = {0};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	inet_pton(AF_INET, ip, &addr.sin_addr);
	ip_ = std::string(ip);
	port_ = port;
	auto ret = ::connect(sockfd_, (struct sockaddr *)&addr, sizeof(sockaddr_in));
	if (ret == 0)
	{
		return;
	}
	if (ret == -1 && errno == EINTR)
	{
		return connect(ip, port);
	}
	cppCo::Scheduler::getScheduler()->getProcessor(threadIdx)->waitEvent(sockfd_, EPOLLOUT);
	return connect(ip, port);
}

// 往socket中写数据
ssize_t Socket::send(const void *buf, size_t count)
{
	// 忽略SIGPIPE信号
	size_t sendIdx = ::send(sockfd_, buf, count, MSG_NOSIGNAL);
	if (sendIdx >= count)
	{
		return count;
	}
	cppCo::Scheduler::getScheduler()->getProcessor(threadIdx)->waitEvent(sockfd_, EPOLLOUT);
	return send((char *)buf + sendIdx, count - sendIdx);
}

int Socket::shutdownWrite()
{
	int ret = ::shutdown(sockfd_, SHUT_WR);
	return ret;
}

int Socket::setTcpNoDelay(bool on)
{
	int optval = on ? 1 : 0;
	int ret = ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY,
						   &optval, static_cast<socklen_t>(sizeof optval));
	return ret;
}

int Socket::setReuseAddr(bool on)
{
	int optval = on ? 1 : 0;
	int ret = ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR,
						   &optval, static_cast<socklen_t>(sizeof optval));
	return ret;
}

int Socket::setReusePort(bool on)
{
	int ret = -1;
#ifdef SO_REUSEPORT
	int optval = on ? 1 : 0;
	ret = ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT,
					   &optval, static_cast<socklen_t>(sizeof optval));
#endif
	return ret;
}

int Socket::setKeepAlive(bool on)
{
	int optval = on ? 1 : 0;
	int ret = ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE,
						   &optval, static_cast<socklen_t>(sizeof optval));
	return ret;
}

// 设置socket为非阻塞的
int Socket::setNonBolckSocket()
{
	auto flags = fcntl(sockfd_, F_GETFL, 0);
	int ret = fcntl(sockfd_, F_SETFL, flags | O_NONBLOCK); // 设置成非阻塞模式
	return ret;
}

// 设置socket为阻塞的
int Socket::setBlockSocket()
{
	auto flags = fcntl(sockfd_, F_GETFL, 0);
	int ret = fcntl(sockfd_, F_SETFL, flags & ~O_NONBLOCK); // 设置成阻塞模式；
	return ret;
}