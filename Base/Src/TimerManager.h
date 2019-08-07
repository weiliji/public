#ifndef __TIMERMANAGER_H__
#define __TIMERMANAGER_H__
#include "Base/IntTypes.h"
#include "Base/Thread.h"
#include "Base/Time.h"
#include "Base/Guard.h"
#include "Base/Semaphore.h"
#include "Base/ThreadPool.h"

namespace Public {
namespace Base {

class TimerObject
{
public:
	TimerObject() {}
	virtual ~TimerObject() {}

	virtual bool checkIsNeedRun() = 0;
	virtual bool runFunc() = 0;
	virtual bool reset() = 0;

	uint64_t 			delay;
	uint64_t 			period;
};

class TimerManager : public Thread,public ThreadPool
{
	struct TimerRunInfo
	{
		uint64_t	starttime;
		TimerObject*	timerobj;

		bool operator < (const TimerRunInfo& info) const
		{
			return starttime < info.starttime;
		}
	};
public:
	TimerManager(): Thread("TimerManager", Thread::priorTop), ThreadPool(8)
	{
		curTime = Time::getCurrentMilliSecond();

		createThread();
	}
	/// 析构函数
	~TimerManager()
	{
		destroyThread();

		timerList.clear();
	}
	bool addTimer(TimerObject * pTimer)
	{
		Guard locker(timerMutex);

		std::set<TimerObject*>::iterator iter = timerList.find(pTimer);
		if (iter == timerList.end())
		{
			TimerRunInfo info;
			info.starttime = curTime + pTimer->delay;

			runtimeList.push_back(info);
			runtimeList.sort();
		}

		return true;
	}
	bool removeTimer(TimerObject * pTimer)
	{
		Guard locker(timerMutex);
		timerList.erase(pTimer);

		return true;
	}
	static shared_ptr<TimerManager> getManager()
	{
		static Mutex				  timerMutex;
		static weak_ptr<TimerManager> manager;

		{
			Guard locker(timerMutex);
			shared_ptr<TimerManager> timermanager = manager.lock();
			if (timermanager == NULL)
			{
				timermanager = shared_ptr<TimerManager>(new TimerManager());
				manager = timermanager;
			}

			return timermanager;
		}
	}
private:
	bool resetTimer(TimerObject * pTimer, uint64_t usedTime)
	{
		Guard locker(timerMutex);

		std::set<TimerObject*>::iterator iter = timerList.find(pTimer);
		if (iter != timerList.end())
		{
			(*iter)->reset();
			if ((*iter)->period == 0)
			{
				timerList.erase(iter);
				return false;
			}

			TimerRunInfo info;
			info.starttime = curTime + (*iter)->period;

			runtimeList.push_back(info);
			runtimeList.sort();
		}


		return true;
	}
	void threadProc()
	{
#define MAXTIMERSLEEPTIME		10

		while (looping())
		{
			setTimeout(10000); // 设置超时时间为10秒钟，超时看门狗会重启
			Thread::sleep(10);

			
			{
				Guard locker(timerMutex);

				uint64_t OldTime = curTime;
				curTime = Time::getCurrentMilliSecond();

				// 计时没有改变，可能是因为计时器精度不高
				if (curTime == OldTime)
				{
					continue;
				}

				if (curTime < OldTime)
				{
					//	assert(0); // overflowd
				}

				while (runtimeList.size() > 0)
				{
					TimerRunInfo& info = runtimeList.front();

					if (curTime < info.starttime) break;

					if (!dispatch(ThreadPool::Proc(&TimerManager::timerRunProc, this), info.timerobj))
					{
						assert(0);
					}

					runtimeList.pop_front();
				}
			}
		}
	}
	void timerRunProc(void* param)
	{
		TimerObject * runtimer = (TimerObject *)param;
		if (runtimer != NULL)
		{
			if (!runtimer->checkIsNeedRun()) return;

			uint64_t startTime = Time::getCurrentMilliSecond();
			runtimer->runFunc();
			uint64_t stopTime = Time::getCurrentMilliSecond();
			resetTimer(runtimer, stopTime >= startTime ? stopTime - startTime : 0);
		}
	}
private:
	std::set<TimerObject*> 								timerList;
	std::list<TimerRunInfo>								runtimeList;
	Mutex 												timerMutex;

public:
	uint64_t 										curTime;
};

} // namespace Base
} // namespace Public


#endif //__TIMERMANAGER_H__
