#include "IOWorker.h"
#include "Network/TcpClient.h"

using namespace std;
namespace Public{
namespace Network{

shared_ptr<Socket> TCPClient::create(const shared_ptr<IOWorker>& _worker)
{
	shared_ptr<ASocket> asocket = ASocket::create(_worker, _worker->internal->ioserver, NetType_TcpClient);
	
	return asocket;
}
shared_ptr<Socket> TCPClient::create(const shared_ptr<IOWorker>& _worker, const NewSocketInfo& newsockinfo)
{
	shared_ptr<ASocket> asocket = ASocket::create(_worker, _worker->internal->ioserver, newsockinfo);

	return asocket;
}
};
};



