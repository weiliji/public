#pragma once
#include "_poll.h"

struct Reactor_SendEvent : public Event
{
	shared_ptr<Socket::SendBuffer> send1;
	std::vector<Socket::SBuf> send2;
	std::vector<shared_ptr<Socket::SendBuffer>> send3;
	Socket::SendedCallback sendcallback;
	Socket::SendedCallback1 sendcallback1;
	Socket::SendedCallback2 sendcallback2;
	Socket::SendedCallback3 sendcallback3;

	NetAddr sendtoaddr;

	const char *sendaddr = NULL;
	size_t sendlen = 0;

	size_t havesendlen = 0;
	size_t sendbufpos = 0;

	int realsendlen = 0;

	Reactor_SendEvent(const char *addr, uint32_t len, const Socket::SendedCallback &_callback, const NetAddr &toaddr) : Event(EventType_Write)
	{
		sendtoaddr = toaddr;

		sendaddr = addr;
		sendlen = len;
		sendcallback = _callback;
	}
	Reactor_SendEvent(const shared_ptr<Socket::SendBuffer> &sendb, const Socket::SendedCallback1 &_callback, const NetAddr &toaddr) : Event(EventType_Write)
	{
		sendtoaddr = toaddr;
		sendcallback1 = _callback;
		send1 = sendb;
	}
	Reactor_SendEvent(const std::vector<Socket::SBuf> &sbuf, const Socket::SendedCallback2 &_callback, const NetAddr &toaddr) : Event(EventType_Write)
	{
		sendtoaddr = toaddr;
		sendcallback2 = _callback;
		send2 = sbuf;
	}
	Reactor_SendEvent(const std::vector<shared_ptr<Socket::SendBuffer>> &sbuf, const Socket::SendedCallback3 &_callback, const NetAddr &toaddr) : Event(EventType_Write)
	{
		sendtoaddr = toaddr;
		sendcallback3 = _callback;
		send3 = sbuf;
	}
	~Reactor_SendEvent()
	{
	}
	bool doEventUdp0(const shared_ptr<Socket> &sock)
	{
		realsendlen = ::sendto(sock->getHandle(), sendaddr, (int)sendlen, 0, sendtoaddr.getAddr(), sendtoaddr.getAddrLen());

		return true;
	}
	bool doEventUdp1(const shared_ptr<Socket> &sock)
	{
		realsendlen = ::sendto(sock->getHandle(), send1->bufferaddr(), send1->bufferlen(), 0, sendtoaddr.getAddr(), sendtoaddr.getAddrLen());

		return true;
	}
	bool doEventUdp2(const shared_ptr<Socket> &sock)
	{
		uint32_t sendbufferlen = 0;
		for (size_t i = 0; i < send2.size(); i++)
		{
			sendbufferlen += send2[i].buflen;
		}

		String sendbuftmp;
		char *sendbuf = sendbuftmp.alloc(sendbufferlen);
		uint32_t havecopylen = 0;
		for (size_t i = 0; i < send2.size(); i++)
		{
			memcpy(sendbuf + havecopylen, send2[i].bufaddr, send2[i].buflen);
			havecopylen += send2[i].buflen;
		}

		realsendlen = ::sendto(sock->getHandle(), sendbuf, sendbufferlen, 0, sendtoaddr.getAddr(), sendtoaddr.getAddrLen());

		return true;
	}
	bool doEventUdp3(const shared_ptr<Socket> &sock)
	{
		uint32_t sendbufferlen = 0;
		for (size_t i = 0; i < send3.size(); i++)
		{
			sendbufferlen += send3[i]->bufferlen();
		}

		String sendbuftmp;
		char *sendbuf = sendbuftmp.alloc(sendbufferlen);
		uint32_t havecopylen = 0;
		for (size_t i = 0; i < send3.size(); i++)
		{
			memcpy(sendbuf + havecopylen, send3[i]->bufferaddr(), send3[i]->bufferlen());
			havecopylen += send3[i]->bufferlen();
		}

		realsendlen = ::sendto(sock->getHandle(), sendbuf, sendbufferlen, 0, sendtoaddr.getAddr(), sendtoaddr.getAddrLen());

		return true;
	}
	bool doEventTcp0(const shared_ptr<Socket> &sock)
	{
		const char *sendbufferaddr = sendaddr + realsendlen;
		uint32_t sendbufferlen = (uint32_t)sendlen - realsendlen;

		int sendlentmp = ::send(sock->getHandle(), sendbufferaddr, sendbufferlen, 0);
		if (sendlentmp < 0)
		{
			return false; //请外部继续
		}

		realsendlen += sendlentmp;

		if (realsendlen < (int)sendlen)
		{
			return false; //请外部继续
		}

		return true;
	}
	bool doEventTcp1(const shared_ptr<Socket> &sock)
	{
		const char *sendbufferaddr = send1->bufferaddr() + realsendlen;
		uint32_t sendbufferlen = send1->bufferlen() - realsendlen;

		int sendlentmp = ::send(sock->getHandle(), sendbufferaddr, sendbufferlen, 0);
		if (sendlentmp < 0)
		{
			return false; //请外部继续
		}

		realsendlen += sendlentmp;

		if (realsendlen < (int)send1->bufferlen())
		{
			return false; //请外部继续
		}

		return true;
	}
	bool doEventTcp2(const shared_ptr<Socket> &sock)
	{
		while (sendbufpos < send2.size())
		{
			const char *sendbufferaddr = send2[sendbufpos].bufaddr + havesendlen;
			uint32_t sendbufferlen = send2[sendbufpos].buflen - (uint32_t)havesendlen;

			int sendlen = ::send(sock->getHandle(), sendbufferaddr, sendbufferlen, 0);
			if (sendlen < 0)
			{
				return false; //请外部继续
			}

			realsendlen += sendlen;
			havesendlen += sendlen;

			if (sendlen < (int)sendbufferlen)
			{
				return false; //请外部继续
			}
			else
			{
				sendbufpos++;
				havesendlen = 0;
			}
		}

		return true;
	}

