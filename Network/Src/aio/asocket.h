#pragma  once
#include "Base/Base.h"
#include "ioserver.h"
using namespace Public::Base;
class ASocket:public Socket
{
protected:
	ASocket(const shared_ptr<IOWorker>& _ioworker, const shared_ptr<IOServer>& _ioserver, const shared_ptr<Socket>& _sockptr, NetType _type)
		:ioworker(_ioworker), ioserver(_ioserver), socketptr(_sockptr), status(NetStatus_disconnected),  type(_type), ishavelisten(false)
	{
		userthread = make_shared<_UserThread>();
	}
	ASocket(const shared_ptr<IOWorker>& _ioworker, const shared_ptr<IOServer>& _ioserver, const shared_ptr<Socket>& _sockptr, const NewSocketInfo& newsock)
		:ioworker(_ioworker), ioserver(_ioserver), socketptr(_sockptr), status(NetStatus_connected),  type(NetType_TcpConnection), ishavelisten(false)
	{
		userthread = make_shared<_UserThread>();
		sock = newsock.newsocket;
		otheraddr = newsock.otheraddr;
#ifdef WIN32
		int flag = 1;
		setSocketOpt(IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
#endif
	}
public:
	static shared_ptr<ASocket> create(const shared_ptr<IOWorker>& _ioworker, const shared_ptr<IOServer>& _ioserver, const shared_ptr<Socket>& _sockptr, NetType _type);
	static shared_ptr<ASocket> create(const shared_ptr<IOWorker>& _ioworker, const shared_ptr<IOServer>& _ioserver, const shared_ptr<Socket>& _sockptr, const NewSocketInfo& newsock);
	
	virtual ~ASocket()
	{
		userthread->waitAllOtherCallbackThreadUsedEnd();
	}
	virtual bool disconnect()
	{
		userthread->quit();
		ioserver->destory(sock);

		return true;
	}
	
	virtual bool bind(const NetAddr& addr, bool reusedAddr = true) 
	{
		if (sock <= 0 || addr.getPort() == 0) return false;

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

	
	virtual bool getSocketBuffer(uint32_t& recvSize, uint32_t& sendSize) const 
	{
		int sendlen = sizeof(uint32_t), recvlen = sizeof(uint32_t);

		bool ret = getSocketOpt(SOL_SOCKET, SO_SNDBUF, (char*)&sendSize, &sendlen);
		ret |= getSocketOpt(SOL_SOCKET, SO_RCVBUF, (char*)&recvSize, &recvlen);

		return ret == 0;
	}

	
	virtual bool setSocketBuffer(uint32_t recvSize, uint32_t sendSize) 
	{
		int sendlen = sizeof(uint32_t), recvlen = sizeof(uint32_t);

		bool ret = setSocketOpt(SOL_SOCKET, SO_SNDBUF, (char*)&sendSize, sendlen);
		ret |= setSocketOpt(SOL_SOCKET, SO_RCVBUF, (char*)&recvSize, recvlen);

		return ret == 0;
	}

	
	virtual bool getSocketTimeout(uint32_t& recvTimeout, uint32_t& sendTimeout) const 
	{
		int sendlen = sizeof(sendTimeout), recvlen = sizeof(recvTimeout);

		bool ret = getSocketOpt(SOL_SOCKET, SO_SNDTIMEO, (char*)&sendTimeout, &sendlen);
		ret |= getSocketOpt(SOL_SOCKET, SO_RCVTIMEO, (char*)&recvTimeout, &recvlen);

		return ret == 0;
	}

	
	virtual bool setSocketTimeout(uint32_t recvTimeout, uint32_t sendTimeout) 
	{
		int sendlen = sizeof(sendTimeout), recvlen = sizeof(recvTimeout);

		bool ret = setSocketOpt(SOL_SOCKET, SO_SNDTIMEO, (char*)&sendTimeout, sendlen);
		ret |= setSocketOpt(SOL_SOCKET, SO_RCVTIMEO, (char*)&recvTimeout, recvlen);

		return ret == 0;
	}
		
	virtual shared_ptr<Socket> accept() 
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

		SOCKADDR_IN accept_addr;
		memset(&accept_addr, 0, sizeof(accept_addr));
		accept_addr.sin_family = AF_INET;
		socklen_t accept_addrlen = sizeof(accept_addr);

		int s_accept = ::accept(sock, (SOCKADDR *)&accept_addr, &accept_addrlen);
		if (s_accept <= 0)
		{
			return shared_ptr<Socket>();
		}

		NewSocketInfo* newsocketinfo = new NewSocketInfo;
		newsocketinfo->newsocket = s_accept;
		newsocketinfo->otheraddr = NetAddr(*(SockAddrIPv4*)&accept_addr);

		shared_ptr<Socket> newsock = TCPClient::create(ioworker, newsocketinfo);

		return newsock;
	}

	virtual bool connect(const NetAddr& addr) 
	{
		if (sock <= 0 || type != NetType_TcpClient || status == NetStatus_connected || addr.getPort() != 0)
		{
			return false;
		}
		
		if (::connect(sock, addr.getAddr(), addr.getAddrLen()) < 0)
		{
			return false;
		}

		otheraddr = addr;
		
		status = NetStatus_connected;

		return true;
	}
		
	virtual int recv(char *buf, uint32_t len) 
	{
		if (buf == NULL || len <= 0 || sock <= 0 || status != NetStatus_connected)
		{
			return false;
		}
		int recv_len = ::recv(sock, buf, len, 0);

		return recv_len;
	}

	
	virtual int send(const char * buf, uint32_t len) 
	{
		if (buf == NULL || len <= 0 || sock <= 0 || status != NetStatus_connected)
		{
			return false;
		}

		int send_len = ::send(sock, buf, len, 0);

		return send_len;
	}
		
	virtual int recvfrom(char *buf, uint32_t len, NetAddr& other) 
	{
		if (sock <= 0 || buf == NULL || len <= 0 || type != NetType_Udp)
		{
			return -1;
		}
		
		
		struct sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		socklen_t addrlen = sizeof(addr);

		int readlen = ::recvfrom(sock, buf, len, 0, (struct sockaddr*)&addr, &addrlen);
		
		if (readlen > 0) other = NetAddr(*(SockAddr*)&addr);

		return readlen; 
	}

	virtual int sendto(const char * buf, uint32_t len, const NetAddr& other) 
	{
		if (sock <= 0 || buf == NULL || len <= 0 || type != NetType_Udp)
		{
			return -1;
		}

		int sendlen = ::sendto(sock, buf, len, 0, other.getAddr(), other.getAddrLen());
		
		return sendlen; 
	}

	virtual void socketReady()
	{
		status = NetStatus_connected;
	}

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

		return ::setsockopt(sock, level, optname, (const char*)optval, optlen) >= 0;
	}
	virtual bool getSocketOpt(int level, int optname, void *optval, int *optlen) const 
	{
		if (optval == NULL || sock <= 0)
		{
			return false;
		}

		return ::getsockopt(sock, level, optname, (char*)optval, (socklen_t*)optlen) >= 0;
	}


