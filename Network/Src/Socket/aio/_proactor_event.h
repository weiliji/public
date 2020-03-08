#pragma  once

#include "_poll.h"

#ifdef SUPPORT_IOCP

#include <winsock2.h>
#include <ws2ipdef.h>
#include <mswsock.h>
typedef int socklen_t;

#define SWAPBUFFERLEN			100

struct WinIOCP_OVERLAPPED
{
	OVERLAPPED					overlped;
	WSABUF						wbuf;
	NetAddr						addr;
	int 						addrlen;
	DWORD						dwBytes;
	DWORD						dwFlags;
	shared_ptr<Event>			event;
	weak_ptr<_PollResource>		resource;

	WinIOCP_OVERLAPPED()
	{
		memset(&overlped, 0, sizeof(overlped));
		memset(&wbuf, 0, sizeof(wbuf));

		dwBytes = dwFlags = 0;
		addr.reset();
		addrlen = addr.getAddrLen();
	}
	virtual ~WinIOCP_OVERLAPPED() {}
};

struct SocketDisconnectInfo
{
    weak_ptr<Socket>		sock;
	WinIOCP_OVERLAPPED*		overlapped = NULL;

    static void socketSendDisconnectCallback(void* param)
    {
        SocketDisconnectInfo* info = (SocketDisconnectInfo*)param;
        if (info != NULL)
        {
            shared_ptr<Socket> socket = info->sock.lock();
            if(socket) socket->socketError("disconnected");
        }
		SAFE_DELETE(info->overlapped);
        delete info;
    }
};

struct Proactor_SendEvent:public Event
{
	WinIOCP_OVERLAPPED*							overlapped;
	WSABUF*										wsbuf;
	uint32_t									wsbufsize;

	Socket::SendedCallback						sendcallback;
	Socket::SendedCallback1						sendcallback1;
	Socket::SendedCallback2						sendcallback2;
	Socket::SendedCallback3						sendcallback3;

	shared_ptr<Socket::SendBuffer>				send1;
	std::vector<Socket::SBuf>					send2;
	std::vector<shared_ptr<Socket::SendBuffer>>	send3;


	const char* sendaddr = NULL;
	uint32_t	sendlen = 0;
	

	Proactor_SendEvent(WinIOCP_OVERLAPPED* _overlapped):Event(EventType_Write),overlapped(_overlapped), wsbuf(NULL),wsbufsize(0)
	{
	}
	~Proactor_SendEvent()
	{
		if (wsbuf != NULL)
		{
			delete[] wsbuf;
			wsbuf = NULL;
			wsbufsize = 0;
		}
	}
	void init(const char* _sendbuf,uint32_t _sendlen, const Socket::SendedCallback& _callback, const NetAddr& toaddr)
	{
		sendaddr = _sendbuf;
		sendlen = _sendlen;

		sendcallback = _callback;

		initpost({ Socket::SBuf(_sendbuf,_sendlen) }, toaddr);
	}
	void init(const shared_ptr<Socket::SendBuffer>& _buffer, const Socket::SendedCallback1& _callback, const NetAddr& toaddr)
	{
		send1 = _buffer;
		sendcallback1 = _callback;
;

		initpost({ Socket::SBuf(_buffer->bufferaddr(),_buffer->bufferlen()) }, toaddr);
	}
	void init(const std::vector<Socket::SBuf>& _sendbuf, const Socket::SendedCallback2& _callback,const NetAddr& toaddr)
	{
		send2 = _sendbuf;
		sendcallback2 = _callback;

		initpost(_sendbuf, toaddr);
	}
	void init(const std::vector<shared_ptr<Socket::SendBuffer>>& _sendbuf, const Socket::SendedCallback3& _callback, const NetAddr& toaddr)
	{
		send3 = _sendbuf;
		sendcallback3 = _callback;

		std::vector<Socket::SBuf> sendbuf;
		for (size_t i = 0; i < _sendbuf.size(); i++)
		{
			sendbuf.push_back(Socket::SBuf(_sendbuf[i]->bufferaddr(), _sendbuf[i]->bufferlen()));
		}

		initpost(sendbuf, toaddr);
	}

	void initpost(const std::vector<Socket::SBuf>& _sendbuf, const NetAddr& toaddr)
	{
		overlapped->addr = toaddr;

		wsbufsize = (uint32_t)_sendbuf.size();
		wsbuf = new WSABUF[_sendbuf.size()];
		
		for(size_t i = 0; i < _sendbuf.size(); i++)
		{
			wsbuf[i].buf = (char*)_sendbuf[i].bufaddr;
			wsbuf[i].len = _sendbuf[i].buflen;
		}
	}
	
