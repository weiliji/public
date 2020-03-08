//
//  Copyright (c)1998-2012,  Public Technology
//  All Rights Reserved.
//
//	Description:
//	$Id: Socket.h 216 2015-12-15 11:33:55Z  $
//

#ifndef __NETWORK_SSLSOCKET_H__
#define __NETWORK_SSLSOCKET_H__

#include "Socket.h"


namespace Public{
namespace Network{


//SSLSocket 只支持TCP连接成功的连接
//-------------------------------------以下函数为ssl不支持的接口
//virtual bool bind(const NetAddr& addr, bool reusedAddr = true) { return false; }
//virtual bool async_accept(const Socket::AcceptedCallback& callback) { return false; }
//virtual shared_ptr<Socket> accept() { return shared_ptr<Socket>(); }
//virtual bool async_connect(const NetAddr& addr, const Socket::ConnectedCallback& callback) { return false; }
//virtual bool connect(const NetAddr& addr) { return false; }
//virtual bool async_recvfrom(char* buf, uint32_t len, const Socket::RecvFromCallback& received) { return false; }
//virtual bool async_recvfrom(const shared_ptr<Socket::RecvBuffer>& buffer, const Socket::RecvFromCallback1& received) { return false; }
//virtual bool async_recvfrom(const Socket::RecvFromCallback& received, int maxlen = 1024) { return false; }
//virtual int recvfrom(char* buf, uint32_t len, NetAddr& other) { return 0; }
//virtual bool async_sendto(const char* buf, uint32_t len, const NetAddr& other, const Socket::SendedCallback& sended) { return false; }
//virtual bool async_sendto(const shared_ptr<SendBuffer>& buffer, const NetAddr& other, const SendedCallback1& sended) { return false; }
//virtual bool async_sendto(const std::vector<SBuf>& sendbuf, const NetAddr& other, const SendedCallback2& sended) { return false; }
//virtual bool async_sendto(const std::vector<shared_ptr<SendBuffer>>& sendbuf, const NetAddr& other, const SendedCallback3& sended) { return false; }
//virtual int sendto(const char* buf, uint32_t len, const NetAddr& other) { return 0; }
//virtual void socketReady() {}
//virtual void socketError(const std::string& errmsg) {}

class NETWORK_API SSLSocket
{
public:
	//服务端,socket must tcp by accept  and connected
	static shared_ptr<Socket> create(const shared_ptr<Socket>& sock,const std::string& cert,const std::string& key,uint32_t timeout = 5000);
	//客户端，socket must tcp by connected
	static shared_ptr<Socket> create(const shared_ptr<Socket>& sock, uint32_t timeout = 5000);
};


};
};




#endif //__NETWORK_SOCKET_H__

