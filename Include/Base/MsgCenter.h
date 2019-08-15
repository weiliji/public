#pragma  once
#include "Base/IntTypes.h"
#include "Base/Guard.h"
#include "Base/Shared_ptr.h"

namespace Public {
namespace Base {


//消息中心/用于订阅发布消息
class MsgCenter
{
private:
	struct WorkerFunc
	{
		virtual ~WorkerFunc() {}
	};
	struct WorkerImpl
	{
	public:
		WorkerImpl() {}
		virtual ~WorkerImpl() {}

		virtual std::string type() = 0;

		virtual void unsubscribe(void* flag)
		{
			GuardWriteMutex guard(rwmutex);

			funclist.erase(flag);
		}
	protected:
		void _subscribe(void* flag, const shared_ptr<WorkerFunc>& func)
		{
			GuardWriteMutex guard(rwmutex);

			funclist[flag] = func;
		}
	protected:
		ReadWriteMutex rwmutex;
		std::map<void*, shared_ptr<WorkerFunc> > funclist;
	};
public:
	template<typename ... ARGS >
	struct WorkerProxy :public WorkerImpl
	{
		struct WorkerProxyFunc :public WorkerFunc
		{
			void run(ARGS ... a) { func(a ...); }

			Function<void(ARGS ...)> func;
		};
	public:
		WorkerProxy() {}
		virtual ~WorkerProxy() {}

		void subscribe(void* flag, const Function<void (ARGS ...)>& func)
		{
			shared_ptr<WorkerProxy> worktmp = make_shared<WorkerProxy>();
			worktmp->func = func;

			_subscribe(flag, worktmp);
		}

		void publish(ARGS  ... a)
		{
			GuardReadMutex locker(rwmutex);
			for (std::map<void*, shared_ptr<WorkerFunc> >::iterator iter = funclist.begin(); iter != funclist.end(); iter++)
			{
				WorkerProxy* func1 = (WorkerProxy*)iter->second.get();
				func1->run(a ...);
			}
		}

		std::string type()
		{
			return typeid(Function<void(ARGS ...)>()).name();
		}
	};

	shared_ptr<WorkerImpl> getAndSetWroker(int msgid, const shared_ptr<WorkerImpl>& worker)
	{
		Guard locker(mutex);
		std::map<int, shared_ptr< WorkerImpl > >::iterator iter = worklist.find(msgid);
		if (iter == worklist.end())
		{
			worklist[msgid] = worker;

			return worker;
		}

		if (worker->type() != iter->second->type())
		{
			return shared_ptr<WorkerImpl>();
		}

		return iter->second;
	}
public:
	template<typename ...ARGS >
	struct Worker
	{
		Worker() {}
		Worker(const shared_ptr<WorkerProxy<ARGS ...> >& _worker) :worker(_worker) {}
		Worker(const Worker& tmp) { worker = tmp.worker; }

		shared_ptr<WorkerProxy<ARGS ...> > operator->() { return worker; }
	public:
		shared_ptr<WorkerProxy<ARGS ...> > worker;
	};
public:
	MsgCenter() {}
	~MsgCenter() {}

	template<typename ...ARGS >
	Worker<ARGS ...> worker(int msgid)
	{
		shared_ptr<WorkerProxy<ARGS ...> > ptr = make_shared<WorkerProxy<ARGS ...> >();
		return static_pointer_cast<WorkerProxy<ARGS ...> >(getAndSetWroker(msgid, ptr));
	}
private:
	Mutex								 mutex;
	std::map<int, shared_ptr< WorkerImpl > > worklist;
};



}
}

