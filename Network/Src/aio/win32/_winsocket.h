#pragma  once
#include "_winevent.h"
#include "../asocket.h"

#ifdef WIN32

class _SystemSocket :public ASocket
{
	SendEvent		sendevent;
	RecvEvent		recvevent;
	AcceptEvent		acceptevent;
	ConnectEvent	connectevent;

public:
	_SystemSocket(const shared_ptr<IOWorker>& _ioworker, const shared_ptr<IOServer>& _ioserver,  NetType _type)
	:ASocket(_ioworker,_ioserver,_type){}

	_SystemSocket(const shared_ptr<IOWorker>& _ioworker, const shared_ptr<IOServer>& _ioserver, const TCPClient::NewSocketInfo& newsock)
	:ASocket(_ioworker,_ioserver ,newsock)
	{
		int flag = 1;
		setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char*)&flag, sizeof(flag));
	}

	~_SystemSocket() 
	{
		disconnect();
	}

	virtual bool creatSocket(NetType type)
	{
		int sockettype = type == NetType_Udp ? SOCK_DGRAM : SOCK_STREAM;
		int protocol = type == NetType_Udp ? IPPROTO_UDP : IPPROTO_TCP;

		sock = WSASocket(AF_INET, sockettype, protocol, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (sock <= 0)
		{
			return false;
		}

		nonBlocking(true);

		if (type == NetType_TcpClient)
		{
			int flag = 1;
			setsockopt(sock,IPPROTO_TCP, TCP_NODELAY, (const char*)&flag, sizeof(flag));
		}

		return true;
	}

	virtual bool disconnect()
	{
		ASocket::disconnect();

		if(sock > 0)
			closesocket(sock);

		sock = -1;

		return true;
	}
	virtual bool async_accept(const AcceptedCallback& accepted)
	{
		shared_ptr< _PoolResource> res = resourece;
		if (!accepted || sock <= 0 || res == NULL)
		{
			return false;
		}

		if (type == NetType_TcpServer && !ishavelisten)
		{
			int ret = listen(sock, SOMAXCONN);
			if (ret == SOCKET_ERROR)
			{
				return false;
			}

			ishavelisten = true;
		}

		//AcceptEvent(const shared_ptr<IOWorker>& _ioworker,const shared_ptr<Socket>& sock, LPFN_ACCEPTEX	acceptExFunc, LPFN_GETACCEPTEXSOCKADDRS _getAcceptAddExFunc,const Socket::AcceptedCallback& _acceptcallback) :WinEvent(sock)
		acceptevent.init(accepted);
		if(!res->postEvent(&acceptevent))
		{
			return false;
		}
		return true;
		
		
		return false; }
	virtual bool async_connect(const NetAddr& addr, const ConnectedCallback& connected)
	{
		shared_ptr< _PoolResource> res = resourece;
		if (sock == -1 || type == NetType_TcpConnection || status == NetStatus_connected || addr.getPort() == 0 || !connected || res == NULL)
		{
			return false;
		}

		if (myaddr.getPort() == 0)
		{
			bind(NetAddr(Host::getAvailablePort(50000 + Time::getCurrentMilliSecond() % 10000, Host::SocketType_TCP)));
		}

		otheraddr = addr;

		//ConnectEvent(const NetAddr& toaddr , LPFN_CONNECTEX _connectExFunc,const Socket::ConnectedCallback& _connectcallback)
		connectevent.init(addr, connected);

		if (!res->postEvent(&connectevent))
		{
			return false;
		}

		return true;
	}
	virtual bool async_recv(char *buf, uint32_t len, const ReceivedCallback& received) 
	{
		shared_ptr< _PoolResource> res = resourece;
		if (sock == -1 || status != NetStatus_connected || buf == NULL || len <= 0 || !received || res == NULL)
		{
			return false;
		}

		//RecvEvent(char* buffer, uint32_t len, const Socket::ReceivedCallback& _recvcallback, const Socket::RecvFromCallback& _recvfromcallback)
		recvevent.init(buf, len, received, Socket::RecvFromCallback());

		if (!res->postEvent(&recvevent))
		{
			return false;
		}

		return true;
	}
	virtual bool async_recv(const ReceivedCallback& received, int maxlen = 1024) 
	{
		shared_ptr< _PoolResource> res = resourece;
		if (sock == -1 || status != NetStatus_connected || maxlen <= 0 || !received || res == NULL)
		{
			return false;
		}

		//(char* buffer, uint32_t len, const Socket::ReceivedCallback& _recvcallback, const Socket::RecvFromCallback& _recvfromcallback)
		recvevent.init((char*)NULL, maxlen, received, Socket::RecvFromCallback());

		if (!res->postEvent(&recvevent))
		{
			return false;
		}

		return true;
	}
	virtual bool async_send(const std::deque<SBuf>& sendbuf, const SendedCallback& sended)
	{
		shared_ptr< _PoolResource> res = resourece;
		if (sock == -1 || status != NetStatus_connected || sendbuf.size() <= 0 || !sended || res == NULL)
		{
			return false;
		}

		//SendEvent(const char* buffer, uint32_t len, const Socket::SendedCallback& _callback, const NetAddr& toaddr)
		sendevent.init(sendbuf, sended, NetAddr());

		if (!res->postEvent( &sendevent))
		{
			return false;
		}

		return true;
	}
	virtual bool async_recvfrom(char *buf, uint32_t len, const RecvFromCallback& received) 
	{
		shared_ptr< _PoolResource> res = resourece;
		if (sock == -1 ||  buf == NULL || len <= 0 || !received || res == NULL)
		{
			return false;
		}

		//RecvEvent(char* buffer, uint32_t len, const Socket::ReceivedCallback& _recvcallback, const Socket::RecvFromCallback& _recvfromcallback)
		recvevent.init(buf, len, Socket::ReceivedCallback(), received);

		if (!res->postEvent(&recvevent))
		{
			return false;
		}

		return true;
	}
	virtual bool async_recvfrom(const RecvFromCallback& received, int maxlen = 1024) 
	{
		shared_ptr< _PoolResource> res = resourece;
		if (sock == -1 || maxlen <= 0 || !received || res == NULL)
		{
			return false;
		}

		//RecvEvent(char* buffer, uint32_t len, const Socket::ReceivedCallback& _recvcallback, const Socket::RecvFromCallback& _recvfromcallback)
		recvevent.init((char*)NULL, maxlen, Socket::ReceivedCallback(), received);
		
		if (!res->postEvent(&recvevent))
		{
			return false;
		}

		return true;
	}
	virtual bool async_sendto(const std::deque<SBuf>& sendbuf, const NetAddr& other, const SendedCallback& sended)
	{
		shared_ptr< _PoolResource> res = resourece;
		if (sock == -1 || sendbuf.size() <= 0 || !sended ||res == NULL)
		{
			return false;
		}

		//SendEvent(const char* buffer, uint32_t len, const Socket::SendedCallback& _callback, const NetAddr& toaddr)
		sendevent.init(sendbuf, sended, other);
		
		if (!res->postEvent(&sendevent))
		{
			return false;
		}

		return true;
	}	
	virtual bool nonBlocking(bool nonblock)
	{
		if (sock <= 0) return false;

		unsigned long on_windows = nonblock ? 1 : 0;
		
		return ioctlsocket(sock, FIONBIO, &on_windows) == 0;
	}
};

#endif