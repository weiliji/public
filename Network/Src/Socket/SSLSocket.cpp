#include "Network/Socket/SSLSocket.h"
#include "openssl/ssl.h"

namespace Public
{
namespace Network
{

#ifdef SUPPORT_OPENSSL

static bool sslinitflag = false;
static Mutex sslbiomutex;

enum
{
	BIO_WRITE = 0,
	BIO_READ = 1,
	BIO_MAX = 2,

	SSL_WRITE = BIO_READ,
	SSL_READ = BIO_WRITE,
};

#define MAXTCPREADSIZE 1024 * 128

struct BIOSendBuffer : public Socket::SendBuffer
{
	String biosendbuffer;

	virtual const char *bufferaddr() { return biosendbuffer.c_str(); }
	virtual uint32_t bufferlen() { return biosendbuffer.length(); }
};

struct SocketSendInfo
{
	shared_ptr<Socket::SendBuffer> socketSend1;
	std::vector<Socket::SBuf> socketSend2;
	std::vector<shared_ptr<Socket::SendBuffer>> socketSend3;

	const char *socketSendAddr = NULL;
	int socketSendLen = 0;

	Socket::SendedCallback socketSendCallback;
	Socket::SendedCallback1 socketSendCallback1;
	Socket::SendedCallback2 socketSendCallback2;
	Socket::SendedCallback3 socketSendCallback3;

	std::vector<shared_ptr<Socket::SendBuffer>> biosendbuffer;

	uint64_t startTime = Time::getCurrentMilliSecond();
	uint32_t timeout = INFINITE;
};

struct SocketRecvInfo
{
	shared_ptr<Socket::RecvBuffer> socketRecvBuffer;
	String socketRecvString;
	Socket::ReceivedCallback1 socketRecvCallback1;
	Socket::ReceivedCallback socketRecvCallback;

	char *sslReadAddr;
	int sslReadLen;
	int sslHaveReadLen = 0;
	uint64_t startTime = Time::getCurrentMilliSecond();
	uint32_t timeout = INFINITE;
};

struct SSLSocketImpl : public Socket
{
	weak_ptr<Socket> sslsocket;
	shared_ptr<Socket> socket;

	BIO *bio[BIO_MAX] = {
		NULL,
	};

	SSL_CTX *sslctx = NULL;
	SSL *ssl = NULL;

	bool sslConnected = false;

	bool socketRecving = false;

	Mutex mutex;
	std::list<shared_ptr<SocketSendInfo>> sendList;
	std::list<shared_ptr<SocketRecvInfo>> recvList;

	SSLSocketImpl(const shared_ptr<Socket> &sock) : Socket(sock->ioWorker()), sslsocket(sock)
	{
		Guard locker(sslbiomutex);
		if (!sslinitflag)
		{
			SSL_library_init();
			sslinitflag = true;
		}
	}

