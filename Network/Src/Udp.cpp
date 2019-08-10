#include "IOWorker.h"
#include "Network/Udp.h"

using namespace std;
namespace Public{
namespace Network{

shared_ptr<Socket> UDP::create(const shared_ptr<IOWorker>& worker)
{
	shared_ptr<Socket> asocket = ASocket::create(worker, worker->internal->ioserver, NetType_Udp);

	return asocket;
}

};
};

