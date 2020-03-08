#include "IOWorker.h"
#include "Network/Socket/RawSocket.h"

using namespace std;
namespace Public{
namespace Network{

shared_ptr<Socket> RawSocket::create(const shared_ptr<IOWorker>& _worker, InetType inet)
{
	shared_ptr<ASocket> asocket = ASocket::create(_worker, _worker->internal->poll, inet,NetType_Raw);
	
	return asocket;
}
};
};



