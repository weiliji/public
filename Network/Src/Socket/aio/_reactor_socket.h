#pragma once

#ifdef WIN32
#include <winsock2.h>
#include <ws2ipdef.h>
#include <mswsock.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

#include "_reactor_event.h"
#include "asocket.h"

class _Reactor_Socket : public ASocket
{
public:
	_Reactor_Socket(const shared_ptr<IOWorker> &_ioworker, const shared_ptr<_Poll> &_poll, InetType _inet, NetType _type, SOCKET _sock, const NetAddr &_otheraddr)
		: ASocket(_ioworker, _poll, _type, _inet, _sock, _otheraddr) {}

	~_Reactor_Socket() {}

	virtual bool creatSocket(NetType type)
	{
		int sockettype = type == NetType_Udp ? SOCK_DGRAM : SOCK_STREAM;
		int protocol = type == NetType_Udp ? IPPROTO_UDP : IPPROTO_TCP;
		int inet = _famlily == INET_IPV4 ? AF_INET : AF_INET6;

		sock = socket(inet, sockettype, protocol);
		if (sock <= 0)
		{
			perror("socket");
			return false;
		}

		{
#ifdef WIN32
			unsigned long on_windows = 1;

			return ioctlsocket(sock, FIONBIO, &on_windows) == 0;
#else
			int flags = fcntl(sock, F_GETFL, 0);

			flags |= O_NONBLOCK;

			fcntl(sock, F_SETFL, flags);

			return true;
#endif
		}

		return true;
	}

