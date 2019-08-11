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

	TimerWaitInfo():calledId(0),called(false),started(false),quitsem(NULL){}

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

struct Timer::TimerInternal:public TimerWaitInfo
{
	HANDLE				hTimer;
public:
	TimerInternal()
	{
		param = 0;
		hTimer = NULL;
	}
	~TimerInternal()
	{
		stop();
	}

	bool start(const Proc& _fun, uint32_t delay, uint32_t period, unsigned long _param /* = 0 */)
	{
		if (hTimer != NULL || started) return false;

		fun = _fun;
		param = _param;

		started = true;
		if (!CreateTimerQueueTimer(&hTimer, TimerQueue::instance()->hTimerQueue, TimerRoutine, this, delay, period, WT_EXECUTEDEFAULT))
		{
			assert(0);
		}

		return true;
	}

	bool stop()
	{
		if (hTimer == NULL || !started) return false;

		started = false;

		DeleteTimerQueueTimer(hTimer, TimerQueue::instance()->hTimerQueue, INVALID_HANDLE_VALUE);
		hTimer = NULL;

		return true;
	}

	static VOID CALLBACK TimerRoutine(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
	{
		TimerInternal* internal = (TimerInternal*)lpParameter;
		if (internal == NULL) return;


		{
			Guard locker(internal->mutex);

			if (internal->called || !internal->started)
			{
				return;
			}
			internal->called = true;
			internal->calledId = Thread::getCurrentThreadID();
		}

		internal->fun(internal->param);

		{
			Guard locker(internal->mutex);
			internal->called = false;
			internal->calledId = 0;
			if (internal->quitsem != NULL)
			{
				internal->quitsem->post();
			}
		}
	}
};

#else
struct Timer::TimerInternal :public TimerWaitInfo
{
	timer_t			timerid;
public:
	TimerInternal()
	{
		param = 0;
		timerid = 0;
	}
	~TimerInternal()
	{
		stop();
	}

	bool start(const Proc& _fun, uint32_t delay, uint32_t period, unsigned long _param /* = 0 */)
	{
		if (timerid != 0 || started) return false;

		fun = _fun;
		param = _param;

		started = true;

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
		if (timerid == 0 || !started) return false;

		started = false;

		timer_delete(timerid);
		timerid = 0;


		return true;
	}

	static void timer_thread(union sigval v)
	{
		TimerInternal* internal = (TimerInternal*)v.sival_ptr;
		if (internal == NULL) return;


		{
			Guard locker(internal->mutex);

			if (internal->called || !internal->started)
			{
				return;
			}
			internal->called = true;
			internal->calledId = Thread::getCurrentThreadID();
		}

		internal->fun(internal->param);

		{
			Guard locker(internal->mutex);
			internal->called = false;
			internal->calledId = 0;
			if (internal->quitsem != NULL)
			{
				internal->quitsem->post();
			}
		}
	}
};

#endif


Timer::Timer(const std::string& pName)
{
	internal = new TimerInternal();

	internal->name = pName;
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
	internal->stopandwait();
	
	return true;
}

std::string Timer::getName()
{
	return internal->name;
}

void Timer::setName(const std::string& pszName)
{
	internal->name = pszName;
}

bool Timer::isStarted()
{
	return internal->started;
}

bool Timer::isCalled()
{
	return internal->called;
}

bool Timer::isRunning()
{
	return internal->started;
}



} // namespace Base
} // namespace Public

