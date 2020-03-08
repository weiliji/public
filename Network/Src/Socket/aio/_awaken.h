#pragma once
#include "Base/Base.h"
#include "Network/Socket/Socket.h"
using namespace Public::Base;
using namespace Public::Network;

class Pipe
{
public:
    Pipe() {}
    virtual ~Pipe() {}

    virtual SOCKET readfd() = 0;
    virtual SOCKET writefd() = 0;
};

#ifdef WIN32
inline int socketpair(int family, int type, int protocol, SOCKET fd[2])
{
	SOCKET listener = -1;
	SOCKET connector = -1;
	SOCKET acceptor = -1;
	struct sockaddr_in listen_addr;
	struct sockaddr_in connect_addr;
	int size;
	int saved_errno = -1;

	listener = socket(AF_INET, type, 0);
	if (listener < 0)
		return -1;

	memset(&listen_addr, 0, sizeof(listen_addr));
	listen_addr.sin_family = AF_INET;
	listen_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	listen_addr.sin_port = 0;	/* kernel chooses port.	 */

	if (::bind(listener, (struct sockaddr *) &listen_addr, sizeof(listen_addr)) == -1)
		goto tidy_up_and_fail;
	if (::listen(listener, 1) == -1)
		goto tidy_up_and_fail;

	connector = socket(AF_INET, type, 0);
	if (connector < 0)
		goto tidy_up_and_fail;
	/* We want to find out the port number to connect to.  */
	size = sizeof(connect_addr);
	//获取监听地址
	if (getsockname(listener, (struct sockaddr *) &connect_addr, &size) == -1)
		goto tidy_up_and_fail;
	if (size != sizeof(connect_addr))
		goto abort_tidy_up_and_fail;
	//connector连接到listener上
	if (connect(connector, (struct sockaddr *) &connect_addr,
		sizeof(connect_addr)) == -1)
		goto tidy_up_and_fail;

	size = sizeof(listen_addr);
	//listener接受连接，此时connector与acceptor是一对已连接的socket pair
	acceptor = accept(listener, (struct sockaddr *) &listen_addr, &size);
	if (acceptor < 0)
		goto tidy_up_and_fail;
	if (size != sizeof(listen_addr))
		goto abort_tidy_up_and_fail;
	/* 判断两个socket之间的地址，端口，协议是否相同	 */
	if (getsockname(connector, (struct sockaddr *) &connect_addr, &size) == -1)
		goto tidy_up_and_fail;
	if (size != sizeof(connect_addr)
		|| listen_addr.sin_family != connect_addr.sin_family
		|| listen_addr.sin_addr.s_addr != connect_addr.sin_addr.s_addr
		|| listen_addr.sin_port != connect_addr.sin_port)
		goto abort_tidy_up_and_fail;
	closesocket(listener);//不需要再监听了
	fd[0] = connector;
	fd[1] = acceptor;

	/**
	* 此时就已经创建了两个连接起来的socket，即可以实现进程间的通信
	*/
	return 0;

abort_tidy_up_and_fail:
tidy_up_and_fail:
//	if (saved_errno < 0)
//		;//saved_errno = EVUTIL_SOCKET_ERROR();
	if (listener != -1)
		closesocket(listener);
	if (connector != -1)
		closesocket(connector);
	if (acceptor != -1)
		closesocket(acceptor);

	return -1;
}
void nonblock(SOCKET sock)
{
	unsigned long ul = 1;
	ioctlsocket(sock, FIONBIO, (unsigned long *)&ul);
}
#define PAIR_AF_LOCAL	AF_INET
#else
void nonblock(SOCKET sock)
{
	int flags = fcntl(sock, F_GETFL, 0);

	flags |= O_NONBLOCK;

	fcntl(sock, F_SETFL, flags);
}
#define PAIR_AF_LOCAL	AF_LOCAL
#endif
class Pipe_Socketpair : public Pipe
{
public:
    Pipe_Socketpair()
    {
        int ret = socketpair(PAIR_AF_LOCAL, SOCK_STREAM, 0, fd);
        assert(ret == 0);
        (void)ret;
		nonblock(fd[0]);
		nonblock(fd[1]);
    }
    virtual ~Pipe_Socketpair()
    {
        if (fd[0] != 0)
            closesocket(fd[0]);
        if (fd[1] != 0)
			closesocket(fd[1]);
    }

    virtual SOCKET readfd() { return fd[1]; }
    virtual SOCKET writefd() { return fd[0]; }

private:
	SOCKET fd[2] = {
        0,
    };
};


class Awaken
{
public:
	Awaken(uint32_t _cachesize = 1024) : cachesize(_cachesize)
    {
        pipe = make_shared<Pipe_Socketpair>();
        cache.alloc(cachesize);
    }
    virtual ~Awaken() {}

    SOCKET readfd() { return pipe->readfd(); }

    bool write_v(const void *data, uint32_t len)
    {
        if (pipe == NULL)
            return false;

        int writelen = ::send(pipe->writefd(),(const char*)data, len,0);

        return writelen == (int)len;
    }
    template <typename T>
    std::vector<T> read_v()
    {
        std::vector<T> readv;

        char *cachebuf = cache.c_str();

		while (1)
		{
			if (pipe)
			{
				int canreadlen = (int)(cachesize - cache.length());
				int readlen = ::recv(pipe->readfd(), cachebuf, canreadlen,0);
				if (readlen > 0)
				{
					cache.resize(cache.length() + readlen);
				}
			}
			size_t cachelen = cache.length();
			if (cachelen < sizeof(T)) break;

			size_t usedlen = 0;
			while (cachelen - usedlen >= sizeof(T))
			{
				T v = *(T *)(cachebuf + usedlen);
				readv.push_back(v);
				usedlen += sizeof(T);
			}
			if (usedlen > 0 && cachelen - usedlen > 0)
			{
				memmove(cachebuf, cachebuf + usedlen, cachelen - usedlen);
			}
			cache.resize(cachelen - usedlen);
		}

        

        return readv;
    }

private:
    shared_ptr<Pipe> pipe;
    String cache;
    uint32_t cachesize = 0;
};