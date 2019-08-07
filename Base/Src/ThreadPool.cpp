//
//  Copyright (c)1998-2012,  Public Technology
//  All Rights Reserved.
//
//	Description:
//	$Id: ThreadPool.cpp 3 2013年8月21日10:18:30  $
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

class ThreadPoolThread:public Thread
{
public:
	ThreadPoolThread() :Thread("ThreadPoolThread"),prevusedtime(Time::getCurrentMilliSecond()), idelflag(false)
	{
		createThread();
	}
	~ThreadPoolThread()
	{
		cancelThread();
		sem.post();
		destroyThread();
	}
	uint64_t getPrevAliveTime() 
	{
		Guard locker(mutex);
		return prevusedtime; 
	}
	void postEvent(const ThreadPool::Proc& _calblack, void* _param)
	{
		Guard locker(mutex);

		idelflag = false;
		callback = _calblack;
		param = _param;

		sem.post();
	}
	bool idel() 
	{
		Guard locker(mutex);
		return idelflag;
	}
private:
	void threadProc()
	{
		while (looping())
		{
			sem.pend(10000);

			ThreadPool::Proc _callback;
			void* _param;

			{
				Guard locker(mutex);
				if (!callback) continue;

				_callback = callback;
				_param = param;

				callback = NULL;
				param = NULL;

				prevusedtime = Time::getCurrentMilliSecond();
			}

			if (_callback) _callback(_param);

			{
				Guard locker(mutex);
				idelflag = true;
			}
		}
	}
private:
	uint64_t	prevusedtime;
	bool			idelflag;

	Semaphore	sem;
	Mutex		mutex;

	ThreadPool::Proc callback;
	void*			param;
};

#define THREADTIMEOUT	2*60*1000

struct ThreadPool::ThreadPoolInternal
{
	Mutex			mutex;
	std::map<Thread*, shared_ptr< ThreadPoolThread> > threadlist;
	uint32_t		minthread;
	uint32_t		maxthread;
public:
	ThreadPoolInternal(uint32_t threadnum)
	{
		if (threadnum <= 0) threadnum = 1;
		if (threadnum >= 128) threadnum = 128;

		minthread = 1;
		maxthread = threadnum;
	}
	~ThreadPoolInternal()
	{
		threadlist.clear();
	}

	bool dispatch(const ThreadPool::Proc& func, void* param)
	{
		std::list< shared_ptr< ThreadPoolThread> > freelist;

		shared_ptr< ThreadPoolThread> canusedthread;
		{

			Guard locker(mutex);

			//查找能用的线程
			for (std::map<Thread*, shared_ptr< ThreadPoolThread> >::iterator iter = threadlist.begin(); iter != threadlist.end(); iter++)
			{
				if (iter->second->idel())
				{
					canusedthread = iter->second;
					break;
				}
			}

			//释放过期的线程
			uint64_t nowtime = Time::getCurrentMilliSecond();
			for (std::map<Thread*, shared_ptr< ThreadPoolThread> >::iterator iter = threadlist.begin(); iter != threadlist.end() && threadlist.size() > minthread; )
			{
				uint64_t threadusedtime = iter->second->getPrevAliveTime();
				if (iter->second->idel() && iter->second.get() != canusedthread.get() && nowtime > threadusedtime && nowtime - threadusedtime >= THREADTIMEOUT)
				{
					freelist.push_back(iter->second);
					threadlist.erase(iter++);
				}
				else
				{
					iter++;
				}
			}

			//如果没线程，分配线程 
			if (canusedthread == NULL && threadlist.size() < maxthread)
			{
				canusedthread = make_shared<ThreadPoolThread>();
				threadlist[canusedthread.get()] = canusedthread;
			}

			if (canusedthread) canusedthread->postEvent(func, param);
		}

		return canusedthread != NULL;
	}
	void* threadpoolHandle() const { return NULL; }
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


