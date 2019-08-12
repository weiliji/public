//
//  Copyright (c)1998-2012,  Public Technology
//  All Rights Reserved.
//
//	Description:
//	$Id: Timer.cpp 56 2013-03-08 01:47:53Z  $
//



#include <string.h>
#include <assert.h>
#include <algorithm>
#ifdef WIN32
	#include <Windows.h>
	#include <Mmsystem.h>
	#include <stdio.h>
#ifndef snprintf
#define snprintf _snprintf
#endif
	UINT     wTimerRes;
#elif defined(__linux__)
	#include <stdio.h>
	#include <errno.h>
	#include <time.h>
	#include <signal.h>
	#include <sys/time.h>
#else
	#error "Unknown Platform"
#endif

#include "Base/Time.h"
#include "Base/Timer.h"
#include "Base/PrintLog.h"
#include "Base/Thread.h"
#include "Base/Semaphore.h"
#include "Base/Guard.h"
#include "Base/BaseTemplate.h"

namespace Public{
namespace Base{

struct TimerWaitInfo
{
	std::string  		name;
	Timer::Proc 		fun;
	unsigned long 		param;

	int					calledId;
	bool 				called;
	bool				started;
	Mutex 				mutex;
	Semaphore*			quitsem;

	TimerWaitInfo():param(0),calledId(0),called(false),started(false),quitsem(NULL){}

	~TimerWaitInfo() { stopandwait(); }
	void stopandwait()
	{
		{
			Guard locker(mutex);
			SAFE_DELETE(quitsem);
			started = false;
			if (called /*&& calledId != 0 && calledId != Thread::getCurrentThreadID()*/)
			{
				quitsem = new Semaphore();
			}
		}

		if (quitsem != NULL)
		{
			quitsem->pend();
		}
		{
			Guard locker(mutex);
			called = false;
			SAFE_DELETE(quitsem);
		}
	}
	bool startRunFunc()
	{
		Guard locker(mutex);

		if (called || !started)
		{
			return false;
		}
		called = true;
		calledId = Thread::getCurrentThreadID();

		return true;
	}

	bool stopRunFunc()
	{
		Guard locker(mutex);
		called = false;
		calledId = 0;
		if (quitsem != NULL)
		{
			quitsem->post();
		}

		return true;
	}
};


class TimerWaitInfoManager
{
public:
	TimerWaitInfoManager() {}
	~TimerWaitInfoManager() {}

	void insert(void* flag, const shared_ptr<TimerWaitInfo>& timerinfo)
	{
		Guard locker(mutex);
		timerlist[flag] = timerinfo;
	}
	void erase(void* flag)
	{
		Guard locker(mutex);
		timerlist.erase(flag);
	}
	shared_ptr<TimerWaitInfo> getAndRun(void* flag)
	{
		std::map<void*, shared_ptr< TimerWaitInfo> >::iterator iter = timerlist.find(flag);
		if (iter == timerlist.end()) return make_shared<TimerWaitInfo>();

		iter->second->startRunFunc();

		return iter->second;
	}

	static TimerWaitInfoManager* instance()
	{
		static TimerWaitInfoManager manager;
		return &manager;
	}
private:
	Mutex	mutex;
	std::map<void*, shared_ptr< TimerWaitInfo> >timerlist;
};


#ifdef WIN32

struct TimerQueue
{
public:
	HANDLE hTimerQueue;

	TimerQueue()
	{
		hTimerQueue = CreateTimerQueue();
		if (NULL == hTimerQueue)
		{
			assert(0);
		}
	}
	~TimerQueue()
	{
		if (hTimerQueue != NULL)
		{
			DeleteTimerQueue(hTimerQueue);
			hTimerQueue = NULL;
		}
	}

	static TimerQueue* instance()
	{
		static TimerQueue queue;
		return &queue;
	}
};

struct Timer::TimerInternal
{
	shared_ptr<TimerWaitInfo> waitinfo;
	HANDLE				hTimer;
public:
	TimerInternal()
	{
		waitinfo = make_shared<TimerWaitInfo>();
		hTimer = NULL;
	}
	~TimerInternal()
	{
		stop();
		waitinfo->stopandwait();
		waitinfo = NULL;
	}

	bool start(const Proc& _fun, uint32_t delay, uint32_t period, unsigned long _param /* = 0 */)
	{
		if (hTimer != NULL || waitinfo->started) return false;

		waitinfo->fun = _fun;
		waitinfo->param = _param;

		waitinfo->started = true;

		TimerWaitInfoManager::instance()->insert(this, waitinfo);

		if (!CreateTimerQueueTimer(&hTimer, TimerQueue::instance()->hTimerQueue, TimerRoutine, this, delay, period, WT_EXECUTEDEFAULT))
		{
			assert(0);
		}

		return true;
	}

