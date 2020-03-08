#include "IOWorker.h"

namespace Public
{
namespace Network
{

IOWorker::IOWorker(const ThreadNum &num, Thread::Priority pri)
{
	NetworkSystem::init();

	uint32_t threadnum = num.getNum();
	if (threadnum < 0)
		threadnum = 1;

	internal = new IOWorkerInternal;

	internal->poll = ASocket::createPoll(threadnum, pri);
}
IOWorker::~IOWorker()
{
	internal->poll = NULL;

	SAFE_DELETE(internal);
}
bool IOWorker::postEvent(const EventCallback &callback, void *param)
{
	return internal->poll->postExtExentFunction(callback, param);
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
} // namespace Network
} // namespace Public
