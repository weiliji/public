#include "_poll.h"
#include "asocket.h"

#ifdef SUPPORT_IOCP
#include "_proactor_event.h"

#define SIO_RCVALL _WSAIOW(IOC_VENDOR,1) 

class _Proactor_Socket :public ASocket
{
	struct SyncConnectObject
	{
		Semaphore waitpos;

		void connectCallback(const weak_ptr<Socket>& /*connectSock*/, bool, const std::string&)
		{
			waitpos.post();
		}
	};
public:
	_Proactor_Socket(const shared_ptr<IOWorker>& _ioworker, const shared_ptr<_Poll>& _poll, InetType _inet, NetType _type, SOCKET _sock, const NetAddr& _otheraddr)
	:ASocket(_ioworker,_poll,_type, _inet,_sock,_otheraddr){}

	~_Proactor_Socket()
	{
		disconnect();
	}

	virtual bool creatSocket(NetType type)
	{
		int inet = _famlily == INET_IPV4 ? AF_INET : AF_INET6;
		int sockettype = SOCK_DGRAM;
		int protocol = IPPROTO_UDP;
		int wsaflag = WSA_FLAG_OVERLAPPED;

		if (type == NetType_Udp)
		{
			sockettype = SOCK_DGRAM;
			protocol = IPPROTO_UDP;
		}
		else if (type == NetType_TcpServer || type == NetType_TcpClient || type == NetType_TcpConnection)
		{
			sockettype = SOCK_STREAM;
			protocol = IPPROTO_TCP;
		}
		else if (type == NetType_Raw)
		{
		//	inet = AF_PACKET;
			sockettype = SOCK_RAW;
			protocol = IPPROTO_RAW;
		//	wsaflag = 0;
		}
		sock = WSASocket(inet, sockettype, protocol, NULL, 0, wsaflag);
		if (sock == INVALID_SOCKET)
		{
			return false;
		}

		{
			unsigned long on_windows = 1;

			return ioctlsocket(sock, FIONBIO, &on_windows) == 0;
		}

		if (type == NetType_Raw)
		{
			int flag = 1;
			setsockopt(sock, IPPROTO_IP, IP_HDRINCL, (const char*)&flag, sizeof(flag));

			//if (ioctlsocket(sock, SIO_RCVALL, (u_long*)&flag) != 0)
			//{
			//	assert(0);
			//}
		}

		return true;
	}

