#pragma once
#include "_poll.h"

class ASocket : public Socket
{
protected:
	ASocket(const shared_ptr<IOWorker> &_ioworker, const shared_ptr<_Poll> &_poll, NetType _type, InetType _inet, SOCKET _sock = INVALIDHANDLE, const NetAddr &_otheraddr = NetAddr())
		: Socket(_ioworker), poll(_poll), sock(_sock), _famlily(_inet), status(NetStatus_disconnected), type(_type), otheraddr(_otheraddr), ishavelisten(false)
	{
		if (_sock != (SOCKET)INVALIDHANDLE)
		{
			status = NetStatus_connected;
		}
	}

public:
	static shared_ptr<ASocket> create(const shared_ptr<IOWorker> &_ioworker, const shared_ptr<_Poll> &_poll, InetType inet, NetType _type, SOCKET _sock = INVALIDHANDLE, const NetAddr &_otheraddr = NetAddr());
	static shared_ptr<_Poll> createPoll(uint32_t threadnum, Thread::Priority pri);

	virtual ~ASocket()
	{
	}
	virtual bool disconnect()
	{
		status = NetStatus_disconnected;

        shared_ptr<_PollResource> tmp = resourece;
		if (tmp)
            tmp->quit();

		resourece = NULL;

		return true;
	}

	virtual bool bind(const NetAddr &addr, bool reusedAddr = true)
	{
		if (sock <= 0 || addr.getPort() == 0)
			return false;

		if (reusedAddr)
		{
			int reuseaddr = 1;
			setSocketOpt(SOL_SOCKET, SO_REUSEADDR, (const char *)&reuseaddr, sizeof(int));
		}

		int bindret = ::bind(sock, addr.getAddr(), addr.getAddrLen());

		if (bindret < 0)
		{
			return false;
		}

		myaddr = addr;

		return true;
	}

	virtual bool getSocketBuffer(uint32_t &recvSize, uint32_t &sendSize) const
	{
		int sendlen = sizeof(uint32_t), recvlen = sizeof(uint32_t);

		bool ret = getSocketOpt(SOL_SOCKET, SO_SNDBUF, (char *)&sendSize, &sendlen);
		ret |= getSocketOpt(SOL_SOCKET, SO_RCVBUF, (char *)&recvSize, &recvlen);

		return ret == 0;
	}

	virtual bool setSocketBuffer(uint32_t recvSize, uint32_t sendSize)
	{
		int sendlen = sizeof(uint32_t), recvlen = sizeof(uint32_t);

		bool ret = setSocketOpt(SOL_SOCKET, SO_SNDBUF, (char *)&sendSize, sendlen);
		ret |= setSocketOpt(SOL_SOCKET, SO_RCVBUF, (char *)&recvSize, recvlen);

		return ret == 0;
	}

	virtual shared_ptr<Socket> accept(uint32_t timeoutms)
	{
		if (sock <= 0 || type != NetType_TcpServer)
		{
			return shared_ptr<Socket>();
		}

		if (type == NetType_TcpServer && !ishavelisten)
		{
			int ret = listen(sock, SOMAXCONN);
			if (ret <= 0)
			{
				return shared_ptr<Socket>();
			}

			ishavelisten = true;
		}

		uint64_t startTime = Time::getCurrentMilliSecond();

		while (Time::getCurrentMilliSecond() - startTime < timeoutms)
		{
			NetAddr accept_addr(inet());

			socklen_t accept_addrlen = accept_addr.getAddrLen();
			int s_accept = (int)::accept(sock, (SOCKADDR *)accept_addr.getAddr(), &accept_addrlen);
			if (s_accept <= 0)
			{
				Thread::sleep(10);
				continue;
			}

			NewSocketInfo newsocketinfo;
			newsocketinfo.newsocket = s_accept;
			newsocketinfo.otheraddr = NetAddr(*(SockAddrIPv4 *)&accept_addr);

			shared_ptr<Socket> newsock = TCPClient::create(worker, newsocketinfo);

			return newsock;
		}

		return shared_ptr<Socket>();
	}

	virtual bool connect(const NetAddr &addr, uint32_t timeoutms)
	{
		if (status == NetStatus_connected)
			return true;

		if (sock <= 0 || type == NetType_TcpConnection || addr.getPort() == 0)
		{
			return false;
		}

		uint64_t startTime = Time::getCurrentMilliSecond();

		while (status != NetStatus_connected && Time::getCurrentMilliSecond() - startTime < timeoutms)
		{
			if (::connect(sock, addr.getAddr(), addr.getAddrLen()) < 0)
			{
				Thread::sleep(10);
				continue;
			}

			otheraddr = addr;

			status = NetStatus_connected;
		}

		return status == NetStatus_connected;
	}

