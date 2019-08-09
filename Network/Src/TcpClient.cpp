#include "IOWorker.h"
#include "Network/TcpClient.h"

using namespace std;
namespace Public{
namespace Network{

struct TCPClient::TCPClientInternalPointer
{
	shared_ptr<ASocket>		asocket;
	Socket::DisconnectedCallback	disconnectcallback;

	weak_ptr<Socket>		sockobj;
};

shared_ptr<Socket> TCPClient::create(const shared_ptr<IOWorker>& _worker, void* socketptr)
{
	shared_ptr<TCPClient> sock = shared_ptr<TCPClient>(new TCPClient(_worker));
	NewSocketInfo* newsocketinfo = (NewSocketInfo*)socketptr;
	if (newsocketinfo == NULL)
	{
		sock->internal->asocket = ASocket::create(_worker, _worker->internal->ioserver, sock, NetType_TcpClient);
	}
	else
	{
		sock->internal->asocket = ASocket::create(_worker, _worker->internal->ioserver, sock, *newsocketinfo);
		SAFE_DELETE(newsocketinfo);
	}
	sock->internal->sockobj = sock;
	

	return sock;
}
TCPClient::TCPClient(const shared_ptr<IOWorker>& worker):Socket(worker)
{
	internal = new TCPClientInternalPointer();
}
TCPClient::~TCPClient()
{
	disconnect();
	SAFE_DELETE(internal);
}
bool TCPClient::bind(const NetAddr& addr,bool reusedAddr)
{
	return internal->asocket->bind(addr, reusedAddr);
}
bool TCPClient::disconnect()
{
	return internal->asocket->disconnect();
}
bool TCPClient::getSocketBuffer(uint32_t& recvSize,uint32_t& sendSize) const
{
	return internal->asocket->getSocketBuffer(recvSize, sendSize);
}
bool TCPClient::setSocketBuffer(uint32_t recvSize,uint32_t sendSize)
{
	return internal->asocket->setSocketBuffer(recvSize, sendSize);
}
bool TCPClient::getSocketTimeout(uint32_t& recvTimeout,uint32_t& sendTimeout) const
{
	return internal->asocket->getSocketTimeout(recvTimeout, sendTimeout);
}
bool TCPClient::setSocketTimeout(uint32_t recvTimeout,uint32_t sendTimeout)
{
	return internal->asocket->setSocketTimeout(recvTimeout, sendTimeout);
}
bool TCPClient::nonBlocking(bool nonblock)
{
	return internal->asocket->nonBlocking(nonblock);
}
SOCKET TCPClient::getHandle() const
{
	return internal->asocket->getHandle();
}
NetStatus TCPClient::getStatus() const
{
	return internal->asocket->getStatus();
}
NetType TCPClient::getNetType() const
{
	return internal->asocket->getNetType();
}
NetAddr TCPClient::getMyAddr() const
{
	return internal->asocket->getMyAddr();
}
NetAddr TCPClient::getOtherAddr() const
{
	return internal->asocket->getOtherAddr();
}
bool TCPClient::async_connect(const NetAddr& addr,const ConnectedCallback& connected)
{
	return internal->asocket->async_connect(addr, connected);
}
bool TCPClient::connect(const NetAddr& addr)
{
	return internal->asocket->connect(addr);
}
bool TCPClient::setDisconnectCallback(const Socket::DisconnectedCallback& disconnected)
{
	internal->disconnectcallback = disconnected;

	return true;
}
bool TCPClient::async_recv(char *buf , uint32_t len,const Socket::ReceivedCallback& received)
{
	return internal->asocket->async_recv(buf, len, received);
}
bool TCPClient::async_recv(const ReceivedCallback& received, int maxlen)
{
	return internal->asocket->async_recv(received, maxlen);
}
bool TCPClient::async_send(const char * buf, uint32_t len,const Socket::SendedCallback& sended)
{
	return internal->asocket->async_send(buf, len, sended);
}
int TCPClient::recv(char *buf , uint32_t len)
{
	return internal->asocket->recv(buf, len);
}
int TCPClient::send(const char * buf, uint32_t len)
{
	return internal->asocket->send(buf, len);
}
bool TCPClient::setSocketOpt(int level, int optname, const void *optval, int optlen)
{
	return internal->asocket->setSocketOpt(level, optname, optval, optlen);
}
bool TCPClient::getSocketOpt(int level, int optname, void *optval, int *optlen) const
{
	return internal->asocket->getSocketOpt(level, optname, optval, optlen);
}
void TCPClient::socketReady() 
{
	internal->asocket->socketReady();
}
void TCPClient::socketError(const std::string &errmsg)
{
	internal->disconnectcallback(internal->sockobj.lock(), "disconnected");
}

};
};



