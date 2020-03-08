#pragma once
#include "Base/Base.h"
#include "Network/Socket/Socket.h"
#include "Network/Socket/TcpClient.h"
using namespace Public::Base;
using namespace Public::Network;

#ifndef WIN32
#define closesocket close
#endif

#ifdef WIN32
#define SUPPORT_IOCP
#endif

#if defined(__linux__)
#define SUPPORT_EPOLL
#endif

#if defined(__APPLE__)
#define SUPPORT_KQUEUE
#endif

#if defined(WIN32) || defined(__linux__) || defined(__APPLE__)
#define SUPPORT_POLL
#endif

typedef enum
{
	PoolType_IOCP,
	PoolType_EPOLL,
	PoolType_KQUEUE,
	PoolType_POLL
} PoolType;

struct Public::Network::NewSocketInfo
{
	InetType inet;
	SOCKET newsocket;
	NetAddr otheraddr;
};

typedef enum
{
	EventType_None = 0,
	EventType_Read = 1,
	EventType_Write = 2,
} EventType;

struct OtherDeleteSocket
{
	shared_ptr<Socket> sock;

	static void DelteSocketCallback(void *param)
	{
		OtherDeleteSocket *delobj = (OtherDeleteSocket *)param;
		if (delobj)
			SAFE_DELETE(delobj);
	}
};

class Event
{
public:
	Event(EventType _pooltype) : pooltype(_pooltype) {}
	virtual ~Event() {}

	//处理事件，IOCP内部处理，其他reactor模型需要手动处理
	//return true表示处理结束，return false 表示还需要继续处理
	virtual bool doEvent()
	{
		shared_ptr<Socket> sockptr = sock.lock();
		if (sockptr == NULL)
		{
			return false;
		}

		bool ret = doEvent1(sockptr);

		return ret;
	}
	EventType type() const { return pooltype; }

	virtual bool postEvent(const shared_ptr<Socket> &_sock)
	{
		sock = _sock;

		return true;
	}
	//执行调用event的结果，bytes处理的梳理，status 处理的结果 
	virtual void callEvent(int bytes, bool status)
	{
		shared_ptr<Socket> sockptr = sock.lock();
		if (sockptr == NULL)
			return;

		callEvent1(sockptr,bytes,status);
	}
private:
	virtual void callEvent1(const shared_ptr<Socket> &sock, int bytes, bool status) = 0;
	virtual bool doEvent1(const shared_ptr<Socket> &sock) {return false;}

private:
	weak_ptr<Socket> sock;
	EventType pooltype;
};

//这里是异步处理IO

class _PollResource;
class _Poll
{
	friend class _PollResource;
	struct TimeoutEvent
	{
		weak_ptr<_PollResource> res;
		weak_ptr <Event> event;
		EventType type;
		uint64_t timeout = 0;

		bool operator<(const TimeoutEvent &event) const
		{
			return timeout < event.timeout;
		}
	};
public:
	_Poll() 
	{
		checkTimer = make_shared<Timer>("_Poll");
		checkTimer->start(Timer::Proc(&_Poll::checkEventTimeout, this), 0, 1000);
	}
	virtual ~_Poll() { checkTimer->stop(); }

	virtual shared_ptr<_PollResource> createResource(SOCKET socketfd, const shared_ptr<Socket> &_sock) = 0;
	virtual bool destoryResource(SOCKET socketfd, _PollResource *res) = 0;

	virtual bool postExtExentFunction(const IOWorker::EventCallback &_callback, void *param) = 0;
	virtual PoolType type() = 0;


