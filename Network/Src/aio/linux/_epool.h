#pragma  once
#include "../common/_Pool.h"
#include "../common/_EventThreadPool.h"

#ifndef WIN32
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/types.h>   
#include <sys/socket.h>

#define EVENT_INIT		0
#define EVENT_ERROR		(EPOLLHUP | EPOLLERR)
#define EVENT_READ		(EPOLLIN | EPOLLPRI | EVENT_ERROR)
#define EVENT_WRITE		(EPOLLOUT)

#define EVENTISREAD(event) (event&EPOLLIN  || event&EPOLLPRI)
#define EVENTISWRITE(event) (event&EPOLLOUT)
#define EVENTISERROR(event) (event&EPOLLHUP  || event&EPOLLERR)

class _SystemPool :public _Pool
{
	struct EpoolEventInfo
	{
		int		event;
		void*	readeventid;
		void*	writeeventid;

		EpoolEventInfo():event(EVENT_INIT),readeventid(NULL),writeeventid(NULL){}
	};
	int 										epoll;
	Mutex										mutex;
	std::map<int, shared_ptr< EpoolEventInfo> >	 eventmap;
public:
	_SystemPool() :epoll(0){}
	~_SystemPool(){}

	virtual bool start(_EventThreadPool*_eventpool, uint32_t threadnum)
	{
		epoll = ::epoll_create(10000);
		if (epoll <= 0) return false;

		return _Pool::start(_eventpool, threadnum);
	}
	virtual bool stop()
	{
		if (epoll > 0) close(epoll);
		epoll = 0;

		return _Pool::stop();
	}
	virtual void create(int sockfd)
	{
		if (epoll <= 0) return;


		shared_ptr< EpoolEventInfo> eventinfo = make_shared<EpoolEventInfo>();
		addEventInfo(sockfd, eventinfo);

		epoll_event pevent;
		memset(&pevent, 0, sizeof(pevent));
		pevent.data.fd = sockfd;
		pevent.events = eventinfo->event;

		::epoll_ctl(epoll, EPOLL_CTL_ADD, sockfd, &pevent);
	}
	virtual void destory(int sockfd)
	{
		if (epoll <= 0) return;

		epoll_event pevent;
		memset(&pevent, 0, sizeof(pevent));

		::epoll_ctl(epoll, EPOLL_CTL_DEL, sockfd, &pevent);

		delEventInfo(sockfd);
	}
	virtual void add(int sockfd, PoolType type, void* eventid)
	{
		if (epoll <= 0) return;

		shared_ptr< EpoolEventInfo> eventinfo = getEventInfo(sockfd);
		if (eventinfo == NULL)	return;

		if (type == PoolType_Read)
		{
			eventinfo->event |= EVENT_READ;
			eventinfo->readeventid = eventid;
		}
		else
		{
			eventinfo->event |= EVENT_WRITE;
			eventinfo->writeeventid = eventid;
		}
		
		epoll_event pevent;
		memset(&pevent, 0, sizeof(pevent));
		pevent.data.fd = sockfd;
		pevent.events = eventinfo->event;

		::epoll_ctl(epoll, EPOLL_CTL_MOD, sockfd, &pevent);
	}
	virtual void clean(int sockfd, PoolType type,shared_ptr<EpoolEventInfo>& eventinfo)
	{
		if (epoll <= 0) return;

		if (type == PoolType_Read)
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
	virtual void runPool()
	{
		if (epoll <= 0) return;

#define MAXEVENTNUM			256	
		epoll_event workEpoolEvent[MAXEVENTNUM];

		int pollSize = ::epoll_wait(epoll, workEpoolEvent, MAXEVENTNUM, 1000);
		if (pollSize <= 0) return;

		for (int i = 0; i < pollSize; i++)
		{
			int sockfd = workEpoolEvent[i].data.fd;

			shared_ptr< EpoolEventInfo> eventinfo = getEventInfo(sockfd);
			if (eventinfo == NULL)	continue;
			void* eventid = NULL;

			if (EVENTISERROR(workEpoolEvent[i].events))
			{
				eventid = eventinfo->readeventid;
				clean(sockfd, PoolType_Read, eventinfo);

				eventpool->postEvent(eventid, false);
			}
			if (EVENTISREAD(workEpoolEvent[i].events))
			{
				eventid = eventinfo->readeventid;
				clean(sockfd, PoolType_Read, eventinfo);

				eventpool->postEvent(eventid, true);
			}
			
			if (EVENTISWRITE(workEpoolEvent[i].events))
			{
				eventid = eventinfo->writeeventid;
				clean(sockfd, PoolType_Write, eventinfo);

				eventpool->postEvent(eventid, true);
			}
		}
	}
private:
	void addEventInfo(int socketfd, const shared_ptr< EpoolEventInfo>& info)
	{
		Guard((Mutex&)mutex);

		eventmap[socketfd] = info;
	}
	void delEventInfo(int socketfd)
	{
		Guard((Mutex&)mutex);

		eventmap.erase(socketfd);
	}
	shared_ptr< EpoolEventInfo> getEventInfo(int socketfd)
	{
		Guard((Mutex&)mutex);

		std::map<int, shared_ptr< EpoolEventInfo> >::iterator iter = eventmap.find(socketfd);
		if (iter == eventmap.end()) return iter->second;

		return iter->second;
	}	
};

#endif