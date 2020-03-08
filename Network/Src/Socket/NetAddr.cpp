//
//  Copyright (c)1998-2012, Public Technology
//  All Rights Reserved.
//
//  Description:
//  $Id: NetAddr.cpp 17 2013-01-22 09:26:19Z  $
//

#include <string.h>

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif
#include "Base/String.h"
#include "Network/Socket/NetAddr.h"
namespace Public
{
namespace Network
{

//一个IP地址和一个端口号构成了一个IPV4地址.
struct NetAddr::NetAddrInternal
{
	std::string ipstr;

	InetType inet;
	SockAddrIPv4 ipv4;
	SockAddrIPv6 ipv6;

	NetAddrInternal()
	{
		reset();
	}

	void reset()
	{
		inet = INET_IPV4;
		ipstr = "";
		memset(&ipv4, 0, sizeof(ipv4));
		memset(&ipv6, 0, sizeof(ipv6));

		ipv4.sin_family = AF_INET;
		ipv6.sin6_family = AF_INET6;
	}
};

/// 缺省构造函数，无效地址
NetAddr::NetAddr(InetType inet)
{
	internal = new NetAddrInternal();
	internal->inet = inet;
}

NetAddr::NetAddr(const NetAddr &addr)
{
	internal = new NetAddrInternal();
	*internal = *addr.internal;
}

/// 构造函数 本地 地址为 INADDR_ANY
/// \param [in] port 端口号
NetAddr::NetAddr(uint16_t port, InetType inet)
{
	internal = new NetAddrInternal();
	internal->inet = inet;

	setPort(port);
}
/// 构造函数
/// \param [in] ip 点分十进制IPv4字符串
/// \param [in] port 端口号
NetAddr::NetAddr(const std::string &addr, int port)
{
	internal = new NetAddrInternal();

	setAddr(addr, port);
}

/// 构造函数
/// \param [in] addr SockAddrIPv4 构造
NetAddr::NetAddr(const SockAddrIPv4 &ipv4)
{
	internal = new NetAddrInternal();

	setAddr(ipv4);
}

NetAddr::NetAddr(const SockAddr &addr)
{
	internal = new NetAddrInternal();

	setAddr(addr);
}

/// 析构函数
NetAddr::~NetAddr()
{
	delete internal;
}

/// 赋值运算
NetAddr &NetAddr::operator=(NetAddr const &other)
{
	*internal = *other.internal;

	return *this;
}

/// 关系运算:等于
bool NetAddr::operator==(const NetAddr &other) const
{
	if (internal->inet != other.internal->inet)
		return false;

	if (internal->inet == INET_IPV4)
	{
		if ((internal->ipv4.sin_port == other.internal->ipv4.sin_port) && (internal->ipv4.sin_addr.s_addr == other.internal->ipv4.sin_addr.s_addr))
			return true;
	}
	else if (internal->inet == INET_IPV6)
	{
		if ((internal->ipv6.sin6_port == other.internal->ipv6.sin6_port) && !memcmp((char *)internal->ipv6.sin6_addr.s6_addr, (char *)other.internal->ipv6.sin6_addr.s6_addr, sizeof(other.internal->ipv6.sin6_addr.s6_addr)))
			return true;
	}

	return false;
}

bool NetAddr::operator<(const NetAddr &other) const
{
	char buffer1[128];
	char buffer2[128];

	snprintf_x(buffer1, 127, "%s-%d", getIP(), getPort());
	snprintf_x(buffer2, 127, "%s-%d", other.getIP(), other.getPort());

	return String::strcasecmp(buffer1, buffer2) < 0;
}

// 当前的网络地址是否为有效的
bool NetAddr::isValid() const
{
	return getPort() != 0 || getIP().length() > 0;
}

InetType NetAddr::getNetType() const
{
	return internal->inet;
}

/// 获得实现类的SockAddr指针
/// \param [in/out] addr 指针
/// \param [in]  addr的长度
/// \retval !=NULL 有效的地址
/// \retval ==NULL 无效地址
SockAddr *NetAddr::getAddr() const
{
	if (internal->inet == INET_IPV6)
	{
		return (SockAddr *)&internal->ipv6;
	}
	return (SockAddr *)&internal->ipv4;
}

SockAddrIPv4 *NetAddr::getAddrV4() const
{
	if (internal->inet == INET_IPV6)
	{
		return NULL;
	}
	return &internal->ipv4;
}

/// 获得实现类的SockAddr地址的长度
/// \retval  >  0 地址长度
/// \reval   == 0 地址无效
uint32_t NetAddr::getAddrLen() const
{
	if (internal->inet == INET_IPV6)
	{
		return sizeof(SockAddrIPv6);
	}
	return sizeof(SockAddrIPv4);
}

std::string NetAddr::getIP() const
{
	if (internal->ipstr.length() == 0)
	{
		if (internal->inet == INET_IPV4 && internal->ipv4.sin_addr.s_addr != 0)
		{
			const char *tmp = inet_ntoa(internal->ipv4.sin_addr);
			if (tmp != NULL)
				internal->ipstr = tmp;
		}
		else if (internal->inet == INET_IPV6)
		{
			assert(0);
		}
	}

	return internal->ipstr;
}

uint16_t NetAddr::getPort() const
{
	if (internal->inet == INET_IPV6)
	{
		return ntohs(internal->ipv6.sin6_port);
	}
	return ntohs(internal->ipv4.sin_port);
}

/// 设置IP地址
/// \param [in] ip 点分十进制IPv4字符串
/// \param [in] port 端口号
/// \reval  >=0 成功
/// \reval  < 0 失败
int NetAddr::setAddr(const SockAddrIPv4 &addr)
{
	internal->ipv4 = addr;

	internal->inet = INET_IPV4;

	return 0;
}
/// 设置IP地址
/// \param [in] addr SockAddrIPv4
/// \reval  >=0 成功
/// \reval  < 0 失败
int NetAddr::setAddr(const SockAddr &addr)
{
	return setAddr(*(SockAddrIPv4 *)&addr);
}

/// 只设置IP地址,不改变端口
/// \param [in] ip 点分十进制IPv4字符串
/// \reval  >=0 成功
/// \reval  < 0 失败
int NetAddr::setAddr(const std::string &ip, uint16_t port)
{
	setPort(port);

	internal->ipstr = ip;

	if (ip == "")
	{
		//	internal->addrin.sin_addr.s_addr = (uint32_t)(-1);
	}
	else if (inet_addr(ip.c_str()) != INADDR_NONE)
	{
		internal->ipv4.sin_addr.s_addr = inet_addr(ip.c_str());
		internal->ipv4.sin_family = AF_INET;
		internal->inet = INET_IPV4;
	}
	else if (inet_addr(ip.c_str()) == INADDR_NONE)
	{
		struct hostent *hent = NULL;
		if ((hent = gethostbyname(ip.c_str())) == NULL)
			return -1;
		::memcpy(&internal->ipv4.sin_addr.s_addr, *(hent->h_addr_list), sizeof(internal->ipv4.sin_addr.s_addr));
		internal->ipv4.sin_family = AF_INET;
		internal->inet = INET_IPV4;
	}
	else
	{
		internal->inet = INET_IPV6;
		internal->ipv6.sin6_family = AF_INET6;
#ifdef WIN32
		struct addrinfo hints;
		struct addrinfo *result = NULL;
		memset(&hints, 0, sizeof(hints));
		hints.ai_flags = AI_CANONNAME;
		hints.ai_family = AF_INET6;
		hints.ai_socktype = SOCK_RAW;
		int ret = getaddrinfo(ip.c_str(), NULL, &hints, &result);
		if (ret != 0)
		{
			struct hostent *hent = NULL;
			if (inet_addr(ip.c_str()) != INADDR_NONE)
			{ //若能转换成ipv4地址，则按ipv4 mapped地址构造IPv6地址
				*((int *)&(internal->ipv6.sin6_addr.s6_addr[12])) = inet_addr(ip.c_str());
				internal->ipv6.sin6_addr.s6_addr[10] = 0xff;
				internal->ipv6.sin6_addr.s6_addr[11] = 0xff;
				return 0;
			}
			else if ((hent = gethostbyname(ip.c_str())) != NULL)
			{
				*((int *)&(internal->ipv6.sin6_addr.s6_addr[12])) = *((int *)(*(hent->h_addr_list)));
				internal->ipv6.sin6_addr.s6_addr[10] = 0xff;
				internal->ipv6.sin6_addr.s6_addr[11] = 0xff;
				return 0;
			}
			return -1;
		}
		for (struct addrinfo *tmp = result; tmp != NULL; tmp = tmp->ai_next)
		{
			if (tmp->ai_family == AF_INET6 && tmp->ai_addr != NULL)
			{
				memcpy(&internal->ipv6, tmp->ai_addr, sizeof(internal->ipv6));
				break;
			}
		}
		freeaddrinfo(result);
#else
		if (inet_pton(AF_INET6, ip.c_str(), internal->ipv6.sin6_addr.s6_addr) <= 0)
		{
			struct in_addr ia;
			if (inet_aton(ip.c_str(), &ia) != 0)
			{
				internal->ipv6.sin6_addr.s6_addr[10] = 0xff;
				internal->ipv6.sin6_addr.s6_addr[11] = 0xff;
				memcpy(internal->ipv6.sin6_addr.s6_addr + 12, (char *)&ia.s_addr, sizeof(ia));
				internal->ipv6.sin6_family = AF_INET6;
			}
			else
				return -1;
		}
#endif
	}

	return 0;
}

/// 只设置端口
/// \param [in] port 端口
/// \reval  >=0 成功
/// \reval  < 0 失败
int NetAddr::setPort(uint16_t port)
{
	internal->ipv4.sin_port = htons(port);
	internal->ipv4.sin_family = AF_INET;

	internal->ipv6.sin6_family = AF_INET6;
	internal->ipv6.sin6_port = htons(port);

	return 0;
}

NetAddr::NetAddr(const SockAddrIPv6 &ipv6)
{
	internal = new NetAddrInternal();

	internal->ipv6 = ipv6;
	internal->inet = INET_IPV6;
}
void NetAddr::reset()
{
	InetType inet = internal->inet;

	internal->reset();

	internal->inet = inet;
}
SockAddrIPv6 *NetAddr::getAddrV6() const
{
	if (internal->inet == INET_IPV4)
	{
		return NULL;
	}
	return (SockAddrIPv6 *)&internal->ipv6;
}
int NetAddr::setAddr(const SockAddrIPv6 &addr)
{
	internal->ipv6 = addr;

	internal->inet = INET_IPV6;

	return 0;
}
NetAddr &NetAddr::operator=(SockAddrIPv6 const &other)
{
	setAddr(other);

	return *this;
}
} // namespace Network
} // namespace Public