	virtual void addTimeoutEvent(const shared_ptr<_PollResource>& res,const shared_ptr<Event> &event, uint32_t timeoutms)
	{
		TimeoutEvent tevent;
		tevent.res = res;
		tevent.type = event->type();
		tevent.event = event;
		tevent.timeout = Time::getCurrentMilliSecond() + timeoutms;

		GuardWrite locker(rwmutex);
        checktimeoutevent.push_back(tevent);
		checktimeoutevent.sort();
	}
private:
	void checkEventTimeout(unsigned long)
	{
		GuardWrite locker(rwmutex);

		uint64_t nowtime = Time::getCurrentMilliSecond();
		while(checktimeoutevent.size() > 0)
		{
			TimeoutEvent tevent = checktimeoutevent.front();

            shared_ptr<Event> event = tevent.event.lock();
            if (event != NULL)
            {
				if (nowtime < tevent.timeout) break;


                timeoutevnetlist.push_back(tevent);

                postExtExentFunction(IOWorker::EventCallback(&_Poll::doCalltimeoutEvent, this),NULL);
            }

            checktimeoutevent.pop_front();
		}
	}
    void doCalltimeoutEvent(void *param);
protected:
	ReadWriteMutex			rwmutex;
	std::list<TimeoutEvent> checktimeoutevent;
    std::list<TimeoutEvent> timeoutevnetlist;
	shared_ptr<Timer>		checkTimer;
};

class _PollResource : public enable_shared_from_this<_PollResource>
{
public:
	_PollResource(SOCKET socketfd, const shared_ptr<Socket> &sock, const shared_ptr<_Poll> &pool)
		: _socketfd(socketfd), _poll(pool), _sock(sock)
	{
	}

	virtual ~_PollResource()
	{
		quit();
	}
	virtual void quit()
	{
		{
			Guard locker(_mutex);
			mustQuit = true;
		}
        shared_ptr<_Poll> tmp = _poll;
		if (tmp)
            tmp->destoryResource(_socketfd, this);

		_poll = NULL;

		waitAllOtherCallbackThreadUsedEnd();
	}

	virtual bool postEvent(const shared_ptr<Event> &event, uint32_t timeout)
	{
		shared_ptr<_Poll> pooltmp = _poll;
		shared_ptr<Socket> sock = _sock.lock();
		if (!sock || !pooltmp)
		{
			return false;
		}

		bool ret = event->postEvent(sock);
		if (timeout != (uint32_t)INFINITE)
		{
			pooltmp->addTimeoutEvent(shared_from_this(),event, timeout);
		}

		return ret;
	}

	//iocp event
    virtual shared_ptr<Event> popEvent(EventType type, void *event) {return shared_ptr<Event>();   }

	SOCKET socket() { return _socketfd; }

	///记录当前回调线程使用信息
	bool callbackThreadUsedStart()
	{
		Guard locker(_mutex);
		if (mustQuit)
			return false;

		uint32_t currThreadId = Thread::getCurrentThreadID();
		usedCallbackThreadId += currThreadId;
		++usedCallbackThreadNum;

		return true;
	}
	///当前回调线程使用结束
	bool callbackThreadUsedEnd()
	{
		Guard locker(_mutex);

		uint32_t currThreadId = Thread::getCurrentThreadID();
		usedCallbackThreadId -= currThreadId;
		--usedCallbackThreadNum;

		return true;
	}

private:
	////等待所有回调线程使用结束
	void waitAllOtherCallbackThreadUsedEnd()
	{
		uint32_t currThreadId = Thread::getCurrentThreadID();
		while (1)
		{
			{
				Guard locker(_mutex);
				if (usedCallbackThreadNum == 0/* || (usedCallbackThreadNum == 1 && usedCallbackThreadId == currThreadId)*/)
				{
					///如果没有线程使用，或者是自己线程关闭自己，可以关闭
					break;
				}
			}
			Thread::sleep(10);
		}
	}

protected:
	SOCKET _socketfd;
	shared_ptr<_Poll> _poll;
	weak_ptr<Socket> _sock;

	Mutex _mutex;

	bool mustQuit = false;				///是不是已经被关闭
	uint64_t usedCallbackThreadId = 0;  //正在使用回调的线程ID和
	uint32_t usedCallbackThreadNum = 0; //正在使用回调的线程数
};

inline void _Poll::doCalltimeoutEvent(void *param)
{
    TimeoutEvent timeoutevent;
    {
        GuardWrite locker(rwmutex);

        if (timeoutevnetlist.size() <= 0)
        {
            return;
        }
        timeoutevent = timeoutevnetlist.front();
        timeoutevnetlist.pop_front();
    }
   

	{
        shared_ptr<_PollResource> res = timeoutevent.res.lock();
        shared_ptr<Event> event = timeoutevent.event.lock();
        if (res && event)
        {
           res->popEvent(timeoutevent.type, event.get());
           event->callEvent(0, false);
        }
	}
}