	~SSLSocketImpl()
	{
		disconnect();

		socket = NULL;
	}
	bool open(const shared_ptr<Socket> &sock, bool isserver, const std::string &cert, const std::string &key, uint32_t timeout)
	{
		socket = sock;

		bio[BIO_WRITE] = BIO_new(BIO_s_mem());
		bio[BIO_READ] = BIO_new(BIO_s_mem());

		if (bio[BIO_WRITE] == NULL || bio[BIO_READ] == NULL)
		{
			return false;
		}

		const SSL_METHOD *method = SSLv23_method();

		sslctx = SSL_CTX_new(method);
		if (sslctx == NULL)
		{
			return false;
		}

		if (isserver)
		{
			int r = SSL_CTX_use_certificate_file(sslctx, cert.c_str(), SSL_FILETYPE_PEM);
			if (r <= 0)
				return false;

			r = SSL_CTX_use_PrivateKey_file(sslctx, key.c_str(), SSL_FILETYPE_PEM);
			if (r <= 0)
				return false;

			r = SSL_CTX_check_private_key(sslctx);
			if (r <= 0)
				return false;
		}

		ssl = SSL_new(sslctx);
		if (ssl == NULL)
		{
			return false;
		}

		SSL_set_bio(ssl, bio[SSL_READ], bio[SSL_WRITE]);

		socket->async_recv(Socket::ReceivedCallback(&SSLSocketImpl::onSocketRecvCallback, this), MAXTCPREADSIZE);
		{
			Guard locker(mutex);
			socketRecving = true;
		}
		if (isserver)
		{
			SSL_set_accept_state(ssl);
		}
		else
		{
			SSL_set_connect_state(ssl);
		}

		uint64_t startTime = Time::getCurrentMilliSecond();

		while (!sslConnected && Time::getCurrentMilliSecond() - startTime < timeout)
		{
			poolBioReadAndSend();

			doCheckSSLIsConnected();
			if (sslConnected)
				break;

			Thread::sleep(10);
		}

		return sslConnected;
	}
	bool disconnect()
	{
		if (socket)
			socket->disconnect();
		socket = NULL;

		if (ssl != NULL)
		{
			SSL_shutdown(ssl);
			SSL_free(ssl);
			ssl = NULL;
		}
		if (sslctx != NULL)
			SSL_CTX_free(sslctx);
		sslctx = NULL;

		//ssl_free 要释放bio，所以不用在释放
		//if (bio[BIO_WRITE] != NULL) BIO_free(bio[BIO_WRITE]);
		//if (bio[BIO_READ] != NULL) BIO_free(bio[BIO_READ]);

		bio[BIO_WRITE] = bio[BIO_READ] = NULL;

		return true;
	}

	bool async_recv(char *buf, uint32_t len, const Socket::ReceivedCallback &received, uint32_t timeoutms)
	{
		if (buf == NULL || len <= 0 || !received)
		{
			return false;
		}
		{
			shared_ptr<SocketRecvInfo> recvInfo = make_shared<SocketRecvInfo>();
			recvInfo->sslReadAddr = buf;
			recvInfo->sslReadLen = len;
			recvInfo->socketRecvCallback = received;
			recvInfo->timeout = timeoutms;

			Guard locker(mutex);
			recvList.push_back(recvInfo);
		}

		doBioReadAndPostRecv();

		return true;
	}
	bool async_recv(const shared_ptr<Socket::RecvBuffer> &buffer, const Socket::ReceivedCallback1 &received, uint32_t timeoutms)
	{
		if (buffer == NULL || buffer->bufferaddr() == NULL || buffer->bufferSize() <= 0 || !received)
		{
			return false;
		}
		{
			shared_ptr<SocketRecvInfo> recvInfo = make_shared<SocketRecvInfo>();
			recvInfo->socketRecvBuffer = buffer;
			recvInfo->sslReadAddr = buffer->bufferaddr();
			recvInfo->sslReadLen = buffer->bufferSize();
			recvInfo->socketRecvCallback1 = received;
			recvInfo->timeout = timeoutms;

			Guard locker(mutex);
			recvList.push_back(recvInfo);
		}

		doBioReadAndPostRecv();

		return true;
	}
	bool async_recv(const Socket::ReceivedCallback &received, int maxlen, uint32_t timeoutms)
	{
		if (maxlen <= 0 || !received)
		{
			return false;
		}
		{
			shared_ptr<SocketRecvInfo> recvInfo = make_shared<SocketRecvInfo>();
			recvInfo->sslReadLen = maxlen;
			recvInfo->socketRecvString.alloc(maxlen);
			recvInfo->sslReadAddr = recvInfo->socketRecvString.c_str();
			recvInfo->socketRecvCallback = received;
			recvInfo->timeout = timeoutms;

			Guard locker(mutex);
			recvList.push_back(recvInfo);
		}

		doBioReadAndPostRecv();

		return true;
	}
	int recv(char *buf, uint32_t len, uint32_t timeoutms)
	{
		shared_ptr<Socket> sock = socket;
		if (buf == NULL || len <= 0 || sock == NULL)
		{
			return 0;
		}

		int readlen = 0;

		uint64_t starttime = Time::getCurrentMilliSecond();
		while (readlen <= 0 && Time::getCurrentMilliSecond() - starttime < timeoutms)
		{
			readlen = sock->recv(buf, len);
			if (readlen <= 0)
				return 0;

			int writelen = BIO_write(bio[BIO_WRITE], buf, readlen);
			assert(writelen == len);
			(void)writelen;

			readlen = SSL_read(ssl, buf, len);
		}

		return readlen;
	}