	virtual bool async_accept(const AcceptedCallback& callback) { return false; }
	virtual bool async_connect(const NetAddr& addr, const ConnectedCallback& callback) { return false; }
	virtual bool async_recv(char *buf, uint32_t len, const ReceivedCallback& received) { return false; }
	virtual bool async_recv(const ReceivedCallback& received, int maxlen = 1024) { return false; }
	virtual bool async_send(const char * buf, uint32_t len, const SendedCallback& sended) { return false; }
	virtual bool async_recvfrom(char *buf, uint32_t len, const RecvFromCallback& received) { return false; }
	virtual bool async_recvfrom(const RecvFromCallback& received, int maxlen = 1024) { return false; }
	virtual bool async_sendto(const char * buf, uint32_t len, const NetAddr& other, const SendedCallback& sended) { return false; }
	virtual bool nonBlocking(bool nonblock) { return false; }
private:
	virtual bool creatSocket(NetType type) = 0;
protected:
	shared_ptr<IOWorker>	ioworker;
	shared_ptr<IOServer>	ioserver;
	weak_ptr<Socket>		socketptr;

	int						sock;

	shared_ptr<_UserThread> userthread;

	NetStatus			 status;
	NetType				 type;

	NetAddr				 myaddr;
	NetAddr				 otheraddr;

	bool				ishavelisten;
};