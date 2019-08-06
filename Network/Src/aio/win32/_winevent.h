#pragma  once

#ifdef WIN32

#include <winsock2.h>
#include <mswsock.h>
typedef int socklen_t;

#include "../common/_pool.h"
#include "../common/_eventthreadpool.h"
#define SWAPBUFFERLEN			100

struct WinEvent
{
	OVERLAPPED		overlped;
	WSABUF			wbuf;
	SOCKADDR		addr;
	int 			addrlen;
	DWORD			dwBytes;
	DWORD			dwFlags;
	int				newsock;

	WinEvent()
	{
		memset(&overlped, 0, sizeof(overlped));
		memset(&wbuf, 0, sizeof(wbuf));
		memset(&addr, 0, sizeof(addr));
		dwBytes = dwFlags = 0;
		newsock = 0;

		addr.sa_family = AF_INET;
		addrlen = sizeof(SOCKADDR);
	}
	virtual ~WinEvent() {}
};

struct SendEvent :public Event,public WinEvent
{
	Socket::SendedCallback sendcallback;

	SendEvent(const shared_ptr<Socket>& sock, const shared_ptr<_UserThread>& userthread):Event(sock,userthread)
	{}
	bool init(const char* buffer, uint32_t len, const Socket::SendedCallback& _callback, const NetAddr& toaddr)
	{
		shared_ptr<Socket> sock = weak_sock.lock();
		if (sock == NULL)
		{
			assert(0);
		}
		{
			addr = *(SOCKADDR*)toaddr.getAddr();
			addrlen = toaddr.getAddrLen();
			sendcallback = _callback;
			wbuf.buf = (CHAR*)buffer;
			wbuf.len = len;
		}
		
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

		return true;
	}
	void doEvent(const shared_ptr<Socket>& sock, int bytes, bool status)
	{
		sendcallback(sock, (const char*)wbuf.buf, bytes);
	}
};

struct RecvEvent :public Event,public WinEvent
{
	Socket::ReceivedCallback recvcallback;
	Socket::RecvFromCallback recvfromcallback;

	bool needFreeBuffer = false;

	RecvEvent(const shared_ptr<Socket>& sock, const shared_ptr<_UserThread>& userthread) :Event(sock, userthread)
	{}
	~RecvEvent() 
	{ 
		if (needFreeBuffer) 
			delete[](char*)wbuf.buf;
	}
	bool init(char* buffer, uint32_t len, const Socket::ReceivedCallback& _recvcallback, const Socket::RecvFromCallback& _recvfromcallback)
	{
		shared_ptr<Socket> sock = weak_sock.lock();
		if (sock == NULL)
		{
			assert(0);
		}

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
		int ret = 0;
		if (recvcallback)
		{
			ret = WSARecv(sock->getHandle(), &wbuf, 1, &dwBytes, &dwFlags, &overlped, NULL);
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

		return true;
	}
	bool socketIsAlive(const shared_ptr<Socket>& sock)
	{
		int sendret = ::send(sock->getHandle(), "", 0, 0);

		return sendret != -1;
	}
	void doEvent(const shared_ptr<Socket>& sock, int bytes, bool status)
	{
		if (!status && !socketIsAlive(sock))
		{
			sock->socketError("socket disconnected");
		}
		else if (bytes <= 0 && !socketIsAlive(sock))
		{
			sock->socketError("socket disconnected");
		}
		else if (status && bytes > 0)
		{
			if (recvcallback) recvcallback(sock, (const char*)wbuf.buf, bytes);
			else recvfromcallback(sock, (const char*)wbuf.buf, bytes, NetAddr(*(SockAddr*)&addr));
		}
	}
};

struct AcceptEvent :public Event,public WinEvent
{
	weak_ptr<IOWorker>		 ioworker;
	Socket::AcceptedCallback acceptcallback;
	char						swapBuffer[SWAPBUFFERLEN];