	bool postEvent(const shared_ptr<Socket>& sock)
	{
		Event::postEvent(sock);


		int ret = 0;
		if (NetAddr(overlapped->addr).getPort() == 0)
		{
			ret = ::WSASend(sock->getHandle(), wsbuf, wsbufsize, &overlapped->dwBytes, 0, &overlapped->overlped, NULL);
		}
		else
		{
			ret = ::WSASendTo(sock->getHandle(), wsbuf, wsbufsize, &overlapped->dwBytes, 0, overlapped->addr.getAddr(), overlapped->addrlen, &overlapped->overlped, NULL);
		}

		if (ret == SOCKET_ERROR)
		{
			int errorno = GetLastError();
			if (errorno != WSA_IO_PENDING)
			{
				/*
                GetLastError的返回值的含义
                (10050)-套接字操作遇到了一个已死 的网络。
                (10051)-向一个无法连接的网络尝试了一个套接字操作。
                (10052)-当该操作在进行中，由于保持活动的操作检测到一个 故障，该连接中断。
                (10053)-您的主机中的软件放弃了一个已建立的连接。
                (10054)-远程主机强迫关闭了一个现有的连接。
                */
				if (errorno >= 10050 && errorno <= 10054)
				{
                    shared_ptr<IOWorker> worker = sock->ioWorker();
                    if (worker)
                    {
                        //抛个事件处理断开
                        SocketDisconnectInfo* info = new SocketDisconnectInfo();
                        info->sock = sock;
						info->overlapped = overlapped;
                        if (!worker->postEvent(SocketDisconnectInfo::socketSendDisconnectCallback, info))
                        {
                            delete info;
                        }
                    }
				}
				else
				{
					SAFE_DELETE(overlapped);
				}
				return false;
			}
		}

		return true;
	}
	void callEvent1(const shared_ptr<Socket>& sock, int bytes, bool status)
	{
		if (sendcallback) sendcallback(sock, sendaddr, bytes);
		else if (sendcallback1) sendcallback1(sock, send1);
		else if (sendcallback2) sendcallback2(sock, send2);
		else if (sendcallback3) sendcallback3(sock, send3);
	}
};

struct Proactor_RecvEvent :public Event
{
	WinIOCP_OVERLAPPED*							overlapped;
	Socket::ReceivedCallback					recvcallback;
	Socket::RecvFromCallback					recvfromcallback;
	Socket::ReceivedCallback1					recvcallback1;
	Socket::RecvFromCallback1					recvfromcallback1;

	shared_ptr<Socket::RecvBuffer>				buffer;
	String										recvbuffer;

	Proactor_RecvEvent(WinIOCP_OVERLAPPED* _overlapped):Event(EventType_Read), overlapped(_overlapped){}
	~Proactor_RecvEvent()
	{ 
		
	}
	void init(const shared_ptr<Socket::RecvBuffer>& _buffer, const Socket::ReceivedCallback1& _recvcallback, const Socket::RecvFromCallback1& _recvfromcallback)
	{
		buffer = _buffer;
		recvcallback1 = _recvcallback;
		recvfromcallback1 = _recvfromcallback;

		init(buffer->bufferaddr(), buffer->bufferSize(), Socket::ReceivedCallback(), Socket::RecvFromCallback());
	}
	void init(char* buffer, uint32_t len, const Socket::ReceivedCallback& _recvcallback, const Socket::RecvFromCallback& _recvfromcallback)
	{
		if (buffer == NULL)
		{
			buffer = recvbuffer.alloc(len);
		}
		recvcallback = _recvcallback;
		recvfromcallback = _recvfromcallback;

		overlapped->wbuf.buf = (CHAR*)buffer;
		overlapped->wbuf.len = len;
	}