	bool async_send(const char *buf, uint32_t len, const Socket::SendedCallback &sended, uint32_t timeoutms)
	{
		if (buf == NULL || len <= 0 || !sended)
			return false;

		shared_ptr<SocketSendInfo> sendinfo = make_shared<SocketSendInfo>();
		sendinfo->socketSendAddr = buf;
		sendinfo->socketSendLen = len;
		sendinfo->socketSendCallback = sended;
		sendinfo->timeout = timeoutms;

		int writelen = SSL_write(ssl, buf, len);
		assert(writelen == len);
		(void)writelen;

		while (poolBioRead(sendinfo))
		{
		}

		sendBioBuffer(sendinfo);

		return true;
	}
	bool async_send(const shared_ptr<Socket::SendBuffer> &buffer, const Socket::SendedCallback1 &sended, uint32_t timeoutms)
	{
		if (buffer == NULL)
			return false;

		shared_ptr<SocketSendInfo> sendinfo = make_shared<SocketSendInfo>();
		sendinfo->socketSend1 = buffer;
		sendinfo->socketSendAddr = buffer->bufferaddr();
		sendinfo->socketSendLen = buffer->bufferlen();
		sendinfo->socketSendCallback1 = sended;
		sendinfo->timeout = timeoutms;

		int writelen = SSL_write(ssl, sendinfo->socketSendAddr, sendinfo->socketSendLen);
		assert(writelen == sendinfo->socketSendLen);
		(void)writelen;

		while (poolBioRead(sendinfo))
		{
		}

		sendBioBuffer(sendinfo);

		return true;
	}
	bool async_send(const std::vector<Socket::SBuf> &sendbuf, const Socket::SendedCallback2 &sended, uint32_t timeoutms)
	{
		if (sendbuf.size() <= 0 || !sended)
			return false;
		shared_ptr<SocketSendInfo> sendinfo = make_shared<SocketSendInfo>();
		sendinfo->socketSend2 = sendbuf;
		sendinfo->socketSendCallback2 = sended;
		sendinfo->timeout = timeoutms;

		for (std::vector<Socket::SBuf>::const_iterator iter = sendbuf.begin(); iter != sendbuf.end(); iter++)
		{
			int writelen = SSL_write(ssl, iter->bufaddr, iter->buflen);
			assert(writelen == iter->buflen);
			(void)writelen;

			while (poolBioRead(sendinfo))
			{
			}
		}

		sendBioBuffer(sendinfo);

		return true;
	}
	bool async_send(const std::vector<shared_ptr<Socket::SendBuffer>> &sendbuf, const Socket::SendedCallback3 &sended, uint32_t timeoutms)
	{
		if (sendbuf.size() <= 0)
			return false;
		shared_ptr<SocketSendInfo> sendinfo = make_shared<SocketSendInfo>();
		sendinfo->socketSend3 = sendbuf;
		sendinfo->socketSendCallback3 = sended;
		sendinfo->timeout = timeoutms;

		for (std::vector<shared_ptr<Socket::SendBuffer>>::const_iterator iter = sendbuf.begin(); iter != sendbuf.end(); iter++)
		{
			int writelen = SSL_write(ssl, (*iter)->bufferaddr(), (*iter)->bufferlen());
			assert(writelen == (*iter)->bufferlen());
			(void)writelen;

			while (poolBioRead(sendinfo))
			{
			}
		}

		sendBioBuffer(sendinfo);

		return true;
	}

	int send(const char *buf, uint32_t len, uint32_t timeoutms)
	{
		shared_ptr<Socket> sock = socket;

		if (buf == NULL || len <= 0 || sock == NULL)
			return 0;

		int writelen = SSL_write(ssl, buf, len);
		assert(writelen == len);
		(void)writelen;

		String bioSendString;
		bioSendString.alloc(MAXTCPREADSIZE);

		while (1)
		{
			int readlen = BIO_read(bio[BIO_READ], bioSendString.c_str(), MAXTCPREADSIZE);
			if (readlen > 0)
			{
				writelen = sock->send(bioSendString.c_str(), readlen, timeoutms);

				assert(writelen == readlen);
			}
			else
			{
				break;
			}
		}
		return len;
	}

