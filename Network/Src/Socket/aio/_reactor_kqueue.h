#pragma once
#include "_poll.h"

#ifdef SUPPORT_KQUEUE
#include "_reactor_resource.h"

#include <sys/socket.h>
#include <sys/event.h>
#include <netinet/in.h>
#include <arpa/inet.h>

struct _KqueueResource : public _ReactorResource
{
	_KqueueResource(int socketfd, const shared_ptr<Socket> &sock, const shared_ptr<_Poll> &pool, int _kqueuefd)
		: _ReactorResource(socketfd, sock, pool), kqueuefd(_kqueuefd)
	{
	}
	virtual ~_KqueueResource()
	{
		struct kevent ev[2];
		EV_SET(&ev[0], socket(), EVFILT_READ, EV_DELETE, 0, 0, (void *)(intptr_t)socket());
		EV_SET(&ev[1], socket(), EVFILT_WRITE, EV_DELETE, 0, 0, (void *)(intptr_t)socket());

		kevent(kqueuefd, ev, 2, NULL, 0, NULL);
	}
	virtual void _addEvent(EventType type)
	{
		struct kevent ev;
		if (type == EventType_Read)
		{
			EV_SET(&ev, socket(), EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, (void *)(intptr_t)socket());
		}
		else if (type == EventType_Write)
		{
			EV_SET(&ev, socket(), EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, (void *)(intptr_t)socket());
		}
		kevent(kqueuefd, &ev, 1, NULL, 0, NULL);
	}
	virtual void _cleanEvent(EventType type)
	{
		struct kevent ev;
		if (type == EventType_Read)
		{
			EV_SET(&ev, socket(), EVFILT_READ, EV_DELETE, 0, 0, (void *)(intptr_t)socket());
		}
		else if (type == EventType_Write)
		{
			EV_SET(&ev, socket(), EVFILT_WRITE, EV_DELETE, 0, 0, (void *)(intptr_t)socket());
		}

		kevent(kqueuefd, &ev, 1, NULL, 0, NULL);
	}

private:
	int kqueuefd;
};

class _Reactor_Kqueue : public _Reactor_Poll, public Thread, public enable_shared_from_this<_Reactor_Kqueue>
{
	int kqueuefd;
	std::map<int, shared_ptr<_KqueueResource>> resourcelist;

public:
	_Reactor_Kqueue(uint32_t num, Thread::Priority pri) : _Reactor_Poll(num, pri), Thread("_Reactor_Kqueue", priorTop, policyRealtime), kqueuefd(0)
	{
		kqueuefd = ::kqueue();
		if (kqueuefd <= 0)
			return;

		SOCKET poolfd = awakenFd();
		if (poolfd != 0)
		{
			struct kevent ev;
			EV_SET(&ev, poolfd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, (void *)(intptr_t)poolfd);
			kevent(kqueuefd, &ev, 1, NULL, 0, NULL);
		}

		createThread();
	}
	~_Reactor_Kqueue()
	{
		destroyThread();

		SOCKET poolfd = awakenFd();
		if (poolfd != 0)
		{
			struct kevent ev;
			EV_SET(&ev, poolfd, EVFILT_READ, EV_DELETE, 0, 0, (void *)(intptr_t)poolfd);
			kevent(kqueuefd, &ev, 1, NULL, 0, NULL);
		}

		if (kqueuefd > 0)
			close(kqueuefd);
		kqueuefd = 0;
	}
	virtual PoolType type() { return PoolType_KQUEUE; }
	virtual shared_ptr<_PollResource> createResource(SOCKET socketfd, const shared_ptr<Socket> &_sock)
	{
		shared_ptr<_KqueueResource> eventinfo = make_shared<_KqueueResource>(socketfd, _sock, shared_from_this(), kqueuefd);
		{
			GuardWrite locker(rwmutex);
			resourcelist[socketfd] = eventinfo;
		}

		return eventinfo;
	}

	virtual bool destoryResource(SOCKET socketfd, _PollResource *res)
	{
		GuardWrite locker(rwmutex);
		resourcelist.erase(socketfd);

		return true;
	}
	void threadProc()
	{
		if (kqueuefd <= 0)
			return;

		struct timespec timeout;
		timeout.tv_sec = 0;
		timeout.tv_nsec = 100 * 1000 * 1000;

#define MAXEVENTNUM 256
		struct kevent activeEvs[MAXEVENTNUM];

		SOCKET poolfd = awakenFd();

		while (looping())
		{
			int n = kevent(kqueuefd, NULL, 0, activeEvs, MAXEVENTNUM, &timeout);

			for (int i = 0; i < n; i++)
			{
				int sockfd = (int)(intptr_t)activeEvs[i].udata;
				int events = activeEvs[i].filter;

				if (poolfd != 0 && poolfd == sockfd)
				{
					doAwaken();
					continue;
				}

				shared_ptr<_KqueueResource> resource = getEventResource(sockfd);
				if (resource == NULL)
					continue;

				if (events == EVFILT_READ)
				{
					resource->_Reactor_DoEvent(EventType_Read);
				}
				if (events == EVFILT_WRITE)
				{
					resource->_Reactor_DoEvent(EventType_Write);
				}
			}
		}
	}

private:
	shared_ptr<_KqueueResource> getEventResource(int socketfd)
	{
		GuardRead locker(rwmutex);

		std::map<int, shared_ptr<_KqueueResource>>::iterator iter = resourcelist.find(socketfd);
		if (iter == resourcelist.end())
			return shared_ptr<_KqueueResource>();

		return iter->second;
	}
};

#endif