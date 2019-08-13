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
	struct IWorker
	{
	public:
		IWorker() {}
		virtual ~IWorker() {}

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
	template<typename P1>
	struct IWorker1 :public IWorker
	{
		struct WorkerFunc1 :public WorkerFunc
		{
			virtual ~WorkerFunc1() {}
			virtual void run(const P1& p) = 0;
		};

		template<typename T1>
		struct WrokerFunc1_TMP :public WorkerFunc1
		{
			void run(const P1& p1) { func(p1); }
			Function1<void, T1> func;
		};
	public:
		IWorker1() {}
		virtual ~IWorker1() {}

		template<typename T1>
		void subscribe(void* flag, const Function1<void, T1>& func)
		{
			shared_ptr<WrokerFunc1_TMP<T1> > worktmp = make_shared<WrokerFunc1_TMP<T1> >();
			worktmp->func = func;

			_subscribe(flag, worktmp);
		}

		void publish(const P1& p1)
		{
			GuardReadMutex locker(rwmutex);
			for (std::map<void*, shared_ptr<WorkerFunc> >::iterator iter = funclist.begin(); iter != funclist.end(); iter++)
			{
				WorkerFunc1* func1 = (WorkerFunc1*)iter->second.get();
				func1->run(p1);
			}
		}

		std::string type()
		{
			return typeid(Function1<void, const P1&>()).name();
		}
	};
	template<typename P1, typename P2>
	struct IWorker2 :public IWorker
	{
		struct WorkerFunc2 :public WorkerFunc
		{
			virtual ~WorkerFunc2() {}
			virtual void run(const P1& p1, const P2& p2) = 0;
		};

		template<typename T1, typename T2>
		struct WrokerFunc2_TMP :public WorkerFunc2
		{
			void run(const P1& p1, const P2& p2) { func(p1, p2); }
			Function2<void, T1, T2> func;
		};
	public:
		IWorker2() {}
		virtual ~IWorker2() {}

		template<typename T1, typename T2>
		void subscribe(void* flag, const Function2<void, T1, T2>& func)
		{
			shared_ptr<WrokerFunc2_TMP<T1, T2> > worktmp = make_shared<WrokerFunc2_TMP<T1, T2> >();
			worktmp->func = func;

			_subscribe(flag, worktmp);
		}

		void publish(const P1& p1, const P2& p2)
		{
			GuardReadMutex locker(rwmutex);
			for (std::map<void*, shared_ptr<WorkerFunc> >::iterator iter = funclist.begin(); iter != funclist.end(); iter++)
			{
				WorkerFunc2* func1 = (WorkerFunc2*)iter->second.get();
				func1->run(p1, p2);
			}
		}

		std::string type()
		{
			return typeid(Function2<void, const P1&, const P2&>()).name();
		}
	};
	template<typename P1, typename P2, typename P3>
	struct IWorker3 :public IWorker
	{
		struct WorkerFunc3 :public WorkerFunc
		{
			virtual ~WorkerFunc3() {}
			virtual void run(const P1& p1, const P2& p2, const P3& p3) = 0;
		};

		template<typename T1, typename T2, typename T3>
		struct WrokerFunc3_TMP :public WorkerFunc3
		{
			void run(const P1& p1, const P2& p2, const P3& p3) { func(p1, p2, p3); }
			Function3<void, T1, T2, T3> func;
		};
	public:
		IWorker3() {}
		virtual ~IWorker3() {}

		template<typename T1, typename T2, typename T3>
		void subscribe(void* flag, const Function3<void, T1, T2, T3>& func)
		{
			shared_ptr<WrokerFunc3_TMP<T1, T2, T3> > worktmp = make_shared<WrokerFunc3_TMP<T1, T2, T3> >();
			worktmp->func = func;

			_subscribe(flag, worktmp);
		}

		void publish(const P1& p1, const P2& p2, const P3& p3)
		{
			GuardReadMutex locker(rwmutex);
			for (std::map<void*, shared_ptr<WorkerFunc> >::iterator iter = funclist.begin(); iter != funclist.end(); iter++)
			{
				WorkerFunc3* func1 = (WorkerFunc3*)iter->second.get();
				func1->run(p1, p2, p3);
			}
		}

		std::string type()
		{
			return typeid(Function3<void, const P1&, const P2&, const P3&>()).name();
		}
	};
	template<typename P1, typename P2, typename P3, typename P4>
	struct IWorker4 :public IWorker
	{
		struct WorkerFunc4 :public WorkerFunc
		{
			virtual ~WorkerFunc4() {}
			virtual void run(const P1& p1, const P2& p2, const P3& p3, const P4& p4) = 0;
		};

		template<typename T1, typename T2, typename T3, typename T4>
		struct WrokerFunc4_TMP :public WorkerFunc4
		{
			void run(const P1& p1, const P2& p2, const P3& p3, const P4& p4) { func(p1, p2, p3, p4); }
			Function4<void, T1, T2, T3, T4> func;
		};
	public:
		IWorker4() {}
		virtual ~IWorker4() {}

		template<typename T1, typename T2, typename T3, typename T4>
		void subscribe(void* flag, const Function4<void, T1, T2, T3, T4>& func)
		{
			shared_ptr<WrokerFunc4_TMP<T1, T2, T3, T4> > worktmp = make_shared<WrokerFunc4_TMP<T1, T2, T3, T4> >();
			worktmp->func = func;

			_subscribe(flag, worktmp);
		}

		void publish(const P1& p1, const P2& p2, const P3& p3, const P4& p4)
		{
			GuardReadMutex locker(rwmutex);
			for (std::map<void*, shared_ptr<WorkerFunc> >::iterator iter = funclist.begin(); iter != funclist.end(); iter++)
			{
				WorkerFunc4* func1 = (WorkerFunc4*)iter->second.get();
				func1->run(p1, p2, p3, p4);
			}
		}

		std::string type()
		{
			return typeid(Function4<void, const P1&, const P2&, const P3&, const P4&>()).name();
		}
	};
	shared_ptr<IWorker> getAndSetWroker(int msgid, const shared_ptr<IWorker>& worker)
	{
		Guard locker(mutex);
		std::map<int, shared_ptr< IWorker > >::iterator iter = worklist.find(msgid);
		if (iter == worklist.end())
		{
			worklist[msgid] = worker;

			return worker;
		}

		if (worker->type() != iter->second->type())
		{
			return shared_ptr<IWorker>();
		}

		return iter->second;
	}
public:
	template<typename P1>
	struct Worker1
	{
		Worker1() {}
		Worker1(const shared_ptr<IWorker1<P1> >& _worker) :worker(_worker) {}
		Worker1(const Worker1& tmp) { worker = tmp.worker; }

		shared_ptr<IWorker1<P1> > operator->() { return worker; }
	public:
		shared_ptr<IWorker1<P1> > worker;
	};
	template<typename P1, typename P2>
	struct Worker2
	{
		Worker2() {}
		Worker2(const shared_ptr<IWorker2<P1, P2> >& _worker) :worker(_worker) {}
		Worker2(const Worker2& tmp) { worker = tmp.worker; }

		shared_ptr<IWorker2<P1, P2> > operator->() { return worker; }
	public:
		shared_ptr<IWorker2<P1, P2> > worker;
	};
	template<typename P1, typename P2, typename P3>
	struct Worker3
	{
		Worker3() {}
		Worker3(const shared_ptr<IWorker3<P1, P2, P3> >& _worker) :worker(_worker) {}
		Worker3(const Worker3& tmp) { worker = tmp.worker; }

		shared_ptr<IWorker3<P1, P2, P3> > operator->() { return worker; }
	public:
		shared_ptr<IWorker3<P1, P2, P3> > worker;
	};
	template<typename P1, typename P2, typename P3, typename P4>
	struct Worker4
	{
		Worker4() {}
		Worker4(const shared_ptr<IWorker4<P1, P2, P3, P4> >& _worker) :worker(_worker) {}
		Worker4(const Worker4& tmp) { worker = tmp.worker; }

		shared_ptr<IWorker4<P1, P2, P3, P4> > operator->() { return worker; }
	public:
		shared_ptr<IWorker4<P1, P2, P3, P4> > worker;
	};
public:
	MsgCenter() {}
	~MsgCenter() {}

	template<typename P1>
	Worker1<P1> worker(int msgid)
	{
		shared_ptr<IWorker1<P1> > ptr = make_shared<IWorker1<P1> >();
		return static_pointer_cast<IWorker1<P1>>(getAndSetWroker(msgid, ptr));
	}

	template<typename P1, typename P2>
	Worker2<P1, P2> worker(int msgid)
	{
		shared_ptr<IWorker2<P1, P2> > ptr = make_shared<IWorker2<P1, P2> >();
		return static_pointer_cast<IWorker2<P1, P2>>(getAndSetWroker(msgid, ptr));
	}

	template<typename P1, typename P2, typename P3>
	Worker3<P1, P2, P3> worker(int msgid)
	{
		shared_ptr<IWorker3<P1, P2, P3> > ptr = make_shared<IWorker3<P1, P2, P3> >();
		return static_pointer_cast<IWorker3<P1, P2, P3>>(getAndSetWroker(msgid, ptr));
	}

	template<typename P1, typename P2, typename P3, typename P4>
	Worker4<P1, P2, P3, P4> worker(int msgid)
	{
		shared_ptr<IWorker4<P1, P2, P3, P4> > ptr = make_shared<IWorker4<P1, P2, P3, P4> >();
		return static_pointer_cast<IWorker4<P1, P2, P3, P4>>(getAndSetWroker(msgid, ptr));
	}
private:
	Mutex								 mutex;
	std::map<int, shared_ptr< IWorker > > worklist;
};



}
}

