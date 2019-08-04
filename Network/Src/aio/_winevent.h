#pragma  once
#include "_pool.h"
#include "_eventthreadpool.h"
#include "Network/TcpClient.h"

#ifdef WIN32

#include <mswsock.h>

struct SendEvent :public Event
{
	Socket::SendedCallback sendcallback;

	SendEvent(const char* buffer, uint32_t len, const Socket::SendedCallback& _callback, const NetAddr& toaddr)
	{
		addr = *toaddr.getAddr();
		addrlen = toaddr.getAddrLen();
		sendcallback = _callback;
		wbuf.buf = (CHAR*)buffer;
		wbuf.len = len;
	}
	bool init(const shared_ptr<Socket>& sock, const shared_ptr<UserThread>& userthread)
	{
		NetAddr toaddr(addr);

		int ret = 0;
		if (toaddr.getPort() == 0)
		{
			ret = ::WSASend(sock->getHandle(), &wbuf, 1, &dwBytes, 0, &overlped, NULL);
		}
		else
		{
			ret = ::WSASendTo(sock->getHandle(), &wbuf, 1, &dwBytes, 0, (sockaddr*)&addr, addrlen, &overlped, NULL);
		}

		if (ret == SOCKET_ERROR)
		{
			int errorno = GetLastError();
			if (errorno != WSA_IO_PENDING)
			{
				return false;
			}
		}

		return Event::init(sock,userthread);
	}
	void doEvent(const shared_ptr<Socket>& sock, int bytes, bool status)
	{
		sendcallback(sock, (const char*)wbuf.buf, bytes);
	}
};

struct ReadEvent :public Event
{
	Socket::ReceivedCallback recvcallback;
	Socket::RecvFromCallback recvfromcallback;

	bool needFreeBuffer = false;

	ReadEvent(char* buffer, uint32_t len, const Socket::ReceivedCallback& _recvcallback, const Socket::RecvFromCallback& _recvfromcallback)
	{
		if (buffer == NULL)
		{
			needFreeBuffer = true;
			buffer = new char[len];
		}
		recvcallback = _recvcallback;
		recvfromcallback = _recvfromcallback;

		wbuf.buf = (CHAR*)buffer;
		wbuf.len = len;
	}
	~ReadEvent() { if (needFreeBuffer) delete[](char*)wbuf.buf; }
	bool init(const shared_ptr<Socket>& sock, const shared_ptr<UserThread>& userthread)
	{
		int ret = 0;
		if (recvcallback)
		{
			ret = WSARecv(sock->getHandle(), &wbuf, 1, &dwBytes, &dwFlags,&overlped, NULL);
		}
		else
		{
			ret = WSARecvFrom(sock->getHandle(), &wbuf, 1, &dwBytes, &dwFlags,(sockaddr*)&addr, &addrlen, &overlped, NULL);
		}

		if (ret == SOCKET_ERROR)
		{
			int errorno = GetLastError();
			if (errorno != WSA_IO_PENDING)
			{
				return false;
			}
		}

		return Event::init(sock,userthread);
	}
	bool socketIsAlive(const shared_ptr<Socket>& sock)
	{
		int sendret = ::send(sock->getHandle(), "", 0, 0);

		return sendret != -1;
	}
	void doEvent(const shared_ptr<Socket>& sock, int bytes, bool status)
	{
		if (!status && !socketIsAlive(sock)) sock->socketError("socket disconnected");
		else
		{
			if (recvcallback) recvcallback(sock, (const char*)wbuf.buf, bytes);
			else recvfromcallback(sock, (const char*)wbuf.buf, bytes, NetAddr(addr));
		}
	}
};

struct AcceptEvent :public Event
{
	weak_ptr<IOWorker>		 ioworker;
	Socket::AcceptedCallback acceptcallback;
	LPFN_ACCEPTEX	acceptExFunc;
	LPFN_GETACCEPTEXSOCKADDRS		getAcceptAddExFunc;

	AcceptEvent(const shared_ptr<IOWorker>& _ioworker,LPFN_ACCEPTEX	_acceptExFunc, LPFN_GETACCEPTEXSOCKADDRS _getAcceptAddExFunc,const Socket::AcceptedCallback& _acceptcallback)
	{
		ioworker = _ioworker;
		acceptcallback = _acceptcallback;
		acceptExFunc = _acceptExFunc;
		getAcceptAddExFunc = _getAcceptAddExFunc;
	}
	bool init(const shared_ptr<Socket>& sock, const shared_ptr<UserThread>& userthread)
	{
		newsock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (newsock == INVALID_SOCKET)
		{
			return false;
		}

		if (false == acceptExFunc(sock->getHandle(), newsock, wbuf.buf, 0,sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, NULL, (LPOVERLAPPED)&overlped))
		{
			int errorno = GetLastError();
			if (errorno != WSA_IO_PENDING)
			{
				closesocket(newsock);
				return false;
			}
		}

		return Event::init(sock,userthread);
	}
	void doEvent(const shared_ptr<Socket>& sock, int bytes, bool status)
	{
		shared_ptr<IOWorker> worker = ioworker.lock();
		if (worker == NULL) return;

		if (!status)
		{
			closesocket(newsock);

			acceptcallback(sock, shared_ptr<Socket>());
		}
		else
		{
			SOCKADDR_IN* clientAddr = NULL, *localAddr = NULL;
			int clientlen = sizeof(SOCKADDR_IN), locallen = sizeof(SOCKADDR_IN);

			getAcceptAddExFunc(wbuf.buf, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, (LPSOCKADDR*)&localAddr, &locallen, (LPSOCKADDR*)&clientAddr, &clientlen);

			NewSocketInfo* newsocketinfo = new NewSocketInfo;
			newsocketinfo->newsock = newsock;
			newsocketinfo->otheraddr = NetAddr(*clientAddr);

			shared_ptr<Socket> newsock = TCPClient::create(worker, newsocketinfo);
			acceptcallback(sock, newsock);
		}
	}
};

struct ConnectEvent :public Event
{
#define SWAPBUFFERLEN			64
	char						swapBuffer[SWAPBUFFERLEN];

	Socket::ConnectedCallback connectcallback;
	LPFN_CONNECTEX			connectExFunc;
	
	ConnectEvent(const NetAddr& toaddr , LPFN_CONNECTEX _connectExFunc,const Socket::ConnectedCallback& _connectcallback)
	{
		wbuf.buf = swapBuffer;
		wbuf.len = SWAPBUFFERLEN;
		connectcallback = _connectcallback;
		connectExFunc = _connectExFunc;

		addr = *toaddr.getAddr();
		addrlen = toaddr.getAddrLen();
	}
	bool init(const shared_ptr<Socket>& sock, const shared_ptr<UserThread>& userthread)
	{
		if (false == connectExFunc(sock->getHandle(), (SOCKADDR*)&addr, addrlen, NULL, 0, NULL, &overlped))
		{
			int errorno = GetLastError();
			if (errorno != WSA_IO_PENDING)
			{
				return false;
			}
		}

		return Event::init(sock,userthread);
	}
	void doEvent(const shared_ptr<Socket>& sock, int bytes, bool status)
	{
		if (!status) connectcallback(sock, false, "connected error");
		else
		{
			sock->socketReady();
			connectcallback(sock, true, "connected success");
		}
	}
};

#endif