	AcceptEvent(const shared_ptr<Socket>& sock, const shared_ptr<_UserThread>& userthread) :Event(sock, userthread)
	{		
		wbuf.buf = swapBuffer;
		wbuf.len = SWAPBUFFERLEN;
	}
	bool init(const shared_ptr<IOWorker>& _ioworker, const Socket::AcceptedCallback& _acceptcallback)
	{
		shared_ptr<Socket> sock = weak_sock.lock();
		if (sock == NULL)
		{
			assert(0);
		}
		{
			ioworker = _ioworker;
			acceptcallback = _acceptcallback;
		}

		LPFN_ACCEPTEX	acceptExFunc;
		GUID acceptEX = WSAID_ACCEPTEX;
		DWORD bytes = 0;

		int ret = WSAIoctl(sock->getHandle(), SIO_GET_EXTENSION_FUNCTION_POINTER, &acceptEX, sizeof(acceptEX), &acceptExFunc, sizeof(acceptExFunc), &bytes, NULL, NULL);
		if (ret == SOCKET_ERROR)
		{
			assert(0);
		}

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

		return true;
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
			LPFN_GETACCEPTEXSOCKADDRS		getAcceptAddExFunc;
			GUID getAcceptAddrEx = WSAID_GETACCEPTEXSOCKADDRS;
			DWORD bytes = 0;

			int ret = WSAIoctl(sock->getHandle(), SIO_GET_EXTENSION_FUNCTION_POINTER, &getAcceptAddrEx, sizeof(getAcceptAddrEx), &getAcceptAddExFunc, sizeof(getAcceptAddExFunc), &bytes, NULL, NULL);
			if (ret == SOCKET_ERROR)
			{
				assert(0);
			}


			SOCKADDR_IN* clientAddr = NULL, *localAddr = NULL;
			int clientlen = sizeof(SOCKADDR_IN), locallen = sizeof(SOCKADDR_IN);

			getAcceptAddExFunc(wbuf.buf, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, (LPSOCKADDR*)&localAddr, &locallen, (LPSOCKADDR*)&clientAddr, &clientlen);

			NewSocketInfo* newsocketinfo = new NewSocketInfo;
			newsocketinfo->newsocket = newsock;
			newsocketinfo->otheraddr = NetAddr(*(SockAddr*)clientAddr);

			shared_ptr<Socket> newsock = TCPClient::create(worker, newsocketinfo);
			acceptcallback(sock, newsock);
		}
	}
};

struct ConnectEvent :public Event,public WinEvent
{
	char						swapBuffer[SWAPBUFFERLEN];
	Socket::ConnectedCallback connectcallback;
	
	
	ConnectEvent(const shared_ptr<Socket>& sock, const shared_ptr<_UserThread>& userthread) :Event(sock, userthread){}
	bool init(const NetAddr& toaddr, const Socket::ConnectedCallback& _connectcallback)
	{
		shared_ptr<Socket> sock = weak_sock.lock();
		if (sock == NULL)
		{
			assert(0);
		}

		{
			wbuf.buf = swapBuffer;
			wbuf.len = SWAPBUFFERLEN;
			connectcallback = _connectcallback;

			addr = *(SOCKADDR*)toaddr.getAddr();
			addrlen = toaddr.getAddrLen();
		}

		GUID connetEx = WSAID_CONNECTEX;
		DWORD bytes = 0;
		LPFN_CONNECTEX			connectExFunc;

		int ret = WSAIoctl(sock->getHandle(), SIO_GET_EXTENSION_FUNCTION_POINTER, &connetEx, sizeof(connetEx), &connectExFunc, sizeof(connectExFunc), &bytes, NULL, NULL);

		if (ret == SOCKET_ERROR)
		{
			assert(0);
		}

		if (false == connectExFunc(sock->getHandle(), &addr, addrlen, NULL, 0,NULL, &overlped))
		{
			int errorno = GetLastError();
			if (errorno != WSA_IO_PENDING)
			{
				return false;
			}
		}

		return true;
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