	virtual bool disconnect()
	{
		ASocket::disconnect();

		if (sock > 0)
			closesocket(sock);

		sock = INVALIDHANDLE;		


		return true;
	}
	virtual bool async_accept(const AcceptedCallback &accepted, uint32_t timeoutms)
	{
		if (!accepted || sock <= 0)
		{
			return false;
		}

		if (type == NetType_TcpServer && !ishavelisten)
		{
			int ret = listen(sock, SOMAXCONN);
			if (ret < 0)
			{
				return false;
			}

			ishavelisten = true;
		}

		//AcceptEvent(const shared_ptr<IOWorker>& _ioworker,const shared_ptr<Socket>& sock, LPFN_ACCEPTEX	acceptExFunc, LPFN_GETACCEPTEXSOCKADDRS _getAcceptAddExFunc,const Socket::AcceptedCallback& _acceptcallback) :WinIOCPEvent(sock)
		shared_ptr<Reactor_AcceptEvent> event = make_shared<Reactor_AcceptEvent>(worker, accepted);
		if (!resourece->postEvent(event,timeoutms))
		{
			return false;
		}
		return true;

		return false;
	}
	virtual bool async_connect(const NetAddr &addr, const ConnectedCallback &connected, uint32_t timeoutms)
	{
		shared_ptr<_PollResource> res = resourece;
		if (sock == (SOCKET)INVALIDHANDLE || type == NetType_TcpConnection || status == NetStatus_connected || addr.getPort() == 0 || !connected || res == NULL)
		{
			return false;
		}

		otheraddr = addr;
		//ConnectEvent(const NetAddr& toaddr , LPFN_CONNECTEX _connectExFunc,const Socket::ConnectedCallback& _connectcallback)
		shared_ptr<Reactor_ConnectEvent> event = make_shared<Reactor_ConnectEvent>(addr, connected);

		
		if (!res->postEvent(event,timeoutms))
		{
			return false;
		}

		return true;
	}
	virtual bool async_recv(const shared_ptr<RecvBuffer> &buffer, const ReceivedCallback1 &received, uint32_t timeoutms)
	{
		shared_ptr<_PollResource> res = resourece;
		if (sock == (SOCKET)INVALIDHANDLE || status != NetStatus_connected || buffer == NULL || buffer->bufferSize() <= 0 || !received || res == NULL)
		{
			return false;
		}

		shared_ptr<Reactor_RecvEvent> event = make_shared<Reactor_RecvEvent>(buffer, received, Socket::RecvFromCallback1());

		if (!res->postEvent(event,timeoutms))
		{
			return false;
		}

		return true;
	}
	virtual bool async_recv(char *buf, uint32_t len, const ReceivedCallback &received, uint32_t timeoutms)
	{
		shared_ptr<_PollResource> res = resourece;
		if (sock == (SOCKET)INVALIDHANDLE || status != NetStatus_connected || buf == NULL || len <= 0 || !received || res == NULL)
		{
			return false;
		}

		//RecvEvent(char* buffer, uint32_t len, const Socket::ReceivedCallback& _recvcallback, const Socket::RecvFromCallback& _recvfromcallback)
		shared_ptr<Reactor_RecvEvent> event = make_shared<Reactor_RecvEvent>(buf, len, received, Socket::RecvFromCallback());

		if (!res->postEvent(event,timeoutms))
		{
			return false;
		}

		return true;
	}
	virtual bool async_recv(const ReceivedCallback &received, int maxlen, uint32_t timeoutms)
	{
		shared_ptr<_PollResource> res = resourece;
		if (sock == (SOCKET)INVALIDHANDLE || status != NetStatus_connected || maxlen <= 0 || !received || res == NULL)
		{
			return false;
		}

		//(char* buffer, uint32_t len, const Socket::ReceivedCallback& _recvcallback, const Socket::RecvFromCallback& _recvfromcallback)
		shared_ptr<Reactor_RecvEvent> event = make_shared<Reactor_RecvEvent>((char *)NULL, maxlen, received, Socket::RecvFromCallback());

		if (!res->postEvent(event,timeoutms))
		{
			return false;
		}

		return true;
	}
	virtual bool async_send(const char *buf, uint32_t len, const SendedCallback &sended, uint32_t timeoutms)
	{
		shared_ptr<_PollResource> res = resourece;
		if (sock == (SOCKET)INVALIDHANDLE || buf == NULL || len <= 0 || !sended || status != NetStatus_connected || !sended || res == NULL)
		{
			return false;
		}

		shared_ptr<Reactor_SendEvent> event = make_shared<Reactor_SendEvent>(buf, len, sended, NetAddr());
		if (!res->postEvent(event,timeoutms))
		{
			return false;
		}

		return true;
	}
	virtual bool async_send(const shared_ptr<SendBuffer> &buffer, const SendedCallback1 &sended, uint32_t timeoutms)
	{
		shared_ptr<_PollResource> res = resourece;
		if (sock == (SOCKET)INVALIDHANDLE || status != NetStatus_connected || buffer == NULL || buffer->bufferlen() <= 0 || res == NULL)
		{
			return false;
		}

		shared_ptr<Reactor_SendEvent> event = make_shared<Reactor_SendEvent>(buffer, sended, NetAddr());
		if (!res->postEvent(event,timeoutms))
		{
			return false;
		}

		return true;
	}
	virtual bool async_send(const std::vector<SBuf> &sendbuf, const SendedCallback2 &sended, uint32_t timeoutms)
	{
		shared_ptr<_PollResource> res = resourece;
		if (sock == (SOCKET)INVALIDHANDLE || status != NetStatus_connected || sendbuf.size() <= 0 || !sended || res == NULL)
		{
			return false;
		}

		//SendEvent(const char* buffer, uint32_t len, const Socket::SendedCallback& _callback, const NetAddr& toaddr)
		shared_ptr<Reactor_SendEvent> event = make_shared<Reactor_SendEvent>(sendbuf, sended, NetAddr());

		if (!res->postEvent(event,timeoutms))
		{
			return false;
		}

		return true;
	}
	virtual bool async_send(const std::vector<shared_ptr<SendBuffer>> &sendbuf, const SendedCallback3 &sended, uint32_t timeoutms)
	{
		shared_ptr<_PollResource> res = resourece;
		if (sock == (SOCKET)INVALIDHANDLE || status != NetStatus_connected || sendbuf.size() <= 0 || res == NULL)
		{
			return false;
		}

		//SendEvent(const char* buffer, uint32_t len, const Socket::SendedCallback& _callback, const NetAddr& toaddr)
		shared_ptr<Reactor_SendEvent> event = make_shared<Reactor_SendEvent>(sendbuf, sended, NetAddr());

		if (!res->postEvent(event,timeoutms))
		{
			return false;
		}

		return true;
	}
	virtual bool async_recvfrom(const shared_ptr<RecvBuffer> &buffer, const RecvFromCallback1 &received, uint32_t timeoutms)
	{
		shared_ptr<_PollResource> res = resourece;
		if (sock == (SOCKET)INVALIDHANDLE || buffer == NULL || buffer->bufferSize() <= 0 || !received || res == NULL)
		{
			return false;
		}

		shared_ptr<Reactor_RecvEvent> event = make_shared<Reactor_RecvEvent>(buffer, Socket::ReceivedCallback1(), received);

		if (!res->postEvent(event,timeoutms))
		{
			return false;
		}

		return true;
	}
	virtual bool async_recvfrom(char *buf, uint32_t len, const RecvFromCallback &received, uint32_t timeoutms)
	{
		shared_ptr<_PollResource> res = resourece;
		if (sock == (SOCKET)INVALIDHANDLE || buf == NULL || len <= 0 || !received || res == NULL)
		{
			return false;
		}

		//RecvEvent(char* buffer, uint32_t len, const Socket::ReceivedCallback& _recvcallback, const Socket::RecvFromCallback& _recvfromcallback)
		shared_ptr<Reactor_RecvEvent> event = make_shared<Reactor_RecvEvent>(buf, len, Socket::ReceivedCallback(), received);

		if (!res->postEvent(event,timeoutms))
		{
			return false;
		}

		return true;
	}
	virtual bool async_recvfrom(const RecvFromCallback &received, int maxlen, uint32_t timeoutms)
	{
		shared_ptr<_PollResource> res = resourece;
		if (sock == (SOCKET)INVALIDHANDLE || maxlen <= 0 || !received || res == NULL)
		{
			return false;
		}

		//RecvEvent(char* buffer, uint32_t len, const Socket::ReceivedCallback& _recvcallback, const Socket::RecvFromCallback& _recvfromcallback)
		shared_ptr<Reactor_RecvEvent> event = make_shared<Reactor_RecvEvent>((char *)NULL, maxlen, Socket::ReceivedCallback(), received);

		if (!res->postEvent(event,timeoutms))
		{
			return false;
		}

		return true;
	}
	virtual bool async_sendto(const char *buf, uint32_t len, const NetAddr &other, const SendedCallback &sended, uint32_t timeoutms)
	{
		shared_ptr<_PollResource> res = resourece;
		if (sock == (SOCKET)INVALIDHANDLE || buf == NULL || len <= 0 || !sended || res == NULL)
		{
			return false;
		}

		shared_ptr<Reactor_SendEvent> event = make_shared<Reactor_SendEvent>(buf, len, sended, other);
		if (!res->postEvent(event,timeoutms))
		{
			return false;
		}

		return true;
	}
	virtual bool async_sendto(const shared_ptr<SendBuffer> &buffer, const NetAddr &other, const SendedCallback1 &sended, uint32_t timeoutms)
	{
		shared_ptr<_PollResource> res = resourece;
		if (sock == (SOCKET)INVALIDHANDLE || buffer == NULL || buffer->bufferlen() <= 0 || res == NULL)
		{
			return false;
		}

		shared_ptr<Reactor_SendEvent> event = make_shared<Reactor_SendEvent>(buffer, sended, other);
		if (!res->postEvent(event,timeoutms))
		{
			return false;
		}

		return true;
	}