	bool getSocketBuffer(uint32_t &recvSize, uint32_t &sendSize) const
	{
		shared_ptr<Socket> sock = socket;
		if (sock == NULL)
			return false;

		return sock->getSocketBuffer(recvSize, sendSize);
	}
	bool setSocketBuffer(uint32_t recvSize, uint32_t sendSize)
	{
		shared_ptr<Socket> sock = socket;
		if (sock == NULL)
			return false;

		return sock->setSocketBuffer(recvSize, sendSize);
	}

	bool setDisconnectCallback(const Socket::DisconnectedCallback &disconnected)
	{
		shared_ptr<Socket> sock = socket;
		if (sock == NULL)
			return false;

		return sock->setDisconnectCallback(disconnected);
	}

	InetType inet() const
	{
		shared_ptr<Socket> sock = socket;
		if (sock == NULL)
			return INET_IPV4;

		return sock->inet();
	}
	SOCKET getHandle() const
	{
		shared_ptr<Socket> sock = socket;
		if (sock == NULL)
			return INVALIDHANDLE;

		return sock->getHandle();
	}
	NetStatus getStatus() const
	{
		shared_ptr<Socket> sock = socket;
		if (sock == NULL)
			return NetStatus_disconnected;

		return sock->getStatus();
	}
	NetType getNetType() const
	{
		shared_ptr<Socket> sock = socket;
		if (sock == NULL)
			return NetType_TcpConnection;

		return sock->getNetType();
	}
	NetAddr getMyAddr() const
	{
		shared_ptr<Socket> sock = socket;
		if (sock == NULL)
			return NetAddr();

		return sock->getMyAddr();
	}
	NetAddr getOtherAddr() const
	{
		shared_ptr<Socket> sock = socket;
		if (sock == NULL)
			return NetAddr();

		return sock->getOtherAddr();
	}

	bool setSocketOpt(int level, int optname, const void *optval, int optlen)
	{
		shared_ptr<Socket> sock = socket;
		if (sock == NULL)
			return false;

		return sock->setSocketOpt(level, optname, optval, optlen);
	}
	bool getSocketOpt(int level, int optname, void *optval, int *optlen) const
	{
		shared_ptr<Socket> sock = socket;
		if (sock == NULL)
			return false;

		return sock->getSocketOpt(level, optname, optval, optlen);
	}

private:
	void doBioReadAndPostRecv()
	{
		while (1)
		{
			shared_ptr<SocketRecvInfo> recvInfo;
			{
				Guard locker(mutex);
				if (recvList.size() <= 0)
					return;
				recvInfo = recvList.front();

				recvInfo->sslHaveReadLen = SSL_read(ssl, recvInfo->sslReadAddr, recvInfo->sslReadLen);
				if (recvInfo->sslHaveReadLen > 0)
				{
					recvList.pop_front();
				}
				else
				{
					recvInfo = NULL;
				}
			}

			if (recvInfo != NULL)
			{
				if (recvInfo->socketRecvCallback)
					recvInfo->socketRecvCallback(sslsocket.lock(), recvInfo->sslReadAddr, recvInfo->sslHaveReadLen);
				else
					recvInfo->socketRecvCallback1(sslsocket.lock(), recvInfo->socketRecvBuffer, recvInfo->sslHaveReadLen);
			}
			else
			{
				Guard locker(mutex);
				if (recvList.size() <= 0 || socketRecving)
					break;

				recvInfo = recvList.front();
				socket->async_recv(Socket::ReceivedCallback(&SSLSocketImpl::onSocketRecvCallback, this), recvInfo->sslReadLen, recvInfo->timeout);
				socketRecving = true;

				break;
			}
		}
	}

