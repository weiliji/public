//
//  Copyright (c)1998-2012,  Public Technology
//  All Rights Reserved.
//
//	Description:
//	$Id: tcpclient.h 216 2015-12-15 11:33:55Z  $
//
#ifndef __NETWORK_TCPCLIENT_H__
#define __NETWORK_TCPCLIENT_H__

#include "Network/Socket.h"

namespace Public{
namespace Network{


class NETWORK_API TCPClient:public Socket
{
public:
	struct NewSocketInfo;
public:
	static shared_ptr<Socket> create(const shared_ptr<IOWorker>& worker);
	static shared_ptr<Socket> create(const shared_ptr<IOWorker>& worker, const NewSocketInfo& newsockinfo);
	
	//TCPClient支持以下接口
	//virtual bool disconnect();
	//virtual bool bind(const NetAddr& addr,bool reusedAddr = true);
	//virtual bool getSocketBuffer(uint32_t& recvSize,uint32_t& sendSize) const;
	//virtual bool setSocketBuffer(uint32_t recvSize,uint32_t sendSize);
	//virtual bool getSocketTimeout(uint32_t& recvTimeout,uint32_t& sendTimeout) const;
	//virtual bool setSocketTimeout(uint32_t recvTimeout,uint32_t sendTimeout);
	//virtual bool nonBlocking(bool nonblock);
	//virtual bool async_connect(const NetAddr& addr,const ConnectedCallback& callback);
	//virtual bool connect(const NetAddr& addr);
	//virtual bool setDisconnectCallback(const DisconnectedCallback& disconnected);
	//virtual bool async_recv(char *buf , uint32_t len,const ReceivedCallback& received);
	//virtual bool async_recv(const ReceivedCallback& received, int maxlen = 1024);
	//virtual int recv(char *buf , uint32_t len);
	//virtual bool async_send(const char * buf, uint32_t len,const SendedCallback& sended);
	//virtual bool async_send(const std::deque<SBuf>& sendbuf, const SendedCallback& sended);
	//virtual int send(const char * buf, uint32_t len);
	//virtual SOCKET getHandle() const;
	//virtual NetStatus getStatus() const;
	//virtual NetType getNetType() const;
	//virtual NetAddr getMyAddr() const;
	//virtual NetAddr getOtherAddr() const;
	//virtual bool setSocketOpt(int level, int optname, const void *optval, int optlen);
	//virtual bool getSocketOpt(int level, int optname, void *optval, int *optlen) const;
};


};
};




#endif //__NETWORK_SOCKET_H__

