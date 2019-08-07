#pragma  once
#include "Base/Base.h"
#include "Network/Socket.h"
#include "_eventlist.h"
#include "../common/_pool.h"
using namespace Public::Base;
using namespace Public::Network;

//这里是异步处理IO

class _Pool;
class _PoolResource:public enable_shared_from_this<_PoolResource>
{
public:
	_PoolResource(int socketfd, const shared_ptr<Socket>& sock, const shared_ptr<_UserThread>& userthread, const shared_ptr<_Pool>& pool);
	
	virtual ~_PoolResource();

	virtual bool postEvent(void* eventid, const shared_ptr<Event>& event);
	int socket() { return _socketfd; }
private:
	int						_socketfd;
	shared_ptr<_Pool>		_pool;
	weak_ptr<Socket>		_sock;
	weak_ptr<_UserThread>	_userthread;
};
class _Pool:public ThreadPool
{
	friend class _PoolResource;

	class _ThreadPoolCallbackInfo
	{
	public:
		_ThreadPoolCallbackInfo(const IOWorker::EventCallback& _callback, const shared_ptr<IOWorker::EventInfo>& _info)
		{
			info = _info;
			callback = _callback;
		}
		~_ThreadPoolCallbackInfo() {}

		static void threadPoolCallback(void* param)
		{
			_ThreadPoolCallbackInfo* callbackinfo = (_ThreadPoolCallbackInfo*)param;
			if (callbackinfo != NULL)
			{
				callbackinfo->callback(callbackinfo->info);
			}
			SAFE_DELETE(callbackinfo);
		}
	private:
		shared_ptr<IOWorker::EventInfo>	info;
		IOWorker::EventCallback	callback;
	};
public:
	_Pool(uint32_t maxthreadnum) :ThreadPool(maxthreadnum)
	{
		_eventlist = make_shared<_EventList>();
	}
	virtual ~_Pool() { _eventlist = NULL; }

	virtual shared_ptr<_PoolResource> addResource(int socketfd, const shared_ptr<Socket>& _sock, const shared_ptr<_UserThread>& _userthread) = 0;

	virtual bool delResource(int socketfd, _PoolResource* res) = 0;

	virtual bool postEvent(const shared_ptr<_PoolResource>& res, const shared_ptr<Event>& event, void* eventid) = 0;

	virtual bool postExtExentFunction(const IOWorker::EventCallback& _callback, const shared_ptr<IOWorker::EventInfo>& _info) 
	{
		_ThreadPoolCallbackInfo* info = new _ThreadPoolCallbackInfo(_callback, _info);

		if (!ThreadPool::dispatch(_ThreadPoolCallbackInfo::threadPoolCallback, info))
		{
			SAFE_DELETE(info);
		}

		return true;
	}

	shared_ptr<_EventList> eventlist() { return _eventlist; }
protected:
	shared_ptr<_EventList>		_eventlist;
};


inline _PoolResource::_PoolResource(int socketfd, const shared_ptr<Socket>& sock, const shared_ptr<_UserThread>& userthread, const shared_ptr<_Pool>& pool)
	:_socketfd(socketfd), _pool(pool), _sock(sock), _userthread(userthread)
{

}
inline _PoolResource::~_PoolResource()
{
	_pool->delResource(_socketfd, this);
}

inline bool _PoolResource::postEvent(void* eventid, const shared_ptr<Event>& event)
{
	shared_ptr<Socket> sock = _sock.lock();
	shared_ptr< _UserThread> userthread = _userthread.lock();

	_pool->eventlist()->pushEvent(eventid, event);

	if (sock == NULL || userthread == NULL || !event->init(sock, userthread))
	{
		_pool->eventlist()->popEvent(eventid);

		return false;
	}

	if (!_pool->postEvent(shared_from_this(), event, eventid))
	{
		_pool->eventlist()->popEvent(eventid);

		return false;
	}

	return true;
}
