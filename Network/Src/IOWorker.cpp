#include "IOWorker.h"

namespace Public{
namespace Network{

IOWorker::IOWorker(const ThreadNum& num)
{
	internal = new IOWorkerInternal;
	internal->ioserver = make_shared<IOServer>(num.getNum());
}
IOWorker::~IOWorker()
{
	internal->ioserver = NULL;

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


}
}
