#pragma  once
#include "_winevent.h"
#include "../asocket.h"

class _WinSocket:public ASocket
{
	LPFN_CONNECTEX					connectExFunc;
	LPFN_ACCEPTEX					acceptExFunc;
	LPFN_GETACCEPTEXSOCKADDRS		getAcceptAddExFunc;
public:
	_WinSocket(const shared_ptr<IOWorker>& _ioworker, const shared_ptr<IOServer>& _ioserver, const shared_ptr<Socket>& _sockptr, NetType _type)
	:ASocket(_ioworker,_ioserver,_sockptr,_type){}

	_WinSocket(const shared_ptr<IOWorker>& _ioworker, const shared_ptr<IOServer>& _ioserver, const shared_ptr<Socket>& _sockptr, const NewSocketInfo& newsock)
	:ASocket(_ioworker,_ioserver,_sockptr ,newsock){}

	~_WinSocket() {}

	virtual bool creatSocket(NetType type)
	{
		int sockettype = type == NetType_Udp ? SOCK_DGRAM : SOCK_STREAM;
		int protocol = type == NetType_Udp ? IPPROTO_UDP : IPPROTO_TCP;

		sock = WSASocket(AF_INET, sockettype, protocol, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (sock <= 0)
		{
			return false;
		}

		if(type != NetType_Udp)
		{
			GUID acceptEX = WSAID_ACCEPTEX;
			GUID getAcceptAddrEx = WSAID_GETACCEPTEXSOCKADDRS;
			GUID connetEx = WSAID_CONNECTEX;
			DWORD bytes = 0;

			int ret = WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &acceptEX, sizeof(acceptEX), &acceptExFunc, sizeof(acceptExFunc), &bytes, NULL, NULL);

			bytes = 0;
			ret |= WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &getAcceptAddrEx, sizeof(getAcceptAddrEx), &getAcceptAddExFunc, sizeof(getAcceptAddExFunc), &bytes, NULL, NULL);

			bytes = 0;
			ret |= WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &connetEx, sizeof(connetEx), &connectExFunc, sizeof(connectExFunc), &bytes, NULL, NULL);

			if (ret == SOCKET_ERROR)
			{
				assert(0);
			}
		}

#ifdef WIN32
		unsigned long on_windows = 1;
		ioctlsocket(sock, FIONBIO, &on_windows);
