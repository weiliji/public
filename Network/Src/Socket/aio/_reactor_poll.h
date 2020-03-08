#pragma once
#include "_poll.h"

#ifdef SUPPORT_POLL

#include "_reactor_resource.h"

#ifdef WIN32
#define poll WSAPoll
#else
#include <poll.h>
#endif

class _Reactor_Polled : public _Reactor_Poll, public Thread, public enable_shared_from_this<_Reactor_Polled>
{
#define MAXPOOLSIZE 1024
	std::map<SOCKET, shared_ptr<_ReactorResource>> resourcelist;

public:
	_Reactor_Polled(uint32_t num, Thread::Priority pri) :_Reactor_Poll(num,pri),Thread("_Reactor_Poll", priorTop, policyRealtime)
	{
		createThread();
	}
	~_Reactor_Polled()
	{
		destroyThread();
	}

	virtual PoolType type() { return PoolType_POLL; }

	virtual shared_ptr<_PollResource> createResource(SOCKET socketfd, const shared_ptr<Socket> &_sock)
	{
		shared_ptr<_ReactorResource> eventinfo = make_shared<_ReactorResource>(socketfd, _sock, shared_from_this());
		{
			GuardWrite locker(rwmutex);
			resourcelist[socketfd] = eventinfo;
		}

		return eventinfo;
	}

	virtual bool destoryResource(SOCKET socketfd, _PollResource *res)
	{
		{
			GuardWrite locker(rwmutex);
			resourcelist.erase(socketfd);
		}
		return true;
	}
	void gentPollFd(struct pollfd pollfds[MAXPOOLSIZE], weak_ptr<_ReactorResource> resourcemap[MAXPOOLSIZE], int &realsize)
	{
		realsize = 0;

		SOCKET poolfd = awakenFd();
		if (poolfd != 0)
		{
			resourcemap[realsize] = shared_ptr<_ReactorResource>();

			pollfds[realsize].fd = poolfd;
			pollfds[realsize].revents = 0;
			pollfds[realsize].events |= POLLIN | POLLRDNORM;	

			realsize++;
		}


		GuardRead locker(rwmutex);

		for (std::map<SOCKET, shared_ptr<_ReactorResource>>::iterator iter = resourcelist.begin(); iter != resourcelist.end() && realsize < MAXPOOLSIZE; iter++)
		{
			size_t readsize = iter->second->readeventlist.size();
			size_t writesize = iter->second->writeeventlist.size();

			if (readsize <= 0 && writesize <= 0)
				continue;

			resourcemap[realsize] = iter->second;

			pollfds[realsize].fd = iter->first;
			pollfds[realsize].events = 0;
			pollfds[realsize].revents = 0;

			if (readsize > 0)
			{
				pollfds[realsize].events |= POLLIN | POLLRDNORM;
			}
			if (writesize > 0)
			{
				pollfds[realsize].events |= POLLOUT /*| POLLERR| POLLHUP | POLLNVAL*/;
			}

			realsize++;
		}
	}
	void doPollSet(struct pollfd pollfds[MAXPOOLSIZE], weak_ptr<_ReactorResource> resourcemap[MAXPOOLSIZE], int realsize)
	{
		SOCKET poolfd = awakenFd();

		for (int i = 0; i < realsize; i++)
		{
			if (pollfds[i].revents == 0)
				continue;

			if (poolfd != 0 && poolfd == pollfds[i].fd)
			{
				doAwaken();
				continue;
			}

			shared_ptr<_ReactorResource> resource = resourcemap[i].lock();
			if (resource == NULL)
				continue;

			assert(resource->socket() == (SOCKET)pollfds[i].fd);

			if (pollfds[i].revents & POLLIN || pollfds[i].revents & POLLRDNORM)
			{
				resource->_Reactor_DoEvent(EventType_Read);
			}

			if (pollfds[i].revents & POLLOUT || pollfds[i].revents & POLLERR || pollfds[i].revents & POLLHUP || pollfds[i].revents & POLLNVAL)
			{
				resource->_Reactor_DoEvent(EventType_Write);
			}
		}
	}
	void threadProc()
	{
		struct pollfd pollfds[MAXPOOLSIZE];
		weak_ptr<_ReactorResource> resource[MAXPOOLSIZE];

		memset(pollfds, 0, sizeof(pollfds));

		int poolsize = 0;

		while (looping())
		{
			gentPollFd(pollfds, resource, poolsize);

			int ret = poll(pollfds, poolsize, 10);
			if (ret > 0)
			{
				doPollSet(pollfds, resource, poolsize);
			}
		}
	}
};

#endif
