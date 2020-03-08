#ifndef __NETWORK_UDP_H__
#define __NETWORK_UDP_H__

//
//  Copyright (c)1998-2012,  Public Technology
//  All Rights Reserved.
//
//	Description:
//	$Id: udp.h 216 2015-12-15 11:33:55Z  $
//
#include "Socket.h"

using namespace std;
namespace Public{
namespace Network{
	
class NETWORK_API UDP:public Socket
{
public:	
	static shared_ptr<Socket> create(const shared_ptr<IOWorker>& worker, InetType inet = INET_IPV4);

	//UDP模式下支持以下函数
	//virtual bool disconnect();
	//virtual bool bind(const NetAddr& addr,bool reusedAddr = true);
	//virtual bool getSocketBuffer(uint32_t& recvSize,uint32_t& sendSize) const;
	//virtual bool setSocketBuffer(uint32_t recvSize,uint32_t sendSize);
	//virtual bool getSocketTimeout(uint32_t& recvTimeout,uint32_t& sendTimeout) const;
	//virtual bool setSocketTimeout(uint32_t recvTimeout,uint32_t sendTimeout);
	//virtual bool nonBlocking(bool nonblock);
	//virtual bool async_recvfrom(char *buf , uint32_t len,const RecvFromCallback& received);
	//virtual bool async_recvfrom(const RecvFromCallback& received, int maxlen = 1024);
	//virtual int recvfrom(char *buf , uint32_t len ,NetAddr& other);
	//virtual bool async_sendto(const char * buf, uint32_t len,const NetAddr& other,const SendedCallback& sended);
	//virtual int sendto(const char * buf, uint32_t len,const NetAddr& other);
	//virtual SOCKET getHandle() const ;
	//virtual NetType getNetType() const;
	//virtual NetAddr getMyAddr() const;
	//virtual bool setSocketOpt(int level, int optname, const void *optval, int optlen);
	//virtual bool getSocketOpt(int level, int optname, void *optval, int *optlen) const;
};


};
};




#endif //__NETWORK_SOCKET_H__