	virtual int recv(char *buf, uint32_t len, uint32_t timeoutms)
	{
		if (buf == NULL || len <= 0 || sock <= 0 || status != NetStatus_connected)
		{
			return false;
		}
		uint64_t startTime = Time::getCurrentMilliSecond();

		while (Time::getCurrentMilliSecond() - startTime < timeoutms)
		{
			int recv_len = ::recv(sock, buf, len, 0);
			if (recv_len <= 0)
			{
				Thread::sleep(10);
				continue;
			}

			return recv_len;
		}

		return 0;
	}

	virtual int send(const char *buf, uint32_t len, uint32_t timeoutms)
	{
		if (buf == NULL || len <= 0 || sock <= 0 || status != NetStatus_connected)
		{
			return false;
		}

		uint64_t startTime = Time::getCurrentMilliSecond();
		uint32_t haveSendLen = 0;
		while (Time::getCurrentMilliSecond() - startTime < timeoutms && haveSendLen < len)
		{
			int send_len = ::send(sock, buf + haveSendLen, len - haveSendLen, 0);
			if (send_len <= 0)
			{
				Thread::sleep(10);
				continue;
			}

			haveSendLen += send_len;
		}

		return haveSendLen;
	}

	virtual int recvfrom(char *buf, uint32_t len, NetAddr &other, uint32_t timeoutms)
	{
		if (sock <= 0 || buf == NULL || len <= 0 || type != NetType_Udp)
		{
			return -1;
		}

		uint64_t startTime = Time::getCurrentMilliSecond();

		while (Time::getCurrentMilliSecond() - startTime < timeoutms)
		{
			NetAddr addr(inet());
			socklen_t addrlen = addr.getAddrLen();

			int readlen = ::recvfrom(sock, buf, len, 0, (struct sockaddr *)addr.getAddr(), &addrlen);
			if (readlen <= 0)
			{
				Thread::sleep(10);
				continue;
			}

			return readlen;
		}

		return 0;
	}

	virtual int sendto(const char *buf, uint32_t len, const NetAddr &other, uint32_t timeoutms)
	{
		if (sock <= 0 || buf == NULL || len <= 0 || type != NetType_Udp)
		{
			return -1;
		}

		uint64_t startTime = Time::getCurrentMilliSecond();
		while (Time::getCurrentMilliSecond() - startTime < timeoutms)
		{
			int sendlen = ::sendto(sock, buf, len, 0, other.getAddr(), other.getAddrLen());
			if (sendlen <= 0)
			{
				Thread::sleep(10);
				continue;
			}

			return sendlen;
		}

		return 0;
	}

	virtual void socketReady()
	{
		status = NetStatus_connected;
	}

	virtual InetType inet() const { return _famlily; }
	virtual SOCKET getHandle() const { return sock; }
	virtual NetStatus getStatus() const { return status; }
	virtual NetType getNetType() const { return type; }
	virtual NetAddr getMyAddr() const { return myaddr; }
	virtual NetAddr getOtherAddr() const { return otheraddr; }

	virtual bool setSocketOpt(int level, int optname, const void *optval, int optlen)
	{
		if (optval == NULL || sock <= 0)
		{
			return false;
		}

		return ::setsockopt(sock, level, optname, (const char *)optval, optlen) >= 0;
	}
	virtual bool getSocketOpt(int level, int optname, void *optval, int *optlen) const
	{
		if (optval == NULL || sock <= 0)
		{
			return false;
		}

		return ::getsockopt(sock, level, optname, (char *)optval, (socklen_t *)optlen) >= 0;
	}

	bool setDisconnectCallback(const Socket::DisconnectedCallback &disconnected)
	{
		disconnectcallback = disconnected;

		return true;
	}

    virtual void socketError(const std::string &errmsg)
	{
        NetStatus statustmp = status;
        status = NetStatus_disconnected;
        if (statustmp == NetStatus_connected)
        {
            disconnectcallback(socketptr.lock(), "disconnected");
        }
	}

private:
	virtual bool creatSocket(NetType type) = 0;

protected:
	shared_ptr<_Poll> poll;
	weak_ptr<Socket> socketptr;

	shared_ptr<_PollResource> resourece;

	SOCKET sock;

	InetType _famlily;

	NetStatus status;
	NetType type;

	NetAddr myaddr;
	NetAddr otheraddr;

	bool ishavelisten;

	Socket::DisconnectedCallback disconnectcallback;
};