	bool poolBioRead(const shared_ptr<SocketSendInfo> &sendinfo)
	{
		//shared_ptr< BIOSendBuffer>

		shared_ptr<BIOSendBuffer> sendbuf = make_shared<BIOSendBuffer>();
		char *sendbufaddr = sendbuf->biosendbuffer.alloc(MAXTCPREADSIZE);

		int readlen = BIO_read(bio[BIO_READ], sendbufaddr, MAXTCPREADSIZE);
		if (readlen <= 0)
			return false;

		sendbuf->biosendbuffer.resize(readlen);

		sendinfo->biosendbuffer.push_back(sendbuf);

		return true;
	}

	void sendBioBuffer(const shared_ptr<SocketSendInfo> &sendinfo)
	{
		if (sendinfo->biosendbuffer.size() > 0)
		{
			Guard locker(mutex);
			sendList.push_back(sendinfo);

			if (sendList.size() == 1)
			{
				shared_ptr<Socket> sock = socket;
				if (sock)
					sock->async_send(sendinfo->biosendbuffer, Socket::SendedCallback3(&SSLSocketImpl::onSocketSendCallback, this), sendinfo->timeout);
			}
		}
		else
		{
			doSocketSendCallback(sendinfo);
		}
	}

	void poolBioReadAndSend()
	{
		shared_ptr<SocketSendInfo> sendinfo = make_shared<SocketSendInfo>();

		while (poolBioRead(sendinfo))
		{
		}

		sendBioBuffer(sendinfo);
	}

private:
	void doCheckSSLIsConnected()
	{
		Guard locker(mutex);

		if (sslConnected)
			return;

		int r = SSL_do_handshake(ssl);
		if (r == 1)
		{
			sslConnected = true;
		}
	}
	void onSocketRecvCallback(const weak_ptr<Socket> &sock, const char *buffer, int len)
	{
		if (bio[BIO_WRITE] != NULL && len > 0)
		{
			int writelen = BIO_write(bio[BIO_WRITE], buffer, len);

			assert(writelen == len);
			(void)writelen;
		}

		{
			Guard locker(mutex);
			socketRecving = false;
		}

		//没有链接成功，需要一直接受握手信息
		if (!sslConnected)
			doCheckSSLIsConnected();
		if (!sslConnected)
		{
			socket->async_recv(Socket::ReceivedCallback(&SSLSocketImpl::onSocketRecvCallback, this), MAXTCPREADSIZE);
			{
				Guard locker(mutex);
				socketRecving = true;
			}
		}
		else if (len <= 0)
		{
			shared_ptr<SocketRecvInfo> recvInfo;
			{
				Guard locker(mutex);
				if (recvList.size() > 0)
				{
					recvInfo = recvList.front();
					recvList.pop_front();
				}
			}
			if (recvInfo != NULL)
			{
				if (recvInfo->socketRecvCallback)
					recvInfo->socketRecvCallback(sslsocket.lock(), recvInfo->sslReadAddr, recvInfo->sslHaveReadLen);
				else
					recvInfo->socketRecvCallback1(sslsocket.lock(), recvInfo->socketRecvBuffer, recvInfo->sslHaveReadLen);
			}
		}
		else
		{
			doBioReadAndPostRecv();
		}
	}

	void onSocketSendCallback(const weak_ptr<Socket> &, const std::vector<shared_ptr<SendBuffer>> &)
	{
		shared_ptr<SocketSendInfo> sendbio;
		{
			Guard locker(mutex);
			if (sendList.size() > 0)
			{
				sendbio = sendList.front();
				sendList.pop_front();
			}
		}
		if (sendbio != NULL)
		{
			doSocketSendCallback(sendbio);
		}

		shared_ptr<Socket> sock = socket;
		{
			Guard locker(mutex);
			if (sock && sendList.size() > 0)
			{
				shared_ptr<SocketSendInfo> sendbio = sendList.front();
				sock->async_send(sendbio->biosendbuffer, Socket::SendedCallback3(&SSLSocketImpl::onSocketSendCallback, this), sendbio->timeout);
			}
		}
	}
	void doSocketSendCallback(const shared_ptr<SocketSendInfo> &sendbio)
	{
		if (sendbio->socketSendCallback)
			sendbio->socketSendCallback(sslsocket.lock(), sendbio->socketSendAddr, sendbio->socketSendLen);
		else if (sendbio->socketSendCallback1)
			sendbio->socketSendCallback1(sslsocket.lock(), sendbio->socketSend1);
		else if (sendbio->socketSendCallback2)
			sendbio->socketSendCallback2(sslsocket.lock(), sendbio->socketSend2);
		else if (sendbio->socketSendCallback3)
			sendbio->socketSendCallback3(sslsocket.lock(), sendbio->socketSend3);
	}
	//-------------------------------------以下函数为ssl不支持的接口

