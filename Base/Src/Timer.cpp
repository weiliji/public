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
#include "TimerManager.h"

namespace Public{
namespace Base{



struct Timer::TimerInternal:public TimerObject
{
	Timer::Proc 		fun;
	unsigned long 		param;
	std::string  		name;

	int					calledId;
	bool 				called;
	bool				started;
	Mutex 				mutex;
	Semaphore*			quitsem;
	shared_ptr<TimerManager> manager;
public:
	TimerInternal()
	{
		delay = 0;
		period = 0;
		param = 0;
		called = false;
		started = false;
		calledId = 0;
		quitsem = NULL;
		manager = TimerManager::getManager();
	}
	~TimerInternal()
	{
		stopandwait();
	}

	bool checkIsNeedRun()
	{
		uint64_t curTime = manager->curTime;

		{
			Guard locker(mutex);

			if (called  || !started)
			{
				return false;
			}
			called = true;
		}

		return true;
	}

	bool reset()
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


	void stopandwait()
	{
		{
			Guard locker(mutex);
			SAFE_DELETE(quitsem);
			started = false;
			if (called && calledId != 0 && calledId != Thread::getCurrentThreadID())
			{
				quitsem = new Semaphore();
			}
		}

		if (quitsem != NULL)
		{
			quitsem->pend();
		}
		manager->removeTimer(this);
		
		{
			Guard locker(mutex);
			called = false;
			SAFE_DELETE(quitsem);
		}
	}
	bool runFunc()
	{
		{
			Guard locker(mutex);
			calledId = Thread::getCurrentThreadID();
		}
		
		fun(param);

		return true;
	}
};

Timer::Timer(const std::string& pName)
{
	internal = new TimerInternal();

	setName(pName);
}

Timer::~Timer()
{
	stopAndWait();

	delete internal;
}

bool Timer::start(const Proc& fun, uint32_t delay, uint32_t period, unsigned long param /* = 0 */)
{
	internal->fun = fun;
	internal->delay = (uint64_t)delay;
	internal->period = period;
	internal->param = param;
	internal->started = true;

	internal->manager->addTimer(internal);

	return true;
}

bool Timer::stop(bool bCallNow /* = false */)
{
	Guard locker(internal->mutex);
	internal->started = false;

	return true;
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