	virtual bool disconnect()
	{
		ASocket::disconnect();

		if (sock != INVALID_SOCKET)
		{
			shutdown(sock, 2);
			closesocket(sock);
		}

		sock = INVALID_SOCKET;	

		return true;
	}
	virtual bool async_accept(const AcceptedCallback& accepted, uint32_t timeoutms)
	{
		shared_ptr< _PollResource> res = resourece;
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
		WinIOCP_OVERLAPPED* overlapped = new WinIOCP_OVERLAPPED();
		overlapped->resource = res;
		shared_ptr<Proactor_AcceptEvent> event = make_shared<Proactor_AcceptEvent>(overlapped);
		overlapped->event = event;
		event->init(accepted);
		if(!res->postEvent(event,timeoutms))
		{
			return false;
		}
		return true;
		
		
		return false; }
	virtual bool async_connect(const NetAddr& addr, const ConnectedCallback& connected, uint32_t timeoutms)
	{
		shared_ptr< _PollResource> res = resourece;
		if (sock == -1 || type == NetType_TcpConnection || status == NetStatus_connected || addr.getPort() == 0 || !connected || res == NULL)
		{
			return false;
		}

		if (myaddr.getPort() == 0)
		{
			uint32_t port = Host::getAvailablePort(20000 + Time::getCurrentMilliSecond() % 10000 + ((size_t)this)%1000);

			bind(NetAddr(port));
		}

		otheraddr = addr;
		WinIOCP_OVERLAPPED* overlapped = new WinIOCP_OVERLAPPED();
		overlapped->resource = res;
		shared_ptr<Proactor_ConnectEvent> event = make_shared<Proactor_ConnectEvent>(overlapped);
		overlapped->event = event;
		//ConnectEvent(const NetAddr& toaddr , LPFN_CONNECTEX _connectExFunc,const Socket::ConnectedCallback& _connectcallback)
		event->init(addr, connected);

		if (!res->postEvent(event, timeoutms))
		{
			return false;
		}

		return true;
	}
	virtual bool async_recv(char *buf, uint32_t len, const ReceivedCallback& received, uint32_t timeoutms)
	{
		shared_ptr< _PollResource> res = resourece;
		if (sock == -1 || status != NetStatus_connected || buf == NULL || len <= 0 || !received || res == NULL)
		{
			return false;
		}

		WinIOCP_OVERLAPPED* overlapped = new WinIOCP_OVERLAPPED();
		overlapped->resource = res;
		shared_ptr<Proactor_RecvEvent> event = make_shared<Proactor_RecvEvent>(overlapped);
		overlapped->event = event;
		//RecvEvent(char* buffer, uint32_t len, const Socket::ReceivedCallback& _recvcallback, const Socket::RecvFromCallback& _recvfromcallback)
		event->init(buf, len, received, Socket::RecvFromCallback());

		if (!res->postEvent(event,timeoutms))
		{
			return false;
		}

		return true;
	}
	virtual bool async_recv(const shared_ptr<RecvBuffer>& buffer, const ReceivedCallback1& received, uint32_t timeoutms)
	{
		shared_ptr< _PollResource> res = resourece;
		if (sock == -1 || status != NetStatus_connected || buffer == NULL || buffer->bufferSize() <= 0 || !received || res == NULL)
		{
			return false;
		}

		WinIOCP_OVERLAPPED* overlapped = new WinIOCP_OVERLAPPED();
		overlapped->resource = res;
		shared_ptr<Proactor_RecvEvent> event = make_shared<Proactor_RecvEvent>(overlapped);
		overlapped->event = event;
		//(char* buffer, uint32_t len, const Socket::ReceivedCallback& _recvcallback, const Socket::RecvFromCallback& _recvfromcallback)
		event->init(buffer, received, Socket::RecvFromCallback1());

		if (!res->postEvent(event,timeoutms))
		{
			return false;
		}

		return true;
	}
	virtual bool async_recv(const ReceivedCallback& received, int maxlen, uint32_t timeoutms)
	{
		shared_ptr< _PollResource> res = resourece;
		if (sock == -1 || status != NetStatus_connected || maxlen <= 0 || !received || res == NULL)
		{
			return false;
		}

		WinIOCP_OVERLAPPED* overlapped = new WinIOCP_OVERLAPPED();
		overlapped->resource = res;
		shared_ptr<Proactor_RecvEvent> event = make_shared<Proactor_RecvEvent>(overlapped);
		overlapped->event = event;
		//(char* buffer, uint32_t len, const Socket::ReceivedCallback& _recvcallback, const Socket::RecvFromCallback& _recvfromcallback)
		event->init((char*)NULL, maxlen, received, Socket::RecvFromCallback());

		if (!res->postEvent(event,timeoutms))
		{
			return false;
		}

		return true;
	}	
	virtual bool async_send(const char* buf, uint32_t len, const SendedCallback& sended, uint32_t timeoutms)
	{
		shared_ptr< _PollResource> res = resourece;
		if (buf == NULL || len <= 0 || !sended || status != NetStatus_connected || !sended || res == NULL)
		{
			return false;
		}

		WinIOCP_OVERLAPPED* overlapped = new WinIOCP_OVERLAPPED();
		overlapped->resource = res;
		shared_ptr<Proactor_SendEvent> event = make_shared<Proactor_SendEvent>(overlapped);
		overlapped->event = event;
		//SendEvent(const char* buffer, uint32_t len, const Socket::SendedCallback& _callback, const NetAddr& toaddr)
		event->init(buf, len,sended, NetAddr());

		if (!res->postEvent(event,timeoutms))
		{
			return false;
		}

		return true;
	}
	virtual bool async_send(const shared_ptr<SendBuffer>& buffer, const SendedCallback1& sended, uint32_t timeoutms)
	{
		shared_ptr< _PollResource> res = resourece;
		if (sock == -1 || status != NetStatus_connected || buffer == NULL || buffer->bufferlen() <= 0 || res == NULL)
		{
			return false;
		}

		WinIOCP_OVERLAPPED* overlapped = new WinIOCP_OVERLAPPED();
		overlapped->resource = res;
		shared_ptr<Proactor_SendEvent> event = make_shared<Proactor_SendEvent>(overlapped);
		overlapped->event = event;
		//SendEvent(const char* buffer, uint32_t len, const Socket::SendedCallback& _callback, const NetAddr& toaddr)
		event->init(buffer, sended, NetAddr());

		if (!res->postEvent(event,timeoutms))
		{
			return false;
		}

		return true;
	}
	virtual bool async_send(const std::vector<SBuf>& sendbuf, const SendedCallback2& sended, uint32_t timeoutms)
	{
		shared_ptr< _PollResource> res = resourece;
		if (sock == -1 || status != NetStatus_connected || sendbuf.size() <= 0 || !sended || res == NULL)
		{
			return false;
		}

		WinIOCP_OVERLAPPED* overlapped = new WinIOCP_OVERLAPPED();
		overlapped->resource = res;
		shared_ptr<Proactor_SendEvent> event = make_shared<Proactor_SendEvent>(overlapped);
		overlapped->event = event;
		//SendEvent(const char* buffer, uint32_t len, const Socket::SendedCallback& _callback, const NetAddr& toaddr)
		event->init(sendbuf, sended, NetAddr());

		if (!res->postEvent(event,timeoutms))
		{
			return false;
		}

		return true;
	}
	virtual bool async_send(const std::vector<shared_ptr<SendBuffer>>& sendbuf, const SendedCallback3& sended, uint32_t timeoutms)
	{
		shared_ptr< _PollResource> res = resourece;
		if (sock == -1 || status != NetStatus_connected || sendbuf.size() <= 0 || res == NULL)
		{
			return false;
		}

		WinIOCP_OVERLAPPED* overlapped = new WinIOCP_OVERLAPPED();
		overlapped->resource = res;
		shared_ptr<Proactor_SendEvent> event = make_shared<Proactor_SendEvent>(overlapped);
		overlapped->event = event;
		//SendEvent(const char* buffer, uint32_t len, const Socket::SendedCallback& _callback, const NetAddr& toaddr)
		event->init(sendbuf, sended, NetAddr());

		if (!res->postEvent(event,timeoutms))
		{
			return false;
		}

		return true;
	}
	virtual bool async_recvfrom(char *buf, uint32_t len, const RecvFromCallback& received, uint32_t timeoutms)
	{
		shared_ptr< _PollResource> res = resourece;
		if (sock == -1 ||  buf == NULL || len <= 0 || !received || res == NULL)
		{
			return false;
		}
		WinIOCP_OVERLAPPED* overlapped = new WinIOCP_OVERLAPPED();
		overlapped->resource = res;
		shared_ptr<Proactor_RecvEvent> event = make_shared<Proactor_RecvEvent>(overlapped);
		overlapped->event = event;
		event->init(buf, len, Socket::ReceivedCallback(), received);

		if (!res->postEvent(event,timeoutms))
		{
			return false;
		}

		return true;
	}
	virtual bool async_recvfrom(const shared_ptr<RecvBuffer>& buffer, const RecvFromCallback1& received, uint32_t timeoutms)
	{
		shared_ptr< _PollResource> res = resourece;
		if (sock == -1 || buffer == NULL || buffer->bufferSize() <= 0 || !received || res == NULL)
		{
			return false;
		}
		WinIOCP_OVERLAPPED* overlapped = new WinIOCP_OVERLAPPED();
		overlapped->resource = res;
		shared_ptr<Proactor_RecvEvent> event = make_shared<Proactor_RecvEvent>(overlapped);
		overlapped->event = event;
		event->init(buffer, Socket::ReceivedCallback1(), received);

		if (!res->postEvent(event,timeoutms))
		{
			return false;
		}

		return true;
	}
	virtual bool async_recvfrom(const RecvFromCallback& received, int maxlen, uint32_t timeoutms)
	{
		shared_ptr< _PollResource> res = resourece;
		if (sock == -1 || maxlen <= 0 || !received || res == NULL)
		{
			return false;
		}

		WinIOCP_OVERLAPPED* overlapped = new WinIOCP_OVERLAPPED();
		overlapped->resource = res;
		shared_ptr<Proactor_RecvEvent> event = make_shared<Proactor_RecvEvent>(overlapped);
		overlapped->event = event;
		//RecvEvent(char* buffer, uint32_t len, const Socket::ReceivedCallback& _recvcallback, const Socket::RecvFromCallback& _recvfromcallback)
		event->init((char*)NULL, maxlen, Socket::ReceivedCallback(), received);
		
		if (!res->postEvent(event,timeoutms))
		{
			return false;
		}

		return true;
	}	
	virtual bool async_sendto(const char* buf, uint32_t len, const NetAddr& other, const SendedCallback& sended, uint32_t timeoutms)
	{
		shared_ptr< _PollResource> res = resourece;
		if (buf == NULL || len <= 0 || !sended || res == NULL)
		{
			return false;
		}

		WinIOCP_OVERLAPPED* overlapped = new WinIOCP_OVERLAPPED();
		overlapped->resource = res;
		shared_ptr<Proactor_SendEvent> event = make_shared<Proactor_SendEvent>(overlapped);
		overlapped->event = event;
		//SendEvent(const char* buffer, uint32_t len, const Socket::SendedCallback& _callback, const NetAddr& toaddr)
		event->init(buf,len, sended, other);

		if (!res->postEvent(event,timeoutms))
		{
			return false;
		}

		return true;
	}
	virtual bool async_sendto(const shared_ptr<SendBuffer>& buffer, const NetAddr& other, const SendedCallback1& sended, uint32_t timeoutms)
	{
		shared_ptr< _PollResource> res = resourece;
		if (sock == -1 || buffer == NULL || buffer->bufferlen() <= 0 || res == NULL)
		{
			return false;
		}

		WinIOCP_OVERLAPPED* overlapped = new WinIOCP_OVERLAPPED();
		overlapped->resource = res;
		shared_ptr<Proactor_SendEvent> event = make_shared<Proactor_SendEvent>(overlapped);
		overlapped->event = event;
		//SendEvent(const char* buffer, uint32_t len, const Socket::SendedCallback& _callback, const NetAddr& toaddr)
		event->init(buffer, sended, other);

		if (!res->postEvent(event,timeoutms))
		{
			return false;
		}

		return true;
	}
	virtual bool async_sendto(const std::vector<SBuf>& sendbuf, const NetAddr& other, const SendedCallback2& sended, uint32_t timeoutms)
	{
		shared_ptr< _PollResource> res = resourece;
		if (sock == -1 || sendbuf.size() <= 0 || !sended || res == NULL)
		{
			return false;
		}

		WinIOCP_OVERLAPPED* overlapped = new WinIOCP_OVERLAPPED();
		overlapped->resource = res;
		shared_ptr<Proactor_SendEvent> event = make_shared<Proactor_SendEvent>(overlapped);
		overlapped->event = event;
		//SendEvent(const char* buffer, uint32_t len, const Socket::SendedCallback& _callback, const NetAddr& toaddr)
		event->init(sendbuf, sended, other);
		
		if (!res->postEvent(event,timeoutms))
		{
			return false;
		}

		return true;
	}	
	virtual bool async_sendto(const std::vector<shared_ptr<SendBuffer>>& sendbuf, const NetAddr& other, const SendedCallback3& sended, uint32_t timeoutms)
	{
		shared_ptr< _PollResource> res = resourece;
		if (sock == -1 || sendbuf.size() <= 0 || res == NULL)
		{
			return false;
		}

		WinIOCP_OVERLAPPED* overlapped = new WinIOCP_OVERLAPPED();
		overlapped->resource = res;
		shared_ptr<Proactor_SendEvent> event = make_shared<Proactor_SendEvent>(overlapped);
		overlapped->event = event;
		//SendEvent(const char* buffer, uint32_t len, const Socket::SendedCallback& _callback, const NetAddr& toaddr)
		event->init(sendbuf, sended, other);

		if (!res->postEvent(event,timeoutms))
		{
			return false;
		}

		return true;
	}
};

#endif