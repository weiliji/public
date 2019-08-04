#include "aio/_eventpool.h"
#include "aio/_winevent.h"
#include "aio/_eventthreadpool.h"
#include "aio/_epool.h"
#include "aio/_iocp.h"

using namespace Public::Base;

namespace Public{
namespace Network{

class SocketInitObjec
{
public:
	SocketInitObjec()
	{
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
	}

	~SocketInitObjec() {}
};

class IOWorker::IOWorkerInternal
{
public:
	SocketInitObjec				socketinitobjec;

	uint32_t					threadnum;

	shared_ptr<EventPool>		eventpool;
	shared_ptr<Pool>			pool;
	shared_ptr<EventThreadPool> threadpool;
};

IOWorker::IOWorker(const ThreadNum& num)
{
	internal = new IOWorkerInternal;
	internal->threadnum = num.getNum();


	if (internal->threadnum <= 0) internal->threadnum = 1;

	uint32_t poolthreadnum = internal->threadnum - 1;

	internal->eventpool = make_shared<EventPool>();
	internal->threadpool = make_shared<EventThreadPool>(internal->eventpool, poolthreadnum);

#ifdef WIN32
	internal->pool = make_shared<IOCP>(internal->threadpool);
#else
	internal->pool = make_shared<EPool>(internal->threadpool);
#endif
}
IOWorker::~IOWorker()
{
	internal->pool = NULL;
	internal->eventpool = NULL;
	internal->eventpool = NULL;

	SAFE_DELETE(internal);
}

shared_ptr<IOWorker> IOWorker::defaultWorker()
{
	static weak_ptr<IOWorker> workerptr;
	static Mutex workermutex;

	Guard locker(workermutex);
	shared_ptr<IOWorker> defaultworker = workerptr.lock();
	if (defaultworker == NULL)
	{
		workerptr = defaultworker = make_shared<IOWorker>(ThreadNum(2));
	}

	return defaultworker;
}

uint32_t IOWorker::threadNum()
{
	return internal->threadnum;
}

}
}