	bool stop()
	{
		if (hTimer == NULL || !waitinfo->started) return false;

		waitinfo->started = false;

		DeleteTimerQueueTimer(hTimer, TimerQueue::instance()->hTimerQueue, INVALID_HANDLE_VALUE);
		hTimer = NULL;

		return true;
	}

	static VOID CALLBACK TimerRoutine(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
	{
		shared_ptr<TimerWaitInfo> timerinfo = TimerWaitInfoManager::instance()->getAndRun(lpParameter);
		if (timerinfo == NULL) return;

		timerinfo->fun(timerinfo->param);

		timerinfo->stopRunFunc();
	}
};

#else
struct Timer::TimerInternal
{
	shared_ptr<TimerWaitInfo> waitinfo;
	timer_t			timerid;
public:
	TimerInternal()
	{
		waitinfo = make_shared<TimerWaitInfo>();
		timerid = 0;
	}
	~TimerInternal()
	{
		stop();
		waitinfo->stopandwait();
		waitinfo = NULL;
	}

	bool start(const Proc& _fun, uint32_t delay, uint32_t period, unsigned long _param /* = 0 */)
	{
		if (timerid != 0 || waitinfo->started) return false;

		waitinfo->fun = _fun;
		waitinfo->param = _param;

		waitinfo->started = true;

		TimerWaitInfoManager::instance()->insert(this, waitinfo);

		struct sigevent evp;
		memset(&evp, 0, sizeof(struct sigevent));
		evp.sigev_value.sival_ptr = this;            //也是标识定时器的，这和timerid有什么区别？回调函数可以获得
		evp.sigev_notify = SIGEV_THREAD;            //线程通知的方式，派驻新线程
		evp.sigev_notify_function = timer_thread;        //线程函数地址

		if (timer_create(CLOCK_REALTIME, &evp, &timerid) == -1)
		{
			assert(0);
		}

		// XXX int timer_settime(timer_t timerid, int flags, const struct itimerspec *new_value,struct itimerspec *old_value);
	// timerid--定时器标识
	// flags--0表示相对时间，1表示绝对时间
	// new_value--定时器的新初始值和间隔，如下面的it
	// old_value--取值通常为0，即第四个参数常为NULL,若不为NULL，则返回定时器的前一个值

	//第一次间隔it.it_value这么长,以后每次都是it.it_interval这么长,就是说it.it_value变0的时候会装载it.it_interval的值
		struct itimerspec it;
		memset(&it, 0, sizeof(it));

		it.it_interval.tv_sec = period / 1000;
		it.it_interval.tv_nsec = (period % 1000)*1000;
		it.it_value.tv_sec = delay/1000;
		it.it_value.tv_nsec = (delay%1000)*1000;

		if (timer_settime(timerid, 0, &it, NULL) == -1)
		{
			assert(0);
		}

		return true;
	}

	bool stop()
	{
		if (timerid == 0 || !waitinfo->started) return false;

		waitinfo->started = false;

		timer_delete(timerid);
		timerid = 0;


		return true;
	}

	static void timer_thread(union sigval v)
	{
		shared_ptr<TimerWaitInfo> timerinfo = TimerWaitInfoManager::instance()->getAndRun(v.sival_ptr);
		if (timerinfo == NULL) return;

		timerinfo->fun(timerinfo->param);

		timerinfo->stopRunFunc();
	}
};

#endif


Timer::Timer(const std::string& pName)
{
	internal = new TimerInternal();

	internal->waitinfo->name = pName;
}

Timer::~Timer()
{
	stopAndWait();

	delete internal;
}

bool Timer::start(const Proc& fun, uint32_t delay, uint32_t period, unsigned long param /* = 0 */)
{
	return internal->start(fun, delay, period, param);
}

bool Timer::stop(bool bCallNow /* = false */)
{
	return internal->stop();
}
 
bool Timer::stopAndWait()
{
	internal->waitinfo->stopandwait();
	
	return true;
}

std::string Timer::getName()
{
	return internal->waitinfo->name;
}

void Timer::setName(const std::string& pszName)
{
	internal->waitinfo->name = pszName;
}

bool Timer::isStarted()
{
	return internal->waitinfo->started;
}

bool Timer::isCalled()
{
	return internal->waitinfo->called;
}

bool Timer::isRunning()
{
	return internal->waitinfo->started;
}



} // namespace Base
} // namespace Public

