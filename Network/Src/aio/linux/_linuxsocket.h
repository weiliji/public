#pragma  once
#include "_linuxevent.h"
#include "../asocket.h"

#ifndef WIN32

class _LinuxSocket:public ASocket
{
public:
	_LinuxSocket(const shared_ptr<IOWorker>& _ioworker, const shared_ptr<IOServer>& _ioserver, const shared_ptr<Socket>& _sockptr, NetType _type)
	:ASocket(_ioworker,_ioserver,_sockptr,_type){}

	_LinuxSocket(const shared_ptr<IOWorker>& _ioworker, const shared_ptr<IOServer>& _ioserver, const shared_ptr<Socket>& _sockptr, const NewSocketInfo& newsock)
	:ASocket(_ioworker,_ioserver,_sockptr ,newsock)
	{
	}

	~_WinSocket() {}

	virtual bool creatSocket(NetType type)
	{
		int sockettype = type == NetType_Udp ? SOCK_DGRAM : SOCK_STREAM;
		int protocol = type == NetType_Udp ? IPPROTO_UDP : IPPROTO_TCP;

		sock = socket(AF_INET, sockettype, protocol);
		if (sock <= 0)
		{
			perror("socket");
			return false;
		}

		nonBlocking(true);

		setSocketTimeout(1000, 1000);

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
		shared_ptr<AcceptEvent> event = make_shared<AcceptEvent>(ioworker, accepted);
		if(!resourece->postEvent(&event->overlped, event))
		{
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

		otheraddr = addr;

		//ConnectEvent(const NetAddr& toaddr , LPFN_CONNECTEX _connectExFunc,const Socket::ConnectedCallback& _connectcallback)
		shared_ptr<ConnectEvent> event = make_shared<ConnectEvent>(addr, connected);

		if (!resourece->postEvent(&event->overlped, event))
		{
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
		shared_ptr<RecvEvent> event = make_shared<RecvEvent>(buf, len, received, Socket::RecvFromCallback());

		if (!resourece->postEvent(&event->overlped, event))
		{
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

		//(char* buffer, uint32_t len, const Socket::ReceivedCallback& _recvcallback, const Socket::RecvFromCallback& _recvfromcallback)
		shared_ptr<RecvEvent> event = make_shared<RecvEvent>((char*)NULL, maxlen, received, Socket::RecvFromCallback());

		if (!resourece->postEvent(&event->overlped, event))
		{
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
		shared_ptr<SendEvent> event = make_shared<SendEvent>(buf, len, sended, NetAddr());

		if (!resourece->postEvent(&event->overlped, event))
		{
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
		shared_ptr<RecvEvent> event = make_shared<RecvEvent>(buf, len, Socket::ReceivedCallback(), received);

		if (!resourece->postEvent(&event->overlped, event))
		{
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
		shared_ptr<RecvEvent> event = make_shared<RecvEvent>((char*)NULL, maxlen, Socket::ReceivedCallback(), received);
		
		if (!resourece->postEvent(&event->overlped, event))
		{
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
		shared_ptr<SendEvent> event = make_shared<SendEvent>(buf, len, sended, other);
		
		if (!resourece->postEvent(&event->overlped, event))
		{
			return false;
		}

		return true;
	}	
	virtual bool nonBlocking(bool nonblock)
	{
		if (sock <= 0) return false;


		int flags = fcntl(handle, F_GETFL, 0);

		if (nonblock) flags |= O_NONBLOCK;
		else flags &= ^O_NONBLOCK;

		fcntl(sock, F_SETFL, flags);

		return true;
	}
};

#endif