	virtual bool async_sendto(const std::vector<SBuf> &sendbuf, const NetAddr &other, const SendedCallback2 &sended, uint32_t timeoutms)
	{
		shared_ptr<_PollResource> res = resourece;
		if (sock == (SOCKET)INVALIDHANDLE || sendbuf.size() <= 0 || !sended || res == NULL)
		{
			return false;
		}

		//SendEvent(const char* buffer, uint32_t len, const Socket::SendedCallback& _callback, const NetAddr& toaddr)
		shared_ptr<Reactor_SendEvent> event = make_shared<Reactor_SendEvent>(sendbuf, sended, other);

		if (!res->postEvent(event,timeoutms))
		{
			return false;
		}

		return true;
	}
	virtual bool async_sendto(const std::vector<shared_ptr<SendBuffer>> &sendbuf, const NetAddr &other, const SendedCallback3 &sended, uint32_t timeoutms)
	{
		shared_ptr<_PollResource> res = resourece;
		if (sock == (SOCKET)INVALIDHANDLE || sendbuf.size() <= 0 || res == NULL)
		{
			return false;
		}

		//SendEvent(const char* buffer, uint32_t len, const Socket::SendedCallback& _callback, const NetAddr& toaddr)
		shared_ptr<Reactor_SendEvent> event = make_shared<Reactor_SendEvent>(sendbuf, sended, other);

		if (!res->postEvent(event,timeoutms))
		{
			return false;
		}

		return true;
	}
};
