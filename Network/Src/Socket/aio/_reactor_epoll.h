#pragma  once
#include "_poll.h"

#ifdef SUPPORT_EPOLL
#include "_reactor_resource.h"

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

struct _EPollResource :public _ReactorResource
{
	int						     event = EVENT_INIT;

	_EPollResource(int socketfd, const shared_ptr<Socket> &sock, const shared_ptr<_Poll> &pool, int _epoolfd)
		: _ReactorResource(socketfd, sock, pool),epoolfd(_epoolfd)
	{
		{
			epoll_event pevent;
			memset(&pevent, 0, sizeof(pevent));
			pevent.data.fd = socket();
			pevent.events = event;

			int ret = ::epoll_ctl(epoolfd, EPOLL_CTL_ADD, socket(), &pevent);
			assert(ret == 0);
			(void)ret;
		}
	}
	virtual ~_EPollResource()
	{
		{
			epoll_event pevent;
			memset(&pevent, 0, sizeof(pevent));

			int ret = ::epoll_ctl(epoolfd, EPOLL_CTL_DEL, socket(), &pevent);
			assert(ret == 0);
			(void)ret;
		}
	}

	virtual void _addEvent(EventType type) 
	{
		if (type == EventType_Read)
		{
			event |= EVENT_READ;
		}
		else if(type == EventType_Write)
		{
			event |= EVENT_WRITE;
		}

		{
			epoll_event pevent;
			memset(&pevent, 0, sizeof(pevent));
			pevent.data.fd = socket();
			pevent.events = event;

			int ret = ::epoll_ctl(epoolfd, EPOLL_CTL_MOD, socket(), &pevent);
			assert(ret == 0);
			(void)ret;
		}
	}
	virtual void _cleanEvent(EventType type) 
	{
		if (type == EventType_Read)
		{
			event &= ~EVENT_READ;
		}
		else if(type == EventType_Write)
		{
			event &= ~EVENT_WRITE;
		}

		{
			epoll_event pevent;
			memset(&pevent, 0, sizeof(pevent));
			pevent.data.fd = socket();
			pevent.events = event;

			int ret = ::epoll_ctl(epoolfd, EPOLL_CTL_MOD, socket(), &pevent);
			assert(ret == 0);
			(void)ret;
		}
	}

private:
	int		epoolfd;
};

class _Reactor_Epoll :public _Reactor_Poll,public Thread,public enable_shared_from_this<_Reactor_Epoll>
{
	int 										epoll;
	
	std::map<int, shared_ptr<_EPollResource>> 	resourcelist;

public:
	_Reactor_Epoll(uint32_t num, Thread::Priority pri) :_Reactor_Poll(num,pri),Thread("_Reactor_Epoll", priorTop, policyRealtime), epoll(0)
	{
		epoll = ::epoll_create(10000);
		if (epoll <= 0) return;

		SOCKET poolfd = awakenFd();
		if (poolfd != 0)
		{
			epoll_event pevent;
			memset(&pevent, 0, sizeof(pevent));
			pevent.data.fd = poolfd;
			pevent.events = EVENT_READ;

			int ret = ::epoll_ctl(epoll, EPOLL_CTL_ADD, poolfd, &pevent);
			assert(ret == 0);
			(void)ret;
		}

		createThread();
	}
	~_Reactor_Epoll()
	{
		destroyThread();

		SOCKET poolfd = awakenFd();
		if (poolfd != 0)
		{
			epoll_event pevent;
			memset(&pevent, 0, sizeof(pevent));

			int ret = ::epoll_ctl(epoll, EPOLL_CTL_DEL, poolfd, &pevent);
			assert(ret == 0);
			(void)ret;
		}

		if (epoll > 0) close(epoll);
		epoll = 0;
	}
	virtual PoolType type() { return PoolType_EPOLL; }

	virtual shared_ptr<_PollResource> createResource(SOCKET socketfd, const shared_ptr<Socket>& _sock)
	{
		shared_ptr< _EPollResource> eventinfo = make_shared<_EPollResource>(socketfd, _sock, shared_from_this(),epoll);
		{
			GuardWrite locker(rwmutex);
			resourcelist[socketfd] = eventinfo;
		}
		return eventinfo;
	}

	virtual bool destoryResource(int socketfd, _PollResource* res)
	{
		{
			GuardWrite locker(rwmutex);
			resourcelist.erase(socketfd);
		}

		return true; 
	}
	
	void threadProc()
	{
		if (epoll <= 0) return;

#define MAXEVENTNUM			256	
		epoll_event workEpoolEvent[MAXEVENTNUM];
		
		SOCKET poolfd = awakenFd();

		while (looping())
		{
			int pollSize = ::epoll_wait(epoll, workEpoolEvent, MAXEVENTNUM, 100);		
						
			if (pollSize <= 0)
				continue;

			for (int i = 0; i < pollSize; i++)
			{
				SOCKET sockfd = workEpoolEvent[i].data.fd;

				if (!(EVENTISERROR(workEpoolEvent[i].events) || EVENTISREAD(workEpoolEvent[i].events) || EVENTISWRITE(workEpoolEvent[i].events)))
					continue;
				
				if (poolfd != 0 && poolfd == sockfd)
				{
					doAwaken();
					continue;
				}

				shared_ptr< _EPollResource> resource = getEventResource(sockfd);
				if (resource == NULL)	continue;

				if (EVENTISERROR(workEpoolEvent[i].events) || EVENTISREAD(workEpoolEvent[i].events))
				{
					resource->_Reactor_DoEvent(EventType_Read);
				}
				if (EVENTISWRITE(workEpoolEvent[i].events))
				{
					resource->_Reactor_DoEvent(EventType_Write);
				}
			}
		}

	}
private:
	shared_ptr<_EPollResource> getEventResource(int socketfd)
	{
		GuardRead locker(rwmutex);

		std::map<int, shared_ptr< _EPollResource> >::iterator iter = resourcelist.find(socketfd);
		if (iter == resourcelist.end()) return shared_ptr<_EPollResource>();

		return iter->second;
	}
};

#endif