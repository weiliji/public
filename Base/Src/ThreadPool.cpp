//
//  Copyright (c)1998-2012,  Public Technology
//  All Rights Reserved.
//
//	Description:
//	$Id: ThreadPool.cpp 3 2013Äê8ÔÂ21ÈÕ10:18:30  $
#include <string.h>
#include "Base/ThreadPool.h"
#include "Base/Semaphore.h"
#include "Base/BaseTemplate.h"
#include "Base/Shared_ptr.h"
#include "Base/Guard.h"
#include "Base/Time.h"
#include "Base/PrintLog.h"
#include "Base/ThreadEx.h"
namespace Public{
namespace Base{

#ifdef WIN32

class _WinThreadPoolCallbackInfo
{
public:
	_WinThreadPoolCallbackInfo(const ThreadPool::Proc& _callback, void*	 _param)
	{
		param = _param;
		callback = _callback;
	}
	~_WinThreadPoolCallbackInfo() {}
public:
	static VOID CALLBACK CALLBACKPTP_SIMPLE_CALLBACK(PTP_CALLBACK_INSTANCE Instance, PVOID Context)
	{
		_WinThreadPoolCallbackInfo* callbackinfo = (_WinThreadPoolCallbackInfo*)Context;
		if (callbackinfo != NULL)
		{
			callbackinfo->callback(callbackinfo->param);
		}
		SAFE_DELETE(callbackinfo);
	}
private:
	void*				param;
	ThreadPool::Proc	callback;
};

struct ThreadPool::ThreadPoolInternal
{
	TP_CLEANUP_GROUP*		tp_clean;
	PTP_POOL				tp_pool;
	TP_CALLBACK_ENVIRON		tp_env;


	ThreadPoolInternal(uint32_t threadnum)
	{
		tp_pool = CreateThreadpool(NULL);

		SetThreadpoolThreadMinimum(tp_pool, 1);
		SetThreadpoolThreadMaximum(tp_pool, threadnum <= 1 ? 1 : threadnum);

		InitializeThreadpoolEnvironment(&tp_env);
		SetThreadpoolCallbackPool(&tp_env, tp_pool);
		tp_clean = CreateThreadpoolCleanupGroup();;

		SetThreadpoolCallbackCleanupGroup(&tp_env, tp_clean, NULL);
	}
	~ThreadPoolInternal()
	{
		CloseThreadpoolCleanupGroupMembers(tp_clean, false, NULL);
		CloseThreadpoolCleanupGroup(tp_clean);
		DestroyThreadpoolEnvironment(&tp_env);
		CloseThreadpool(tp_pool);

		tp_clean = NULL;
	}

	bool dispatch(const ThreadPool::Proc& func, void* param)
	{
		_WinThreadPoolCallbackInfo* info = new _WinThreadPoolCallbackInfo(func, param);

		bool ret =  TrySubmitThreadpoolCallback(_WinThreadPoolCallbackInfo::CALLBACKPTP_SIMPLE_CALLBACK, info, &tp_env);
		if (!ret)
		{
			SAFE_DELETE(info);
			return false;
		}

		return true;
	}
	void* threadpoolHandle() const
	{
		return (void*)&tp_env;
	}
};

#else
struct ThreadPool::ThreadPoolInternal
{
	struct ThreadPoolInfo
	{
		ThreadPool::Proc callback;
		void*			param;
	};
public:
	ThreadPoolInternal(uint32_t threadnum)
	{
		if (threadnum <= 0) threadnum = 1;
		if (threadnum >= 128) threadnum = 128;

		for (uint32_t i = 0; i < threadnum; i++)
		{
			shared_ptr<Thread> thread = ThreadEx::creatThreadEx("ThreadPoolInternal", ThreadEx::Proc(&ThreadPoolInternal::threadProc, this), NULL);
			thread->createThread();

			threadlist.push_back(thread);
		}
	}
	~ThreadPoolInternal()
	{
		for (std::list<shared_ptr<Thread> >::iterator iter = threadlist.begin(); iter != threadlist.end(); iter++)
		{
			(*iter)->cancelThread();
		}
		threadlist.clear();
	}

	bool dispatch(const ThreadPool::Proc& func, void* param)
	{
		ThreadPoolInfo info;

		info.callback = func;
		info.param = param;

		{
			Guard locker(mutex);

			waitDoEventList.push_back(info);

			eventsem.post();
		}

		return true;
	}
	void* threadpoolHandle() const { return NULL; }
private:
	void threadProc(Thread* t, void* param)
	{
		while (t->looping())
		{
			eventsem.pend(100);

			ThreadPoolInfo info;
			{
				Guard locker(mutex);
				if (waitDoEventList.size() <= 0) continue;

				info = waitDoEventList.front();
				waitDoEventList.pop_front();
			}

			info.callback(info.param);
		}
	}
private:
	Mutex							mutex;
	Semaphore						eventsem;

	std::list<ThreadPoolInfo>		waitDoEventList;

	uint32_t					    threadsize;
	std::list<shared_ptr<Thread> >	threadlist;
};
#endif

ThreadPool::ThreadPool(uint32_t threadnum)
{
	internal = new ThreadPoolInternal(threadnum);
}

ThreadPool::~ThreadPool()
{
	SAFE_DELETE(internal);
}

bool ThreadPool::dispatch(const ThreadPool::Proc& func,void* param)
{
	return internal->dispatch(func,param);
}
void* ThreadPool::threadpoolHandle() const
{
	return internal->threadpoolHandle();
}
};//Base
};//Public


