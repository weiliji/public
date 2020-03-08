#pragma once
#include "_poll.h"
#include "_awaken.h"

//struct AwakenInfo
//{
//	enum
//	{
//		OPT_CREATE,
//		OPT_DESTORY,
//		OPT_ADD,
//		OPT_CLEAN,
//	} opt = OPT_CREATE;
//	EventType type = EventType_Read;
//	SOCKET fd = 0;
//};

struct _ReactorResource : public _PollResource
{
	struct CallEventParam
	{
		weak_ptr<_PollResource> resource;
		shared_ptr<Event> event;
	};

	Mutex mutex;
	std::list<shared_ptr<Event>> readeventlist;
	std::list<shared_ptr<Event>> writeeventlist;
	uint32_t eventtype = EventType_None;

	_ReactorResource(SOCKET socketfd, const shared_ptr<Socket> &sock, const shared_ptr<_Poll> &pool)
		: _PollResource(socketfd, sock, pool)
	{
	}
	virtual ~_ReactorResource()
	{
	}
	virtual void quit()
	{
		_PollResource::quit();
	}

	virtual void _addEvent(EventType type) {}
	virtual void _cleanEvent(EventType type) {}
	virtual shared_ptr<Event> popEvent(EventType type, void *event)
	{
		Guard locker(mutex);
		if (type == EventType_Read)
		{
			for (std::list<shared_ptr<Event>>::iterator iter = readeventlist.begin(); iter != readeventlist.end(); iter++)
			{
				if (iter->get() == event)
				{
					shared_ptr<Event> event = *iter;
					readeventlist.erase(iter);
					return event;
				}
			}
		}
		else if (type == EventType_Write)
		{
			for (std::list<shared_ptr<Event>>::iterator iter = writeeventlist.begin(); iter != writeeventlist.end(); iter++)
			{
				if (iter->get() == event)
				{
					shared_ptr<Event> event = *iter;
					writeeventlist.erase(iter);
					return event;
				}
			}
		}

		return shared_ptr<Event> ();
	}
	virtual bool postEvent(const shared_ptr<Event> &event, uint32_t timeout)
	{
		{
			Guard locker(mutex);
			if (event->type() == EventType_Read)
			{
				if (!(eventtype & EventType_Read))
				{
					_addEvent(EventType_Read);
					eventtype |= EventType_Read;
				}

				readeventlist.push_back(event);
			}
			else if (event->type() == EventType_Write)
			{
				if (!(eventtype & EventType_Write))
				{
					_addEvent(EventType_Write);
					eventtype |= EventType_Write;
				}

				writeeventlist.push_back(event);
			}
		}

		return _PollResource::postEvent(event, timeout);
	}
	void _Reactor_DoEvent(EventType type)
	{
		if (type == EventType_Read)
		{
			shared_ptr<Event> event = doReadEvent();
			if (event)
			{
				CallEventParam *param = new CallEventParam;
				param->event = event;
				param->resource = shared_from_this();

				_poll->postExtExentFunction(IOWorker::EventCallback(eventThreadProc), param);
			}
		}
		else if (type == EventType_Write)
		{
			shared_ptr<Event> event = doWriteEvent();
			if (event)
			{
				CallEventParam *param = new CallEventParam;
				param->event = event;
				param->resource = shared_from_this();

				_poll->postExtExentFunction(IOWorker::EventCallback(eventThreadProc), param);
			}
		}
	}

private:
	shared_ptr<Event> doReadEvent()
	{
		shared_ptr<Event> event;
		do
		{
			Guard locker(mutex);
			if (readeventlist.size() <= 0)
				break;

			event = readeventlist.front();

		} while (0);

		bool result = event && event->doEvent();

		{
			Guard locker(mutex);
			if (result)
			{
				readeventlist.pop_front();
			}

			if (readeventlist.size() <= 0)
			{
				_cleanEvent(EventType_Read);
				eventtype &= ~EventType_Read;
			}
		}

		return result ? event : shared_ptr<Event>();
	}
	shared_ptr<Event> doWriteEvent()
	{
		shared_ptr<Event> event;
		do
		{
			Guard locker(mutex);
			if (writeeventlist.size() <= 0)
				break;

			event = writeeventlist.front();

		} while (0);

		bool result = event && event->doEvent();

		{
			Guard locker(mutex);
			if (result)
			{
				writeeventlist.pop_front();
			}
			if (writeeventlist.size() <= 0)
			{
				_cleanEvent(EventType_Write);
				eventtype &= ~EventType_Write;
			}
		}

		return result ? event : shared_ptr<Event>();
	}

	void doCallEvent(const shared_ptr<Event> &event)
	{
		if (callbackThreadUsedStart())
		{
			event->callEvent(0,true);

			callbackThreadUsedEnd();
		}
	}
	static void eventThreadProc(void *param)
	{
		CallEventParam *callevent = (CallEventParam *)param;
		if (callevent == NULL)
			return;

		shared_ptr<_PollResource> resource = callevent->resource.lock();
		_ReactorResource *reactorres = (_ReactorResource *)resource.get();
		if (resource && reactorres)
			reactorres->doCallEvent(callevent->event);

		SAFE_DELETE(callevent);
	}
};



class _Reactor_Poll : public _Poll
{
	struct ExentInfo
	{
		IOWorker::EventCallback		callback;
		void *						param;
	};

	shared_ptr<ThreadPool>	threadpool;

	Mutex					mutex;
	std::list<ExentInfo>	exentList;
	shared_ptr<Awaken>		awaken;
public:
	_Reactor_Poll(uint32_t num, Thread::Priority pri)
	{
		int threadnum = num - 1;
		
		if (threadnum > 0)
			threadpool = make_shared<ThreadPool>(threadnum, pri);
		else
			awaken = make_shared<Awaken>();
	}
	virtual ~_Reactor_Poll()
	{
		awaken = NULL;
		threadpool = NULL;

		doAwaken();
	}


	virtual bool postExtExentFunction(const IOWorker::EventCallback &_callback, void *_param)
	{
		if (threadpool)
		{
			return threadpool->dispatch(_callback, _param);
		}
		else if (awaken)
		{
			Guard locker(mutex);

			ExentInfo info;
			info.callback = _callback;
			info.param = _param;

			exentList.push_back(info);

			char data[2] ={0,};

			awaken->write_v(data, 1);
		}

		return true;
	}

	SOCKET awakenFd()
	{
		if (awaken == NULL) return 0;

		return awaken->readfd();
	}

	void doAwaken()
	{
		if (awaken == NULL) return;

		awaken->read_v<char>();

		std::list<ExentInfo> cmdlist;
		
		{
			Guard locker(mutex);
			cmdlist = exentList;
			exentList.clear();
		}
		
		
		for (std::list<ExentInfo>::iterator iter = cmdlist.begin(); iter != cmdlist.end(); iter++)
		{
			iter->callback(iter->param);
		}
	}
};
