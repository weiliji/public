#include "IOWorker.h"
#include "Network/Socket/Strand.h"

namespace Public {
namespace Network {

struct StrandInternalCallbackObj
{
	Strand::StrandCallback handler;
	weak_ptr<Strand::StrandData> data;
};

struct StrandInfo:public enable_shared_from_this<StrandInfo>
{
	Mutex							mutex;
	std::list<StrandInternalCallbackObj> calllist;
	bool							callruning;
	shared_ptr<IOWorker>			worker;

	void strandRunFunc(void* param)
	{
		StrandInternalCallbackObj info;
		{
			Guard locker(mutex);
			if (calllist.size() > 0)
			{
				info = calllist.front();
				calllist.pop_front();
			}
		}

		shared_ptr<Strand::StrandData > stranddata = info.data.lock();
		if (stranddata) info.handler(stranddata);

		{
			Guard locker(mutex);
			callruning = false;
		}

		checkRunFunc();
	}

	void checkRunFunc()
	{
		{
			Guard locker(mutex);
			if (callruning) return;

			if (calllist.size() <= 0) return;

			callruning = true;
		}

		worker->internal->poll->postExtExentFunction(IOWorker::EventCallback(&StrandInfo::strandRunFunc,shared_from_this()), NULL);
	}
};


struct Strand::StrandInternal
{
	shared_ptr<StrandInfo> strandinfo;
};

Strand::Strand(const shared_ptr<IOWorker>& ioworker)
{
	internal = new StrandInternal();
	internal->strandinfo = make_shared<StrandInfo>();
	internal->strandinfo->worker = ioworker;
}
Strand::~Strand()
{
	SAFE_DELETE(internal);
}

void Strand::post(const Strand::StrandCallback& handler, const shared_ptr<Strand::StrandData>& data)
{
	{
		Guard locker(internal->strandinfo->mutex);

		StrandInternalCallbackObj info;
		info.handler = handler;
		info.data = data;

		internal->strandinfo->calllist.push_back(info);
	}

	internal->strandinfo->checkRunFunc();
}

}
}


