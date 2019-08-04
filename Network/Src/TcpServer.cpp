
#include "IOWorker.h"
#include "Network/TcpServer.h"

using namespace std;
namespace Public{
namespace Network{

struct TCPServer::TCPServerInternalPointer
{
	int sock;
	shared_ptr<IOWorker>	ioworker;
	weak_ptr<Socket>		socketptr;
	NetAddr					myaddr;
	shared_ptr<UserThread>	userthread;
#ifdef WIN32
	LPFN_ACCEPTEX					acceptExFunc;
	LPFN_GETACCEPTEXSOCKADDRS		getAcceptAddExFunc;
	LPFN_CONNECTEX					connectExFunc;
#endif

	int	poolid;
	TCPServerInternalPointer():sock(-1),poolid(0){}
};
shared_ptr<Socket> TCPServer::create(const shared_ptr<IOWorker>& _worker,const NetAddr& addr)
{
	shared_ptr<TCPServer> sock = shared_ptr<TCPServer>(new TCPServer(_worker,addr));

	if (sock->tcpserverinternal->sock <= 0) return shared_ptr<Socket>();

	sock->tcpserverinternal->socketptr = sock;

	return sock;
}
TCPServer::TCPServer(const shared_ptr<IOWorker>& _worker, const NetAddr& addr)
{
	tcpserverinternal = new TCPServerInternalPointer;
	tcpserverinternal->userthread = make_shared<UserThread>();
#ifdef WIN32
	
	tcpserverinternal->sock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (tcpserverinternal->sock <= 0)
	{
		return;
	}

	{
		GUID acceptEX = WSAID_ACCEPTEX;
		GUID getAcceptAddrEx = WSAID_GETACCEPTEXSOCKADDRS;
		GUID connetEx = WSAID_CONNECTEX;
		DWORD bytes = 0;

		int ret = WSAIoctl(tcpserverinternal->sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &acceptEX, sizeof(acceptEX), &tcpserverinternal->acceptExFunc, sizeof(tcpserverinternal->acceptExFunc), &bytes, NULL, NULL);

		bytes = 0;
		ret |= WSAIoctl(tcpserverinternal->sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &getAcceptAddrEx, sizeof(getAcceptAddrEx), &tcpserverinternal->getAcceptAddExFunc, sizeof(tcpserverinternal->getAcceptAddExFunc), &bytes, NULL, NULL);

		bytes = 0;
		ret |= WSAIoctl(tcpserverinternal->sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &connetEx, sizeof(connetEx), &tcpserverinternal->connectExFunc, sizeof(tcpserverinternal->connectExFunc), &bytes, NULL, NULL);

		if (ret == SOCKET_ERROR)
		{
			assert(0);
		}
	}
#else
#endif
	tcpserverinternal->ioworker->internal->pool->create(tcpserverinternal->sock,tcpserverinternal->poolid);

	bind(addr, true);
}
TCPServer::~TCPServer()
{
	disconnect();

	tcpserverinternal->userthread->waitAllOtherCallbackThreadUsedEnd();

	SAFE_DELETE(tcpserverinternal);
}

bool TCPServer::bind(const NetAddr& addr, bool reusedAddr)
{
	if (tcpserverinternal->sock <= 0 || addr.getPort() == 0) return false;

	if (reusedAddr)
	{
		int reuseaddr = 1;
		::setsockopt(tcpserverinternal->sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuseaddr, sizeof(int));
	}

	int bindret = ::bind(tcpserverinternal->sock, addr.getAddr(), addr.getAddrLen());

	if (bindret < 0)
	{
		return false;
	}

	tcpserverinternal->myaddr = addr;

	return true;
}
SOCKET TCPServer::getHandle() const
{
	return tcpserverinternal->sock;
}
NetStatus TCPServer::getStatus() const
{
	return NetStatus_disconnected;
}
NetType TCPServer::getNetType() const
{
	return NetType_TcpServer;
}
NetAddr TCPServer::getMyAddr() const
{
	return tcpserverinternal->myaddr;
}
bool TCPServer::disconnect()
{
	tcpserverinternal->userthread->quit();

	if (tcpserverinternal->sock > 0)
	{
		tcpserverinternal->ioworker->internal->pool->destory(tcpserverinternal->sock, tcpserverinternal->poolid);
#ifdef WIN32
		closesocket(tcpserverinternal->sock);
#endif
	}
	tcpserverinternal->sock = -1;

	return true;
}
bool TCPServer::async_accept(const AcceptedCallback& accepted)
{
	if(!accepted || tcpserverinternal->sock <= 0)
	{
		return false;
	}
	//AcceptEvent(const shared_ptr<IOWorker>& _ioworker,const shared_ptr<Socket>& sock, LPFN_ACCEPTEX	acceptExFunc, LPFN_GETACCEPTEXSOCKADDRS _getAcceptAddExFunc,const Socket::AcceptedCallback& _acceptcallback) :WinEvent(sock)
	shared_ptr<AcceptEvent> event = make_shared<AcceptEvent>(tcpserverinternal->ioworker,
		tcpserverinternal->acceptExFunc,tcpserverinternal->getAcceptAddExFunc,accepted);

	tcpserverinternal->ioworker->internal->eventpool->pushEvent(event.get(), event);

	if (!event->init(tcpserverinternal->socketptr.lock(),tcpserverinternal->userthread))
	{
		tcpserverinternal->ioworker->internal->eventpool->popEvent(event.get());
		return false;
	}
	return true;
}


bool TCPServer::getSocketTimeout(uint32_t& recvTimeout,uint32_t& sendTimeout) const
{
	if (tcpserverinternal->sock <= 0)
	{
		return false;
	}

	socklen_t sendlen = sizeof(sendTimeout), recvlen = sizeof(recvTimeout);

	int ret = getsockopt(tcpserverinternal->sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&sendTimeout, (socklen_t*)&sendlen);
	ret |= getsockopt(tcpserverinternal->sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&recvTimeout, (socklen_t*)&recvlen);

	return ret == 0;
}
bool TCPServer::setSocketTimeout(uint32_t recvTimeout,uint32_t sendTimeout)
{
	if (tcpserverinternal->sock <= 0)
	{
		return false;
	}

	int ret = setsockopt(tcpserverinternal->sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&sendTimeout, sizeof(sendTimeout));
	ret |= setsockopt(tcpserverinternal->sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&recvTimeout, sizeof(recvTimeout));

	return ret >= 0;
}
bool TCPServer::nonBlocking(bool nonblock)
{
	return false;
}
bool TCPServer::setSocketOpt(int level, int optname, const void *optval, int optlen)
{
	if (optval == NULL || tcpserverinternal->sock <= 0)
	{
		return false;
	}

	return ::setsockopt(tcpserverinternal->sock, level, optname, (const char*)optval, optlen) >= 0;
}
bool TCPServer::getSocketOpt(int level, int optname, void *optval, int *optlen) const
{
	if (optval == NULL || tcpserverinternal->sock <= 0)
	{
		return false;
	}

	return ::getsockopt(tcpserverinternal->sock, level, optname, (char*)optval, (socklen_t*)optlen) >= 0;
}

};
};


