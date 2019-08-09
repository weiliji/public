
#include "IOWorker.h"
#include "Network/TcpServer.h"

using namespace std;
namespace Public{
namespace Network{

struct TCPServer::TCPServerInternalPointer
{
	shared_ptr<ASocket>		asocket;
};
shared_ptr<Socket> TCPServer::create(const shared_ptr<IOWorker>& _worker,const NetAddr& addr)
{
	shared_ptr<TCPServer> sock = shared_ptr<TCPServer>(new TCPServer(_worker));

	sock->internal->asocket = ASocket::create(_worker, _worker->internal->ioserver, sock, NetType_TcpServer);
	if(sock->internal->asocket->getHandle() <= 0) return shared_ptr<Socket>();

	if(addr.getPort() > 0)
		sock->internal->asocket->bind(addr, true);

	return sock;
}
TCPServer::TCPServer(const shared_ptr<IOWorker>& worker):Socket(worker)
{
	internal = new TCPServerInternalPointer;
}
TCPServer::~TCPServer()
{
	disconnect();

	SAFE_DELETE(internal);
}

bool TCPServer::bind(const NetAddr& addr, bool reusedAddr)
{
	return internal->asocket->bind(addr, reusedAddr);
}
SOCKET TCPServer::getHandle() const
{
	return internal->asocket->getHandle();
}
NetStatus TCPServer::getStatus() const
{
	return internal->asocket->getStatus();
}
NetType TCPServer::getNetType() const
{
	return internal->asocket->getNetType();
}
NetAddr TCPServer::getMyAddr() const
{
	return internal->asocket->getMyAddr();
}
bool TCPServer::disconnect()
{
	return internal->asocket->disconnect();
}
bool TCPServer::async_accept(const AcceptedCallback& accepted)
{
	return internal->asocket->async_accept(accepted);
}

shared_ptr<Socket> TCPServer::accept()
{
	return internal->asocket->accept();
}

bool TCPServer::getSocketTimeout(uint32_t& recvTimeout,uint32_t& sendTimeout) const
{
	return internal->asocket->getSocketTimeout(recvTimeout, sendTimeout);
}
bool TCPServer::setSocketTimeout(uint32_t recvTimeout,uint32_t sendTimeout)
{
	return internal->asocket->setSocketTimeout(recvTimeout, sendTimeout);
}
bool TCPServer::nonBlocking(bool nonblock)
{
	return internal->asocket->nonBlocking(nonblock);
}
bool TCPServer::setSocketOpt(int level, int optname, const void *optval, int optlen)
{
	return internal->asocket->setSocketOpt(level, optname, optval, optlen);
}
bool TCPServer::getSocketOpt(int level, int optname, void *optval, int *optlen) const
{
	return internal->asocket->getSocketOpt(level, optname, optval, optlen);
}

};
};