	bool postEvent(const shared_ptr<Socket>& sock)
	{
		Event::postEvent(sock);


		int ret = 0;
		if (recvcallback)
		{
			ret = WSARecv(sock->getHandle(), &overlapped->wbuf, 1, &overlapped->dwBytes, &overlapped->dwFlags, &overlapped->overlped, NULL);
		}
		else
		{
			ret = WSARecvFrom(sock->getHandle(), &overlapped->wbuf, 1, &overlapped->dwBytes, &overlapped->dwFlags, overlapped->addr.getAddr(), &overlapped->addrlen, &overlapped->overlped, NULL);
		}

		if (ret == SOCKET_ERROR)
		{
			int errorno = GetLastError();
			if (errorno != WSA_IO_PENDING)
			{
				SAFE_DELETE(overlapped);
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
	void callEvent1(const shared_ptr<Socket>& sock, int bytes, bool status)
	{
		if (!status && recvcallback && !socketIsAlive(sock))
		{
			sock->socketError("socket disconnected");
		}
		else if (bytes <= 0 && recvcallback && !socketIsAlive(sock))
		{
			sock->socketError("socket disconnected");
		}
		else  if(bytes > 0)
		{
			if (buffer != NULL)
			{
				if (recvcallback1) recvcallback1(sock, buffer, status ? bytes : 0);
				else recvfromcallback1(sock, buffer, status ? bytes : 0, overlapped->addr);
			}
			else
			{
				if (recvcallback) recvcallback(sock, (const char*)overlapped->wbuf.buf, status ? bytes : 0);
				else recvfromcallback(sock, (const char*)overlapped->wbuf.buf, status ? bytes : 0, overlapped->addr);
			}
		}
        else if (status && bytes <= 0)
        {
            sock->socketError("socket disconnected");
        }
	}
};

struct Proactor_AcceptEvent :public Event
{
	WinIOCP_OVERLAPPED*							overlapped;
	Socket::AcceptedCallback					acceptcallback;
	char										swapBuffer[SWAPBUFFERLEN];
	SOCKET										newsock = 0;

	Proactor_AcceptEvent(WinIOCP_OVERLAPPED* _overlapped) :Event(EventType_Read), overlapped(_overlapped){}
	void init(const Socket::AcceptedCallback& _acceptcallback)
	{		
		acceptcallback = _acceptcallback;
		overlapped->wbuf.buf = swapBuffer;
		overlapped->wbuf.len = SWAPBUFFERLEN;
	}
	bool postEvent(const shared_ptr<Socket>& sock)
	{
		Event::postEvent(sock);


		LPFN_ACCEPTEX	acceptExFunc;
		GUID acceptEX = WSAID_ACCEPTEX;
		DWORD bytes = 0;

		int ret = WSAIoctl(sock->getHandle(), SIO_GET_EXTENSION_FUNCTION_POINTER, &acceptEX, sizeof(acceptEX), &acceptExFunc, sizeof(acceptExFunc), &bytes, NULL, NULL);
		if (ret == SOCKET_ERROR)
		{
			assert(0);
		}

		newsock = (int)WSASocket(sock->inet() == INET_IPV4 ? AF_INET : AF_INET6, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (newsock == INVALID_SOCKET)
		{
			SAFE_DELETE(overlapped);

			return false;
		}

		if (false == acceptExFunc(sock->getHandle(), newsock, overlapped->wbuf.buf, 0,sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, NULL, (LPOVERLAPPED)&overlapped->overlped))
		{
			int errorno = GetLastError();
			if (errorno != WSA_IO_PENDING)
			{
				closesocket(newsock);

				SAFE_DELETE(overlapped);
				return false;
			}
		}

		return true;
	}
	void callEvent1(const shared_ptr<Socket>& sock, int bytes, bool status)
	{
		if (!status)
		{
			closesocket(newsock);

			sock->async_accept(acceptcallback);
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

			getAcceptAddExFunc(overlapped->wbuf.buf, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, (LPSOCKADDR*)&localAddr, &locallen, (LPSOCKADDR*)&clientAddr, &clientlen);

			NewSocketInfo newsocketinfo;
			newsocketinfo.inet = sock->inet();
			newsocketinfo.newsocket = newsock;
			newsocketinfo.otheraddr = NetAddr(*(SockAddr*)clientAddr);

			shared_ptr<Socket> newsock = TCPClient::create(sock->ioWorker(), newsocketinfo);
			acceptcallback(sock, newsock);
		}
	}
};

struct Proactor_ConnectEvent :public Event
{
	WinIOCP_OVERLAPPED*							overlapped;
	Socket::ConnectedCallback					connectcallback;
	
	Proactor_ConnectEvent(WinIOCP_OVERLAPPED* _overlapped) :Event(EventType_Write), overlapped(_overlapped){}
	
	void init(const NetAddr& toaddr, const Socket::ConnectedCallback& _connectcallback)
	{
		connectcallback = _connectcallback;

		overlapped->addr = toaddr;
	}
	bool postEvent(const shared_ptr<Socket>& sock)
	{
		Event::postEvent(sock);

		GUID connetEx = WSAID_CONNECTEX;
		DWORD bytes = 0;
		LPFN_CONNECTEX			connectExFunc;

		int ret = WSAIoctl(sock->getHandle(), SIO_GET_EXTENSION_FUNCTION_POINTER, &connetEx, sizeof(connetEx), &connectExFunc, sizeof(connectExFunc), &bytes, NULL, NULL);

		if (ret == SOCKET_ERROR)
		{
			assert(0);
		}

		if (false == connectExFunc(sock->getHandle(), overlapped->addr.getAddr(), overlapped->addrlen, NULL, 0,NULL, &overlapped->overlped))
		{
			int errorno = GetLastError();
			if (errorno != WSA_IO_PENDING)
			{
				SAFE_DELETE(overlapped);
				return false;
			}
		}

		return true;
	}
	void callEvent1(const shared_ptr<Socket>& sock, int bytes, bool status)
	{
        if (!status)
        {
            connectcallback(sock, false, "connected error");
        }
        else
		{
			sock->socketReady();
			connectcallback(sock, true, "connected success");
		}
	}
};

#endif