	virtual bool bind(const NetAddr &addr, bool reusedAddr = true) { return false; }
	virtual bool async_accept(const Socket::AcceptedCallback &callback, uint32_t timeoutms) { return false; }
	virtual shared_ptr<Socket> accept(uint32_t timeoutms) { return shared_ptr<Socket>(); }
	virtual bool async_connect(const NetAddr &addr, const Socket::ConnectedCallback &callback, uint32_t timeoutms) { return false; }
	virtual bool connect(const NetAddr &addr, uint32_t timeoutms) { return false; }
	virtual bool async_recvfrom(char *buf, uint32_t len, const Socket::RecvFromCallback &received, uint32_t timeoutms) { return false; }
	virtual bool async_recvfrom(const shared_ptr<Socket::RecvBuffer> &buffer, const Socket::RecvFromCallback1 &received, uint32_t timeoutms) { return false; }
	virtual bool async_recvfrom(const Socket::RecvFromCallback &received, int maxlen, uint32_t timeoutms) { return false; }
	virtual int recvfrom(char *buf, uint32_t len, NetAddr &other, uint32_t timeoutms) { return 0; }
	virtual bool async_sendto(const char *buf, uint32_t len, const NetAddr &other, const Socket::SendedCallback &sended, uint32_t timeoutms) { return false; }
	virtual bool async_sendto(const shared_ptr<SendBuffer> &buffer, const NetAddr &other, const SendedCallback1 &sended, uint32_t timeoutms) { return false; }
	virtual bool async_sendto(const std::vector<SBuf> &sendbuf, const NetAddr &other, const SendedCallback2 &sended, uint32_t timeoutms) { return false; }
	virtual bool async_sendto(const std::vector<shared_ptr<SendBuffer>> &sendbuf, const NetAddr &other, const SendedCallback3 &sended, uint32_t timeoutms) { return false; }
	virtual int sendto(const char *buf, uint32_t len, const NetAddr &other, uint32_t timeoutms) { return 0; }
	virtual void socketReady() {}
	virtual void socketError(const std::string &errmsg) {}
};

#endif

shared_ptr<Socket> SSLSocket::create(const shared_ptr<Socket> &sock, const std::string &cert, const std::string &key, uint32_t timeout)
{
	if (sock == NULL || (sock->getNetType() != NetType_TcpClient || sock->getNetType() != NetType_TcpConnection) || sock->getStatus() != NetStatus_connected)
		return shared_ptr<Socket>();

#ifdef SUPPORT_OPENSSL
	shared_ptr<SSLSocketImpl> sslsocket = make_shared<SSLSocketImpl>(sock);

	if (!sslsocket->open(sock, true, cert, key, timeout))
	{
		return shared_ptr<Socket>();
	}

	return sslsocket;
#else
	return shared_ptr<Socket>();
#endif
}
shared_ptr<Socket> SSLSocket::create(const shared_ptr<Socket> &sock, uint32_t timeout)
{
	if (sock == NULL || (sock->getNetType() != NetType_TcpClient || sock->getNetType() != NetType_TcpConnection) || sock->getStatus() != NetStatus_connected)
		return shared_ptr<Socket>();

#ifdef SUPPORT_OPENSSL
	shared_ptr<SSLSocketImpl> sslsocket = make_shared<SSLSocketImpl>(sock);

	if (!sslsocket->open(sock, false, "", "", timeout))
	{
		return shared_ptr<Socket>();
	}

	return sslsocket;
#else
	return shared_ptr<Socket>();
#endif
}

}; // namespace Network
}; // namespace Public
