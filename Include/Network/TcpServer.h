//
//  Copyright (c)1998-2012,  Public Technology
//  All Rights Reserved.
//
//	Description:
//	$Id: tcpserver.h 216 2015-12-15 11:33:55Z  $
//
#ifndef __NETWORK_TCPSERVER_H__
#define __NETWORK_TCPSERVER_H__

#include "Network/Socket.h"

namespace Public{
namespace Network{

class NETWORK_API TCPServer:public Socket
{
public:
	static shared_ptr<Socket> create(const shared_ptr<IOWorker>& worker,const NetAddr& addr = NetAddr());

	//TCPServer支持以下函数
	//virtual bool bind(const NetAddr& addr, bool reusedAddr = true);
	//virtual bool disconnect();
	//virtual bool getSocketTimeout(uint32_t& recvTimeout,uint32_t& sendTimeout) const;
	//virtual bool setSocketTimeout(uint32_t recvTimeout,uint32_t sendTimeout);
	//virtual bool nonBlocking(bool nonblock);
	//virtual bool async_accept(const AcceptedCallback& callback);
	//virtual shared_ptr<Socket> accept();
	//virtual SOCKET getHandle() const ;
	//virtual NetStatus getStatus() const;
	//virtual NetType getNetType() const;
	//virtual NetAddr getMyAddr() const;
	//virtual bool setSocketOpt(int level, int optname, const void *optval, int optlen);
	//virtual bool getSocketOpt(int level, int optname, void *optval, int *optlen) const;
};


};
};




#endif //__NETWORK_TCPSERVER_H__