	bool doEventTcp3(const shared_ptr<Socket> &sock)
	{
		while (sendbufpos < send3.size())
		{
			const char *sendbufferaddr = send3[sendbufpos]->bufferaddr() + havesendlen;
			uint32_t sendbufferlen = send3[sendbufpos]->bufferlen() - (uint32_t)havesendlen;

			int sendlen = ::send(sock->getHandle(), sendbufferaddr, sendbufferlen, 0);
			if (sendlen < 0)
			{
				return false; //请外部继续
			}

			realsendlen += sendlen;
			havesendlen += sendlen;

			if (sendlen < (int)sendbufferlen)
			{
				return false; //请外部继续
			}
			else
			{
				sendbufpos++;
				havesendlen = 0;
			}
		}

		return true;
	}

	bool doEvent1(const shared_ptr<Socket> &sock)
	{
		NetType nettype = sock->getNetType();

		//udp need send sendbuf
		if (nettype == NetType_Udp)
		{
			if (sendaddr != NULL)
				return doEventUdp0(sock);
			else if (send1)
				return doEventUdp1(sock);
			else if (send2.size() > 0)
				return doEventUdp2(sock);
			else if (send3.size() > 0)
				return doEventUdp3(sock);
		}
		else if (nettype != NetType_Udp)
		{
			if (sendaddr != NULL)
				return doEventTcp0(sock);
			else if (send1)
				return doEventTcp1(sock);
			else if (send2.size() > 0)
				return doEventTcp2(sock);
			else if (send3.size() > 0)
				return doEventTcp3(sock);
		}

		return true;
	}
	virtual void callEvent1(const shared_ptr<Socket> &sock, int bytes, bool status)
	{
		if (sendcallback)
			sendcallback(sock, sendaddr, realsendlen);
		else if (sendcallback1)
			sendcallback1(sock, send1);
		else if (sendcallback2)
			sendcallback2(sock, send2);
		else if (sendcallback3)
			sendcallback3(sock, send3);
	}
};

struct Reactor_RecvEvent : public Event
{
	Socket::ReceivedCallback recvcallback;
	Socket::RecvFromCallback recvfromcallback;
	Socket::ReceivedCallback1 recvcallback1;
	Socket::RecvFromCallback1 recvfromcallback1;

	shared_ptr<Socket::RecvBuffer> buffer;

	char *recvBufferAddr = NULL;
	uint32_t recvBufferLen = 0;

	String recvBuffer;

	NetAddr recvaddr;
	int readlen = 0;

