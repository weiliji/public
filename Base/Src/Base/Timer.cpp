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
UINT wTimerRes;
#elif defined(__linux__) || defined(__APPLE__)
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

namespace Public
{
namespace Base
{

struct TimerInternalObject : public TimerObject, public enable_shared_from_this<TimerObject>
{
	Timer::Proc fun;
	unsigned long param;
	std::string name;

	int calledId;
	bool called;
	bool started;
	Mutex mutex;
	Semaphore *quitsem;
	shared_ptr<TimerManager> manager;

public:
	TimerInternalObject()
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
	~TimerInternalObject()
	{
		stopandwait();
	}

	bool checkIsNeedRun()
	{
		//uint64_t curTime = manager->curTime;

		{
			Guard locker(mutex);

			if (called || !started)
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

struct Timer::TimerInternal
{
	shared_ptr<TimerInternalObject> pTimer;
};

Timer::Timer(const std::string &pName)
{
	internal = new TimerInternal();
	internal->pTimer = make_shared<TimerInternalObject>();

	setName(pName);
}

Timer::~Timer()
{
	stopAndWait();

	delete internal;
}

bool Timer::start(const Proc &fun, uint32_t delay, uint32_t period, unsigned long param /* = 0 */)
{
	internal->pTimer->fun = fun;
	internal->pTimer->delay = (uint64_t)delay;
	internal->pTimer->period = period;
	internal->pTimer->param = param;
	internal->pTimer->started = true;

	internal->pTimer->manager->addTimer(internal->pTimer);

	return true;
}

bool Timer::stop(bool bCallNow /* = false */)
{
	Guard locker(internal->pTimer->mutex);
	internal->pTimer->started = false;
	internal->pTimer->fun = NULL;

	return true;
}

bool Timer::stopAndWait()
{
	internal->pTimer->stopandwait();

	return true;
}

std::string Timer::getName()
{
	return internal->pTimer->name;
}

void Timer::setName(const std::string &pszName)
{
	internal->pTimer->name = pszName;
}

bool Timer::isStarted()
{
	return internal->pTimer->started;
}

bool Timer::isCalled()
{
	return internal->pTimer->called;
}

bool Timer::isRunning()
{
	return internal->pTimer->started;
}

} // namespace Base
} // namespace Public
