#pragma  once

#ifdef WIN32

#include <winsock2.h>
#include <mswsock.h>
typedef int socklen_t;

#include "../common/_pool.h"
#define SWAPBUFFERLEN			100

struct WinEvent:public Event
{
	OVERLAPPED		overlped;
	WSABUF			wbuf;
	SOCKADDR		addr;
	int 			addrlen;
	DWORD			dwBytes;
	DWORD			dwFlags;
	int				newsock;

	WinEvent(EventType _pooltype):Event(_pooltype)
	{
		reset();
	}
	virtual ~WinEvent() {}

	virtual void doEvent(const shared_ptr<Socket>& sock, int bytes, bool status) {}

	void reset()
	{
		memset(&overlped, 0, sizeof(overlped));
		memset(&wbuf, 0, sizeof(wbuf));
		memset(&addr, 0, sizeof(addr));
		dwBytes = dwFlags = 0;
		newsock = 0;

		addr.sa_family = AF_INET;
		addrlen = sizeof(SOCKADDR);
	}
};

struct SendEvent :public WinEvent
{
	WSABUF* wsbuf;
	uint32_t wsbufsize;
	Socket::SendedCallback sendcallback;

	SendEvent():WinEvent(EventType_Write), wsbuf(NULL),wsbufsize(0){}
	~SendEvent() 
	{
		if (wsbuf != NULL)
		{
			delete[] wsbuf;
			wsbuf = NULL;
			wsbufsize = 0;
		}
	}
	void init(const std::deque<Socket::SBuf>& _sendbuf, const Socket::SendedCallback& _callback,const NetAddr& toaddr)
	{
		if (wsbuf != NULL)
		{
			delete[] wsbuf;
			wsbuf = NULL;
			wsbufsize = 0;
		}
		reset();

		addr = *(SOCKADDR*)toaddr.getAddr();
		addrlen = toaddr.getAddrLen();
		sendcallback = _callback;

		wsbuf = new WSABUF[_sendbuf.size()];
		uint32_t sendindex = 0;
		for (std::deque<Socket::SBuf>::const_iterator iter = _sendbuf.begin(); iter != _sendbuf.end(); iter++, sendindex++)
		{
			wsbuf[sendindex].buf = (CHAR*)iter->bufadd;
			wsbuf[sendindex].len = iter->buflen;
		}
		wsbufsize = sendindex;
	}
	
	bool init(const shared_ptr<Socket>& sock, const shared_ptr<_UserThread>& _userthread)
	{
		Event::init(sock, _userthread);


		int ret = 0;
		if (NetAddr(addr).getPort() == 0)
		{
			ret = ::WSASend(sock->getHandle(), wsbuf, wsbufsize, &dwBytes, 0, &overlped, NULL);
		}
		else
		{
			ret = ::WSASendTo(sock->getHandle(), wsbuf, wsbufsize, &dwBytes, 0, (sockaddr*)&addr, addrlen, &overlped, NULL);
		}

		if (ret == SOCKET_ERROR)
		{
			int errorno = GetLastError();
			if (errorno != WSA_IO_PENDING)
			{
				//10054 是socket 被他人断开
				if (errorno == 10054)
				{
					sock->socketError("disconnected");
				}
				return false;
			}
		}

		return true;
	}
	void doEvent(const shared_ptr<Socket>& sock, int bytes, bool status)
	{
		sendcallback(sock, (const char*)wsbuf[0].buf, bytes);
	}
};

struct RecvEvent :public WinEvent
{
	Socket::ReceivedCallback recvcallback;
	Socket::RecvFromCallback recvfromcallback;

	RecvEvent():WinEvent(EventType_Read){}

	bool needFreeBuffer = false;
	void init(char* buffer, uint32_t len, const Socket::ReceivedCallback& _recvcallback, const Socket::RecvFromCallback& _recvfromcallback)
	{
		if (needFreeBuffer && wbuf.buf != NULL)
		{
			delete[](char*)wbuf.buf;
		}
		needFreeBuffer = false;
		reset();
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
	~RecvEvent() 
	{ 
		if (needFreeBuffer) 
			delete[](char*)wbuf.buf;
	}
	bool init(const shared_ptr<Socket>& sock, const shared_ptr<_UserThread>& _userthread)
	{
		Event::init(sock, _userthread);


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
		if (!status && recvcallback && !socketIsAlive(sock))
		{
			sock->socketError("socket disconnected");
		}
		else if (bytes <= 0 && recvcallback && !socketIsAlive(sock))
		{
			sock->socketError("socket disconnected");
		}
		else 
		{
			if (recvcallback) recvcallback(sock, (const char*)wbuf.buf, bytes);
			else recvfromcallback(sock, (const char*)wbuf.buf, bytes, NetAddr(*(SockAddr*)&addr));
		}
	}
};

struct AcceptEvent :public WinEvent
{
	Socket::AcceptedCallback acceptcallback;
	char						swapBuffer[SWAPBUFFERLEN];

	AcceptEvent() :WinEvent(EventType_Read) {}
	void init(const Socket::AcceptedCallback& _acceptcallback)
	{		
		reset();
		acceptcallback = _acceptcallback;
		wbuf.buf = swapBuffer;
		wbuf.len = SWAPBUFFERLEN;
	}
	bool init(const shared_ptr<Socket>& sock, const shared_ptr<_UserThread>& _userthread)
	{
		Event::init(sock, _userthread);


		LPFN_ACCEPTEX	acceptExFunc;
		GUID acceptEX = WSAID_ACCEPTEX;
		DWORD bytes = 0;

		int ret = WSAIoctl(sock->getHandle(), SIO_GET_EXTENSION_FUNCTION_POINTER, &acceptEX, sizeof(acceptEX), &acceptExFunc, sizeof(acceptExFunc), &bytes, NULL, NULL);
		if (ret == SOCKET_ERROR)
		{
			assert(0);
		}

		newsock = (int)WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
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

			TCPClient::NewSocketInfo newsocketinfo;
			newsocketinfo.newsocket = newsock;
			newsocketinfo.otheraddr = NetAddr(*(SockAddr*)clientAddr);

			shared_ptr<Socket> newsock = TCPClient::create(sock->ioWorker(), newsocketinfo);
			acceptcallback(sock, newsock);
		}
	}
};

struct ConnectEvent :public WinEvent
{
	char						swapBuffer[SWAPBUFFERLEN];
	Socket::ConnectedCallback connectcallback;
	
	ConnectEvent() :WinEvent(EventType_Write) {}
	
	void init(const NetAddr& toaddr, const Socket::ConnectedCallback& _connectcallback)
	{
		reset();
		wbuf.buf = swapBuffer;
		wbuf.len = SWAPBUFFERLEN;
		connectcallback = _connectcallback;

		addr = *(SOCKADDR*)toaddr.getAddr();
		addrlen = toaddr.getAddrLen();
	}
	bool init(const shared_ptr<Socket>& sock, const shared_ptr<_UserThread>& _userthread)
	{
		Event::init(sock, _userthread);

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