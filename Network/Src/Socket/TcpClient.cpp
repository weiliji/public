#include "IOWorker.h"
#include "Network/Socket/TcpClient.h"

using namespace std;
namespace Public{
namespace Network{

shared_ptr<Socket> TCPClient::create(const shared_ptr<IOWorker>& _worker, InetType inet)
{
	shared_ptr<ASocket> asocket = ASocket::create(_worker, _worker->internal->poll, inet,NetType_TcpClient);
	
	return asocket;
}
shared_ptr<Socket> TCPClient::create(const shared_ptr<IOWorker>& _worker, const NewSocketInfo& newsockinfo)
{
	shared_ptr<ASocket> asocket = ASocket::create(_worker, _worker->internal->poll, newsockinfo.inet,NetType_TcpConnection,newsockinfo.newsocket,newsockinfo.otheraddr);

	return asocket;
}
};
};



