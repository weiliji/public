#pragma  once
#include "Base/Base.h"
#include "Network/Socket.h"
#include "_event.h"
#include "../common/_pool.h"
using namespace Public::Base;
using namespace Public::Network;

//这里是异步处理IO

class _Pool;
class _PoolResource:public enable_shared_from_this<_PoolResource>
{
public:
	_PoolResource(SOCKET socketfd, const shared_ptr<Socket>& sock, const shared_ptr<_UserThread>& userthread, const shared_ptr<_Pool>& pool);
	
	virtual ~_PoolResource();

	virtual bool postEvent(Event* event);
	virtual bool postEvent(const shared_ptr<Event>& event);

	SOCKET socket() { return _socketfd; }
private:
	SOCKET					_socketfd;
	shared_ptr<_Pool>		_pool;
	weak_ptr<Socket>		_sock;
	weak_ptr<_UserThread>	_userthread;
};
class _Pool:public ThreadPool
{
	friend class _PoolResource;

public:
	_Pool(uint32_t maxthreadnum) :ThreadPool(maxthreadnum)
	{
	}
	virtual ~_Pool() { }

	virtual shared_ptr<_PoolResource> addResource(SOCKET socketfd, const shared_ptr<Socket>& _sock, const shared_ptr<_UserThread>& _userthread) = 0;

	virtual bool delResource(SOCKET socketfd, _PoolResource* res) { return true; }

	virtual bool addEvent(const shared_ptr<_PoolResource>& res, Event* event) { return true; }
	virtual bool addEvent(const shared_ptr<_PoolResource>& res, const shared_ptr<Event>& event) { return true; }

	virtual bool postExtExentFunction(const IOWorker::EventCallback& _callback, void* param) 
	{
		return ThreadPool::dispatch(_callback, param);
	}
};


inline _PoolResource::_PoolResource(SOCKET socketfd, const shared_ptr<Socket>& sock, const shared_ptr<_UserThread>& userthread, const shared_ptr<_Pool>& pool)
	:_socketfd(socketfd), _pool(pool), _sock(sock), _userthread(userthread)
{

}
inline _PoolResource::~_PoolResource()
{
	_pool->delResource(_socketfd, this);
}

inline bool _PoolResource::postEvent(Event* event)
{
	shared_ptr<Socket> sock = _sock.lock();
	shared_ptr< _UserThread> userthread = _userthread.lock();

	if (sock == NULL || userthread == NULL || !event->postEvent(sock, userthread))
	{
		return false;
	}

	if (!_pool->addEvent(shared_from_this(), event))
	{
		return false;
	}

	return true;
}

inline bool _PoolResource::postEvent(const shared_ptr<Event>& event)
{
	shared_ptr<Socket> sock = _sock.lock();
	shared_ptr< _UserThread> userthread = _userthread.lock();

	if (sock == NULL || userthread == NULL || !event->postEvent(sock, userthread))
	{
		return false;
	}

	if (!_pool->addEvent(shared_from_this(), event))
	{
		return false;
	}

	return true;
}