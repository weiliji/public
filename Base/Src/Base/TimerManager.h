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
		uint64_t				starttime;
		weak_ptr<TimerObject>	timerobj;

		bool operator < (const TimerRunInfo& info) const
		{
			return starttime < info.starttime;
		}
	};
	struct TimerRunThreadInfo
	{
		weak_ptr<TimerObject> timerobj;
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
	bool addTimer(const shared_ptr<TimerObject>& pTimer)
	{
		Guard locker(timerMutex);

		std::map<TimerObject*, weak_ptr<TimerObject>>::iterator iter = timerList.find(pTimer.get());
		if (iter == timerList.end())
		{
			TimerRunInfo info;
			info.starttime = curTime + pTimer->delay;
			info.timerobj = pTimer;

			runtimeList.push_back(info);
			runtimeList.sort();

			timerList[pTimer.get()] = pTimer;
		}

		return true;
	}
	bool removeTimer(TimerObject* pTimer)
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
	bool resetTimer(const shared_ptr<TimerObject>& pTimer)
	{
		Guard locker(timerMutex);

		std::map<TimerObject*, weak_ptr<TimerObject>>::iterator iter = timerList.find(pTimer.get());
		if (iter != timerList.end())
		{
			shared_ptr<TimerObject> timerobj = iter->second.lock();

			if (timerobj)
			{
				timerobj->reset();
				if (timerobj->period == 0)
				{
					timerList.erase(iter);
					return false;
				}

				TimerRunInfo info;
				info.starttime = curTime + timerobj->period;
				info.timerobj = pTimer;

				runtimeList.push_back(info);
				runtimeList.sort();

			}
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

					do 
					{
						shared_ptr< TimerObject> timerobj = info.timerobj.lock();

						std::map<TimerObject*, weak_ptr<TimerObject>>::iterator titer = timerList.find(timerobj.get());
						if (titer == timerList.end()) break;
						

						if (!timerobj->checkIsNeedRun()) break;

						TimerRunThreadInfo* threadinfo = new TimerRunThreadInfo;
						threadinfo->timerobj = timerobj;

						if (!dispatch(ThreadPool::Proc(&TimerManager::timerRunProc, this), threadinfo))
						{
							assert(0);
							break;
						}

					} while (0);					

					runtimeList.pop_front();
				}
			}
		}
	}
	void timerRunProc(void* param)
	{
		TimerRunThreadInfo* threadinfo = (TimerRunThreadInfo*)param;
		if (threadinfo == NULL) return;

		shared_ptr<TimerObject> runtimer = threadinfo->timerobj.lock();
		
		if (runtimer != NULL)
		{
			runtimer->runFunc();

			resetTimer(runtimer);
		}

		SAFE_DELETE(threadinfo);
	}
private:
	std::map<TimerObject*,weak_ptr<TimerObject>>		timerList;
	std::list<TimerRunInfo>								runtimeList;
	Mutex												timerMutex;

public:
	uint64_t 										curTime;
};

} // namespace Base
} // namespace Public


#endif //__TIMERMANAGER_H__
