#pragma  once

#ifndef WIN32

#include "../common/_pool.h"

struct LinuxEvent
{
	char*			bufferaddr;
	uint32_t		bufferlen;
	SOCKADDR		addr;
	int 			addrlen;
	
	LinuxEvent()
	{
		memset(&addr, 0, sizeof(addr));
		bufferaddr = NULL;
		bufferlen = 0;

		addr.sa_family = AF_INET;
		addrlen = sizeof(SOCKADDR);
	}
	virtual ~LinuxEvent() {}
};

struct SendEvent :public Event,public LinuxEvent
{
	Socket::SendedCallback sendcallback;

	SendEvent(const char* buffer, uint32_t len, const Socket::SendedCallback& _callback, const NetAddr& toaddr):Event(EventType_Write)
	{
		addr = *(SOCKADDR*)toaddr.getAddr();
		addrlen = toaddr.getAddrLen();
		sendcallback = _callback;
		bufferaddr = (char*)buffer;
		bufferlen = len;
	}
	~SendEvent() {}
	void doEvent(const shared_ptr<Socket>& sock, int bytes, bool status)
	{
		uint32_t sendlen = 0;

		if (NetAddr(addr).getPort() == 0)
		{
			sendlen = ::send(sock->getHandle(), bufferaddr, bufferlen, 0);
		}
		else
		{
			sendlen = ::sendto(sock->getHandle(), bufferaddr,bufferlen, 0, &addr, addrlen);
		}


		sendcallback(sock, (const char*)bufferaddr, sendlen);
	}
};

struct RecvEvent :public Event,public LinuxEvent
{
	Socket::ReceivedCallback recvcallback;
	Socket::RecvFromCallback recvfromcallback;

	bool needFreeBuffer = false;
	RecvEvent(char* buffer, uint32_t len, const Socket::ReceivedCallback& _recvcallback, const Socket::RecvFromCallback& _recvfromcallback):Event(EventType_Read)
	{
		if (buffer == NULL)
		{
			needFreeBuffer = true;
			buffer = new char[len];
		}
		recvcallback = _recvcallback;
		recvfromcallback = _recvfromcallback;

		bufferaddr = (char*)buffer;
		bufferlen = len;
	}
	~RecvEvent() 
	{ 
		if (needFreeBuffer) 
			delete[]bufferaddr;
	}
	bool socketIsAlive(const shared_ptr<Socket>& sock)
	{
		int sendret = ::send(sock->getHandle(), "", 0, 0);

		return sendret != -1;
	}
	void doEvent(const shared_ptr<Socket>& sock, int bytes, bool status)
	{
		int readlen = 0;

		if (recvcallback)
		{
			readlen = ::recv(sock->getHandle(), bufferaddr, bufferlen, 0);
		}
		else
		{
			readlen = ::recvfrom(sock->getHandle(), bufferaddr, bufferlen, 0, &addr, &addrlen);
		}


		if (readlen <= 0 /*&& !socketIsAlive(sock)*/)
		{
			sock->socketError("socket disconnected");
		}
		else
		{
			if (recvcallback) recvcallback(sock, (const char*)bufferaddr, readlen);
			else recvfromcallback(sock, (const char*)bufferaddr, readlen, NetAddr(*(SockAddr*)&addr));
		}
	}
};

struct AcceptEvent :public Event,public LinuxEvent
{
	weak_ptr<IOWorker>		 ioworker;
	Socket::AcceptedCallback acceptcallback;

	AcceptEvent(const shared_ptr<IOWorker>& _ioworker, const Socket::AcceptedCallback& _acceptcallback) :Event(EventType_Read)
	{		
		ioworker = _ioworker;
		acceptcallback = _acceptcallback;
	}
	void doEvent(const shared_ptr<Socket>& sock, int bytes, bool status)
	{
		shared_ptr<IOWorker> worker = ioworker.lock();
		if (worker == NULL) return;

		int newsockfd = ::accept(sock->getHandle(), &addr, &addrlen);
		if (newsockfd <= 0)
		{
			assert(0);
		}

		NewSocketInfo* newsocketinfo = new NewSocketInfo;
		newsocketinfo->newsocket = newsockfd;
		newsocketinfo->otheraddr = NetAddr(*(SockAddr*)addr);

		shared_ptr<Socket> newsock = TCPClient::create(worker, newsocketinfo);
		acceptcallback(sock, newsock);
	}
};

struct ConnectEvent :public Event,public LinuxEvent
{
	Socket::ConnectedCallback connectcallback;
	
	
	ConnectEvent(const NetAddr& toaddr, const Socket::ConnectedCallback& _connectcallback) :Event(EventType_Write)
	{
		addr = *(SOCKADDR*)toaddr.getAddr();
		addrlen = toaddr.getAddrLen();
	}
	void doEvent(const shared_ptr<Socket>& sock, int bytes, bool status)
	{
		sock->nonBlocking(false);
		sock->setSocketTimeout(1000, 1000);

		int ret = ::connect(sock->getHandle(), &addr, addrlen);

		sock->nonBlocking(true);

		connectcallback(sock, ret > 0, ret > 0 ? "success", "connect error");

		return true;
	}
};

#endif