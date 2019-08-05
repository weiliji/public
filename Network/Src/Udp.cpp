#include "IOWorker.h"
#include "Network/Udp.h"

using namespace std;
namespace Public{
namespace Network{

struct UDP::UDPInternalPointer
{
	shared_ptr<ASocket> asocket;
};
shared_ptr<Socket> UDP::create(const shared_ptr<IOWorker>& worker)
{
	shared_ptr<UDP> sock = shared_ptr<UDP>(new UDP());
	sock->internal->asocket = ASocket::create(worker, worker->internal->ioserver, sock, NetType_Udp);

	return sock;
}
UDP::UDP()
{
	internal = new UDPInternalPointer();
}
UDP::~UDP()
{
	disconnect();
	SAFE_DELETE(internal);
}
bool UDP::bind(const NetAddr& addr,bool reusedaddr)
{
	return internal->asocket->bind(addr,reusedaddr);
}
bool UDP::disconnect()
{
	return internal->asocket->disconnect();
}
bool UDP::getSocketBuffer(uint32_t& recvSize,uint32_t& sendSize) const
{
	return internal->asocket->getSocketBuffer(recvSize, sendSize);
}
bool UDP::setSocketBuffer(uint32_t recvSize,uint32_t sendSize)
{
	return internal->asocket->setSocketBuffer(recvSize, sendSize);
}
bool UDP::getSocketTimeout(uint32_t& recvTimeout,uint32_t& sendTimeout) const
{
	return internal->asocket->getSocketTimeout(recvTimeout, sendTimeout);
}
bool UDP::setSocketTimeout(uint32_t recvTimeout,uint32_t sendTimeout)
{
	return internal->asocket->setSocketTimeout(recvTimeout, sendTimeout);
}
bool UDP::nonBlocking(bool nonblock)
{
	return internal->asocket->nonBlocking(nonblock);
}
SOCKET UDP::getHandle() const
{
	return internal->asocket->getHandle();
}
NetType UDP::getNetType() const
{
	return NetType_Udp;
}
NetAddr UDP::getMyAddr() const
{
	return internal->asocket->getMyAddr();
}

bool UDP::async_recvfrom(char *buf , uint32_t len,const RecvFromCallback& received)
{
	return internal->asocket->async_recvfrom(buf, len, received);
}
bool UDP::async_recvfrom(const RecvFromCallback& received, int maxlen)
{
	return internal->asocket->async_recvfrom(received, maxlen);
}
bool UDP::async_sendto(const char * buf, uint32_t len,const NetAddr& other,const SendedCallback& sended)
{
	return internal->asocket->async_sendto(buf, len, other, sended);
}

int UDP::recvfrom(char *buf , uint32_t len,NetAddr& other)
{
	return internal->asocket->recvfrom(buf, len, other);
}
int UDP::sendto(const char * buf, uint32_t len,const NetAddr& other)
{
	return internal->asocket->sendto(buf, len, other);
}
bool UDP::setSocketOpt(int level, int optname, const void *optval, int optlen)
{
	return internal->asocket->setSocketOpt(level, optname, optval, optlen);
}
bool UDP::getSocketOpt(int level, int optname, void *optval, int *optlen) const
{
	return internal->asocket->getSocketOpt(level, optname, optval, optlen);
}

};
};

