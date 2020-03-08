#include "_reactor_socket.h"
#include "_proactor_socket.h"
#include "_proactor_iocp.h"
#include "_reactor_epoll.h"
#include "_reactor_poll.h"
#include "_reactor_kqueue.h"

#include "asocket.h"

shared_ptr<ASocket>
ASocket::create(const shared_ptr<IOWorker> &_ioworker, const shared_ptr<_Poll> &_poll, InetType inet, NetType _type, SOCKET _sock, const NetAddr &_otheraddr)
{
	shared_ptr<ASocket> sock;
#ifdef SUPPORT_IOCP
	if (_poll->type() == PoolType_IOCP)
	{
		sock = shared_ptr<ASocket>(new _Proactor_Socket(_ioworker, _poll, inet, _type, _sock, _otheraddr));
	}
#endif
	if (_poll->type() != PoolType_IOCP && sock == NULL)
	{
		sock = shared_ptr<ASocket>(new _Reactor_Socket(_ioworker, _poll, inet, _type, _sock, _otheraddr));
	}

	if (_sock == (SOCKET)INVALIDHANDLE && !sock->creatSocket(_type))
	{
		assert(0);
	}

	sock->socketptr = sock;
	sock->resourece = _poll->createResource(sock->sock, sock);

#ifdef WIN32
	if (_type == NetType_TcpClient || _type == NetType_TcpConnection)
	{
		int flag = 1;
		sock->setSocketOpt(IPPROTO_TCP, TCP_NODELAY, (const char *)&flag, sizeof(flag));
	}
#endif

	return sock;
}

shared_ptr<_Poll> ASocket::createPoll(uint32_t threadnum, Thread::Priority pri)
{
	shared_ptr<_Poll> poll;

#ifdef SUPPORT_IOCP
	if (!poll)
		poll = make_shared<_Proactor_IOCP>(threadnum, pri);
#endif

#ifdef SUPPORT_EPOLL
	if (!poll)
		poll = make_shared<_Reactor_Epoll>(threadnum, pri);
#endif

#ifdef SUPPORT_KQUEUE
	if (!poll)
		poll = make_shared<_Reactor_Kqueue>(threadnum, pri);
#endif

#ifdef SUPPORT_POLL
	if (!poll)
		poll = make_shared<_Reactor_Polled>(threadnum, pri);
#endif

	return poll;
}