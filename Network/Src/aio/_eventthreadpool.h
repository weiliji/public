#pragma  once
#include "_eventpool.h"

//这里是处理事件的线程池
class EventThreadPool
{
	struct EventInfo
	{
		void* eventid;
		int bytes;
		bool status;
	};
public:
	EventThreadPool(const shared_ptr<EventPool>& _eventpool, uint32_t threadnum)
	{
		eventpool = _eventpool;
		threadsize = threadnum;

		for (uint32_t i = 0; i < threadnum; i++)
		{
			shared_ptr<Thread> thread = ThreadEx::creatThreadEx("EventThreadPool", ThreadEx::Proc(&EventThreadPool::threadProc, this), NULL,Thread::priorTop,Thread::policyRealtime);
			thread->createThread();

			threadlist.push_back(thread);
		}
	}

	~EventThreadPool()
	{
		for (std::list<shared_ptr<Thread> >::iterator iter = threadlist.begin(); iter != threadlist.end(); iter++)
		{
			(*iter)->cancelThread();
		}
		threadlist.clear();
	}

	//存放事件，等待线程池处理
	void postEvent(void* eventid, bool status, int bytes = 0)
	{
		EventInfo* eventinfo = new EventInfo;
		eventinfo->eventid = eventid;
		eventinfo->status = status;
		eventinfo->bytes = bytes;

		if (threadsize >= 0)
		{
			Guard locker(mutex);

			waitDoEventList.push_back(eventinfo);

			eventsem.post();
		}
		else
		{
			doEvent(eventinfo);
		}
	}
private:
	void doEvent(EventInfo* eventinfo)
	{
		shared_ptr<Event> event = eventpool->popEvent(eventinfo->eventid);
		
		if (event)
		{
			event->doEvent(eventinfo->bytes, eventinfo->status);
		}

		SAFE_DELETE(eventinfo);
	}
	void threadProc(Thread* t, void* param)
	{
		while (t->looping())
		{
			eventsem.pend(100);

			EventInfo* eventinfo = NULL;
			{
				Guard locker(mutex);
				if(waitDoEventList.size() <= 0) continue;

				eventinfo = waitDoEventList.front();
				waitDoEventList.pop_front();
			}

			doEvent(eventinfo);
		}
	}
private:
	shared_ptr<EventPool>			eventpool;

	Mutex							mutex;
	Semaphore						eventsem;
	
	std::list<EventInfo*>			waitDoEventList;

	uint32_t					    threadsize;
	std::list<shared_ptr<Thread> >	threadlist;
};
