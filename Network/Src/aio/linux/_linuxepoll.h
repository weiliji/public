#pragma  once

#ifndef WIN32
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/types.h>   
#include <sys/socket.h>

#include "../common/_Pool.h"

#define EVENT_INIT		0
#define EVENT_ERROR		(EPOLLHUP | EPOLLERR)
#define EVENT_READ		(EPOLLIN | EPOLLPRI | EVENT_ERROR)
#define EVENT_WRITE		(EPOLLOUT)

#define EVENTISREAD(event) (event&EPOLLIN  || event&EPOLLPRI)
#define EVENTISWRITE(event) (event&EPOLLOUT)
#define EVENTISERROR(event) (event&EPOLLHUP  || event&EPOLLERR)

class _SystemPoll :public _Pool,public Thread,public enable_shared_from_this<_SystemPoll>
{
	struct _EPollResource:public _PoolResource
	{
		int		event;
		shared_ptr<Event>	readevent;
		shared_ptr<Event>	writeevent;

		_EPollResource(int socketfd, const shared_ptr<Socket>& sock, const shared_ptr<_UserThread>& userthread, const shared_ptr<_Pool>& pool)
			:_PoolResource(socketfd,sock,userthread,pool),event(EVENT_INIT){}
	};

	struct _DoThreadPoolInfo
	{
		shared_ptr<Event>	event;
	};
	
	int 										epoll;
	Mutex										mutex;
	std::map<int, shared_ptr<_EPollResource> >	eventmap;
public:
	_SystemPoll(uint32_t maxthreadnum) :_Pool(maxthreadnum - 1), Thread("_SystemPool"), epoll(0)
	{
		epoll = ::epoll_create(10000);
		if (epoll <= 0) return;

		createThread();
	}
	~_SystemPoll()
	{
		destroyThread();

		if (epoll > 0) close(epoll);
		epoll = 0;
	}

	virtual shared_ptr<_PoolResource> addResource(int socketfd, const shared_ptr<Socket>& _sock, const shared_ptr<_UserThread>& _userthread)
	{
		shared_ptr< _EPollResource> eventinfo = make_shared<_EPollResource>(socketfd, _sock, _userthread, shared_from_this());
		{
			Guard locker(mutex);
			eventmap[socketfd] = eventinfo;
		}
		{
			epoll_event pevent;
			memset(&pevent, 0, sizeof(pevent));
			pevent.data.fd = socketfd;
			pevent.events = eventinfo->event;

			::epoll_ctl(epoll, EPOLL_CTL_ADD, socketfd, &pevent);
		}
		return eventinfo;
	}

	virtual bool delResource(int socketfd, _PoolResource* res) 
	{
		{
			Guard locker(mutex);
			eventmap.erase(socketfd);
		}
		{
			epoll_event pevent;
			memset(&pevent, 0, sizeof(pevent));

			::epoll_ctl(epoll, EPOLL_CTL_DEL, socketfd, &pevent);
		}
		return true; 
	}

	virtual bool postEvent(const shared_ptr<_PoolResource>& res, const shared_ptr<Event>& event)
	{
		_EPollResource* epollres = (_EPollResource*)res.get();
		if (res == NULL || epollres == NULL)
		{
			return false;
		}
		if (epollres->event & ((event->type() == EventType_Read) ? EVENT_READ : EVENT_WRITE))
		{
			return false;
		}
		if (event->type() == EventType_Read)
		{
			epollres->readevent = event;
			epollres->event |= EVENT_READ;
		}
		else if (event->type() == EventType_Write)
		{
			epollres->writeevent = event;
			epollres->event |= EVENT_WRITE;
		}

		{
			epoll_event pevent;
			memset(&pevent, 0, sizeof(pevent));
			pevent.data.fd = res->socket();
			pevent.events = epollres->event;

			::epoll_ctl(epoll, EPOLL_CTL_MOD, res->socket(), &pevent);
		}

		return true;
	}

	virtual void clean(int sockfd, EventType type,shared_ptr<_EPollResource>& eventinfo)
	{
		if (epoll <= 0) return;

		if (type == EventType_Read)
		{
			eventinfo->event &= ~ EVENT_READ;
			eventinfo->readeventid = NULL;
		}
		else
		{
			eventinfo->event &= ~ EVENT_WRITE;
			eventinfo->writeeventid = NULL;
		}

		epoll_event pevent;
		memset(&pevent, 0, sizeof(pevent));
		pevent.data.fd = sockfd;
		pevent.events = eventinfo->event;

		::epoll_ctl(epoll, EPOLL_CTL_MOD, sockfd, &pevent);
	}
	void threadProc()
	{
		if (epoll <= 0) return;

#define MAXEVENTNUM			256	
		epoll_event workEpoolEvent[MAXEVENTNUM];

		while (looping())
		{
			int pollSize = ::epoll_wait(epoll, workEpoolEvent, MAXEVENTNUM, 100);
			if (pollSize <= 0) return;

			for (int i = 0; i < pollSize; i++)
			{
				int sockfd = workEpoolEvent[i].data.fd;

				shared_ptr< _EPollResource> eventinfo = getEventInfo(sockfd);
				if (eventinfo == NULL)	continue;
				
				if (EVENTISERROR(workEpoolEvent[i].events))
				{
					_DoThreadPoolInfo* info = new _DoThreadPoolInfo;
					info->event = eventinfo->readevent;
					eventinfo->readevent = NULL;
					clean(sockfd, EventType_Read, eventinfo);

					if (!ThreadPool::dispatch(ThreadPool::Proc(&_SystemPoll::eventThreadProc, shared_from_this()), info))
					{
						assert(0);
					}
				}
				if (EVENTISREAD(workEpoolEvent[i].events))
				{
					_DoThreadPoolInfo* info = new _DoThreadPoolInfo;
					info->event = eventinfo->readeventid;
					eventinfo->readeventid = NULL;
					clean(sockfd, EventType_Read, eventinfo);

					if (!ThreadPool::dispatch(ThreadPool::Proc(&_SystemPoll::eventThreadProc, shared_from_this()), info))
					{
						assert(0);
					}
				}

				if (EVENTISWRITE(workEpoolEvent[i].events))
				{
					_DoThreadPoolInfo* info = new _DoThreadPoolInfo;
					info->event = eventinfo->writeeventid;
					eventinfo->writeeventid = NULL;
					clean(sockfd, EventType_Write, eventinfo);

					if (!ThreadPool::dispatch(ThreadPool::Proc(&_SystemPoll::eventThreadProc, shared_from_this()), info))
					{
						assert(0);
					}
				}
			}
		}
	}
private:
	shared_ptr< _EPollResource> getEventInfo(int socketfd)
	{
		Guard((Mutex&)mutex);

		std::map<int, shared_ptr< _EPollResource> >::iterator iter = eventmap.find(socketfd);
		if (iter == eventmap.end()) return iter->second;

		return iter->second;
	}	
	void eventThreadProc(void* param)
	{
		_DoThreadPoolInfo* info = (_DoThreadPoolInfo*)param;
		if (info == NULL) return;

		if(info->event)
			info->event->doEvent1(0, 0);

		SAFE_DELETE(info);
	}
};

#endif