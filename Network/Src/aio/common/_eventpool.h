#pragma  once
#include "Base/Base.h"
#include "Network/Network.h"
#include "_userthread.h"

using namespace Public::Base;
using namespace Public::Network;

struct NewSocketInfo
{
	int		newsocket;
	NetAddr	otheraddr;
};


class Event
{
public:
	Event(const shared_ptr<Socket>& _sock, const shared_ptr<_UserThread>& _userthread)
	{
		if (_sock == NULL || _userthread == NULL)
		{
			assert(0);
		}
		weak_sock = _sock;
		userthread = _userthread;
	}
	virtual ~Event() {}

	//处理事件，int 为事件当前的值，status为状态
	void doEvent(int bytes, bool status)
	{
		shared_ptr<Socket> sock = weak_sock.lock();
		if (sock == NULL || !userthread || !userthread->callbackThreadUsedStart())
		{
			return;
		}

		doEvent(sock, bytes, status);

		userthread->callbackThreadUsedEnd();
	}

	//判断事件是否有效
	virtual bool isValid() { return weak_sock.lock() != NULL; }
private:
	virtual void doEvent(const shared_ptr<Socket>& sock, int bytes, bool status) = 0;
protected:
	weak_ptr<Socket> weak_sock;
	shared_ptr<_UserThread> userthread;
};

//事件池，存放当前正在使用的事件，定时判断事件是否过期
class _EventPool
{
public:
	_EventPool(){}
	~_EventPool(){}

	bool start()
	{
		checkTimer = make_shared<Timer>("EventPool");
		checkTimer->start(Timer::Proc(&_EventPool::doCheckTimerProc, this), 0, 10000);

		return true;
	}

	bool stop()
	{
		checkTimer = NULL;
		eventmap.clear();

		return true;
	}

	//放入事件
	void pushEvent(void* eventid, const shared_ptr<Event>& event)
	{
		Guard locker(mutex);

		eventmap[eventid] = event;
	}
	//取出事件
	shared_ptr<Event> popEvent(void* eventid)
	{
		Guard locker(mutex);

		std::map<void*, shared_ptr<Event> >::iterator iter = eventmap.find(eventid);
		if (iter == eventmap.end()) return shared_ptr<Event>();

		return iter->second;
	}
private:
	void doCheckTimerProc(unsigned long)
	{
		std::map<void*, shared_ptr<Event> > eventmaptmp;
		{
			Guard locker(mutex);
			eventmaptmp = eventmap;
		}

		//无效的事件列表
		std::set<void*>	invalidList;

		//检测事件是否有效
		for (std::map<void*, shared_ptr<Event> >::iterator iter = eventmaptmp.begin(); iter != eventmaptmp.end(); iter++)
		{
			if (!iter->second->isValid())
			{
				invalidList.insert(iter->first);
			}
		}

		//删除无效的事件
		{
			Guard locker(mutex);
			for (std::set<void*> ::iterator iter = invalidList.begin(); iter != invalidList.end(); iter++)
			{
				eventmap.erase(*iter);
			}
		}
	}
private:
	Mutex								mutex;
	std::map<void*, shared_ptr<Event> > eventmap;

	shared_ptr<Timer>					checkTimer;
};