#include "IOWorker.h"
#include "Network/Socket/Udp.h"

using namespace std;
namespace Public{
namespace Network{

shared_ptr<Socket> UDP::create(const shared_ptr<IOWorker>& worker, InetType inet)
{
	shared_ptr<Socket> asocket = ASocket::create(worker, worker->internal->poll, inet,NetType_Udp);

	return asocket;
}

};
};

