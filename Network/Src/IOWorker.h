#include "aio/ioserver.h"
#include "aio/asocket.h"
#include "Network/Network.h"
using namespace Public::Base;

namespace Public{
namespace Network{

class SocketInitObjec
{
public:
	SocketInitObjec()
	{
#ifdef WIN32
		WSADATA wsaData;
		WORD wVersionRequested;

		wVersionRequested = MAKEWORD(2, 2);
		int errorCode = WSAStartup(wVersionRequested, &wsaData);
		if (errorCode != 0)
		{
			return;
		}

		if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
		{
			WSACleanup();
			return;
		}
#else
#endif
	}

	~SocketInitObjec() {}
};

class IOWorker::IOWorkerInternal
{
	SocketInitObjec		socketinit;
public:
	shared_ptr<IOServer> ioserver;
};


}
}
