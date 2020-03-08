
#include "IOWorker.h"
#include "Network/Socket/TcpServer.h"

using namespace std;
namespace Public{
namespace Network{


shared_ptr<Socket> TCPServer::create(const shared_ptr<IOWorker>& _worker, const NetAddr& addr)
{
	shared_ptr<ASocket> asocket = ASocket::create(_worker, _worker->internal->poll, addr.getNetType(),NetType_TcpServer);
	
	if(addr.getPort() > 0)
		asocket->bind(addr, true);

	return asocket;
}

};
};