#endif

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
		if (!accepted || sock <= 0)
		{
			return false;
		}
		//AcceptEvent(const shared_ptr<IOWorker>& _ioworker,const shared_ptr<Socket>& sock, LPFN_ACCEPTEX	acceptExFunc, LPFN_GETACCEPTEXSOCKADDRS _getAcceptAddExFunc,const Socket::AcceptedCallback& _acceptcallback) :WinEvent(sock)
		shared_ptr<AcceptEvent> event = make_shared<AcceptEvent>(socketptr.lock(), userthread);

		ioserver->pushEvent(&event->overlped, event);

		if (!event->init(ioworker, acceptExFunc, getAcceptAddExFunc, accepted))
		{
			ioserver->popEvent(event.get());
			return false;
		}
		return true;
		
		
		return false; }
	virtual bool async_connect(const NetAddr& addr, const ConnectedCallback& connected)
	{
		if (sock == -1 || type == NetType_TcpConnection || status == NetStatus_connected || addr.getPort() == 0 || !connected)
		{
			return false;
		}

		if (myaddr.getPort() == 0)
		{
			bind(NetAddr(Host::getAvailablePort(50000 + Time::getCurrentMilliSecond() % 10000, Host::SocketType_TCP)));
		}

		otheraddr = addr;

		//ConnectEvent(const NetAddr& toaddr , LPFN_CONNECTEX _connectExFunc,const Socket::ConnectedCallback& _connectcallback)
		shared_ptr<ConnectEvent> event = make_shared<ConnectEvent>(socketptr.lock(), userthread);

		ioserver->pushEvent(&event->overlped, event);

		if (!event->init(addr, connectExFunc, connected))
		{
			ioserver->popEvent(event.get());
			return false;
		}

		return true;
	}
	virtual bool async_recv(char *buf, uint32_t len, const ReceivedCallback& received) 
	{
		if (sock == -1 || status != NetStatus_connected || buf == NULL || len <= 0 || !received)
		{
			return false;
		}

		//RecvEvent(char* buffer, uint32_t len, const Socket::ReceivedCallback& _recvcallback, const Socket::RecvFromCallback& _recvfromcallback)
		shared_ptr<RecvEvent> event = make_shared<RecvEvent>(socketptr.lock(), userthread);

		ioserver->pushEvent(&event->overlped, event);

		if (!event->init(buf, len, received, Socket::RecvFromCallback()))
		{
			ioserver->popEvent(event.get());
			return false;
		}

		return true;
	}
	virtual bool async_recv(const ReceivedCallback& received, int maxlen = 1024) 
	{
		if (sock == -1 || status != NetStatus_connected || maxlen <= 0 || !received)
		{
			return false;
		}

		//RecvEvent(char* buffer, uint32_t len, const Socket::ReceivedCallback& _recvcallback, const Socket::RecvFromCallback& _recvfromcallback)
		shared_ptr<RecvEvent> event = make_shared<RecvEvent>(socketptr.lock(), userthread);

		ioserver->pushEvent(&event->overlped, event);

		if (!event->init(NULL, maxlen, received, Socket::RecvFromCallback()))
		{
			ioserver->popEvent(event.get());
			return false;
		}

		return true;
	}
	virtual bool async_send(const char * buf, uint32_t len, const SendedCallback& sended) 
	{
		if (sock == -1 || status != NetStatus_connected || buf == NULL || len <= 0 || !sended)
		{
			return false;
		}

		//SendEvent(const char* buffer, uint32_t len, const Socket::SendedCallback& _callback, const NetAddr& toaddr)
		shared_ptr<SendEvent> event = make_shared<SendEvent>(socketptr.lock(), userthread);

		ioserver->pushEvent(&event->overlped, event);

		if (!event->init(buf, len, sended, NetAddr()))
		{
			ioserver->popEvent(event.get());
			return false;
		}

		return true;
	}
	virtual bool async_recvfrom(char *buf, uint32_t len, const RecvFromCallback& received) 
	{
		if (sock == -1 ||  buf == NULL || len <= 0 || !received)
		{
			return false;
		}

		//RecvEvent(char* buffer, uint32_t len, const Socket::ReceivedCallback& _recvcallback, const Socket::RecvFromCallback& _recvfromcallback)
		shared_ptr<RecvEvent> event = make_shared<RecvEvent>(socketptr.lock(), userthread);

		ioserver->pushEvent(&event->overlped, event);

		if (!event->init(buf, len, Socket::ReceivedCallback(), received))
		{
			ioserver->popEvent(event.get());
			return false;
		}

		return true;
	}
	virtual bool async_recvfrom(const RecvFromCallback& received, int maxlen = 1024) 
	{
		if (sock == -1 || maxlen <= 0 || !received)
		{
			return false;
		}

		//RecvEvent(char* buffer, uint32_t len, const Socket::ReceivedCallback& _recvcallback, const Socket::RecvFromCallback& _recvfromcallback)
		shared_ptr<RecvEvent> event = make_shared<RecvEvent>(socketptr.lock(), userthread);

		ioserver->pushEvent(&event->overlped, event);

		if (!event->init(NULL, maxlen, Socket::ReceivedCallback(), received))
		{
			ioserver->popEvent(event.get());
			return false;
		}

		return true;
	}
	virtual bool async_sendto(const char * buf, uint32_t len, const NetAddr& other, const SendedCallback& sended) 
	{
		if (sock == -1 || buf == NULL || len <= 0 || !sended)
		{
			return false;
		}

		//SendEvent(const char* buffer, uint32_t len, const Socket::SendedCallback& _callback, const NetAddr& toaddr)
		shared_ptr<SendEvent> event = make_shared<SendEvent>(socketptr.lock(), userthread);

		ioserver->pushEvent(&event->overlped, event);

		if (!event->init(buf, len, sended, other))
		{
			ioserver->popEvent(event.get());
			return false;
		}

		return true;
	}	
};