	Reactor_RecvEvent(const shared_ptr<Socket::RecvBuffer> &buf, const Socket::ReceivedCallback1 &_recvcallback, const Socket::RecvFromCallback1 &_recvfromcallback) : Event(EventType_Read)
	{
		buffer = buf;

		recvBufferAddr = buf->bufferaddr();
		recvBufferLen = buf->bufferSize();

		recvcallback1 = _recvcallback;
		recvfromcallback1 = _recvfromcallback;
	}
	Reactor_RecvEvent(char *buffer, uint32_t len, const Socket::ReceivedCallback &_recvcallback, const Socket::RecvFromCallback &_recvfromcallback) : Event(EventType_Read)
	{
		recvBufferLen = len;
		if (buffer == NULL)
		{
			recvBufferAddr = recvBuffer.alloc(len);
		}
		else
		{
			recvBufferAddr = buffer;
		}
		recvcallback = _recvcallback;
		recvfromcallback = _recvfromcallback;
	}
	~Reactor_RecvEvent()
	{
	}
	bool socketIsAlive(const shared_ptr<Socket> &sock)
	{
		int sendret = ::send(sock->getHandle(), "", 0, 0);

		return sendret != -1;
	}
	bool doEvent1(const shared_ptr<Socket> &sock)
	{
		if (recvcallback)
		{
			while (readlen < (int)recvBufferLen)
			{
				uint32_t canrecvlen = recvBufferLen - readlen;
				if (canrecvlen <= 0)
					break;

				int relen = ::recv(sock->getHandle(), recvBufferAddr + readlen, canrecvlen, 0);
				if (relen <= 0)
					break;

				readlen += relen;
			}
		}
		else
		{
			recvaddr = NetAddr(sock->inet());
			socklen_t addrlen = recvaddr.getAddrLen();

			readlen = ::recvfrom(sock->getHandle(), recvBufferAddr, recvBufferLen, 0, recvaddr.getAddr(), &addrlen);
			if (readlen <= 0)
				return false;
		}

		return true;
	}

	virtual void callEvent1(const shared_ptr<Socket> &sock, int bytes, bool status)
	{
		if (readlen <= 0 && recvcallback && !socketIsAlive(sock))
		{
			sock->socketError("socket disconnected");
		}
		else if (readlen > 0)
		{
			if (recvcallback1)
				recvcallback1(sock, buffer, readlen);
			else if (recvfromcallback1)
				recvfromcallback1(sock, buffer, readlen, recvaddr);
			else if (recvcallback)
				recvcallback(sock, (const char *)recvBufferAddr, readlen);
			else
				recvfromcallback(sock, (const char *)recvBufferAddr, readlen, recvaddr);
		}
	}
};

struct Reactor_AcceptEvent : public Event
{
	weak_ptr<IOWorker> ioworker;
	Socket::AcceptedCallback acceptcallback;
	shared_ptr<Socket> newsock;

	Reactor_AcceptEvent(const shared_ptr<IOWorker> &_ioworker, const Socket::AcceptedCallback &_acceptcallback) : Event(EventType_Read)
	{
		ioworker = _ioworker;
		acceptcallback = _acceptcallback;
	}
	bool doEvent1(const shared_ptr<Socket> &sock)
	{
		shared_ptr<IOWorker> worker = ioworker.lock();
		if (worker != NULL)
		{
			NetAddr addr(sock->inet());
			socklen_t addrlen = 0;

			SOCKET newsockfd = ::accept(sock->getHandle(), addr.getAddr(), &addrlen);
			if (newsockfd <= 0)
			{
				assert(0);
				return false;
			}

			NewSocketInfo newsocketinfo;
			newsocketinfo.inet = sock->inet();
			newsocketinfo.newsocket = newsockfd;
			newsocketinfo.otheraddr = addr;

			newsock = TCPClient::create(sock->ioWorker(), newsocketinfo);
		}

		return true;
	}

	virtual void callEvent1(const shared_ptr<Socket> &sock, int bytes, bool status)
	{
		acceptcallback(sock, newsock);
	}
};

struct Reactor_ConnectEvent : public Event
{
	Socket::ConnectedCallback connectcallback;
	NetAddr toaddr;
	bool connectret = false;

	Reactor_ConnectEvent(const NetAddr &addr, const Socket::ConnectedCallback &_connectcallback) : Event(EventType_Write)
	{
		toaddr = addr;
		connectcallback = _connectcallback;
	}
	virtual bool postEvent(const shared_ptr<Socket> &_sock)
	{
		int ret = connect(_sock->getHandle(), toaddr.getAddr(), toaddr.getAddrLen());
		if (ret >= 0)
		{
			connectcallback(_sock, ret > 0, ret > 0 ? "success" : "connect error");

			return false;
		}

		return Event::postEvent(_sock);
	}
	bool doEvent1(const shared_ptr<Socket> &sock)
	{
		int error = 0;
		socklen_t len = sizeof(error);
		int ret = getsockopt(sock->getHandle(), SOL_SOCKET, SO_ERROR, (char *)&error, &len);
		if (ret >= 0 && error == 0)
		{
			sock->socketReady();
			connectret = true;
		}

		return true;
	}
	virtual void callEvent1(const shared_ptr<Socket> &sock, int bytes, bool status)
	{
		connectcallback(sock, connectret, connectret ? "success" : "connect error");
	}
};
