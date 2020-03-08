//
//  Copyright (c)1998-2012,  Public Technology
//  All Rights Reserved.
//
//	Description:
//	$Id: tcpclient.h 216 2015-12-15 11:33:55Z  $
//
#ifndef __NETWORK_RAWSOCKET_H__
#define __NETWORK_RAWSOCKET_H__

#include "Socket.h"

namespace Public
{
namespace Network
{

#define EthernetIIHEADERLEN 14

struct EthernetIIFrame
{
	union {
		struct
		{
			char destHW[6];
			char srcHW[6];
			uint16_t type;
		};
		char buffer[EthernetIIHEADERLEN] = {
			0,
		};
	} header;

	String data;
};

class NETWORK_API RawSocket : public Socket
{
public:
	static shared_ptr<Socket> create(const shared_ptr<IOWorker> &worker, InetType inet = INET_IPV4);

	//TCPClient支持以下接口
	//virtual bool disconnect();
	//virtual bool bind(const NetAddr& addr,bool reusedAddr = true);
	//virtual bool async_recv(char *buf , uint32_t len,const ReceivedCallback& received);
	//virtual bool async_recv(const ReceivedCallback& received, int maxlen = 1024);
	//virtual int recv(char *buf , uint32_t len);
	//virtual bool async_send(const char * buf, uint32_t len,const SendedCallback& sended);
	//virtual bool async_send(const std::deque<SBuf>& sendbuf, const SendedCallback& sended);
	//virtual int send(const char * buf, uint32_t len);
	//virtual SOCKET getHandle() const;
	//virtual NetType getNetType() const;
	//virtual bool setSocketOpt(int level, int optname, const void *optval, int optlen);
	//virtual bool getSocketOpt(int level, int optname, void *optval, int *optlen) const;
};

}; // namespace Network
}; // namespace Public

#endif //__NETWORK_SOCKET_H__
