#pragma  once
#include "Base/Base.h"
#include "Network/Socket.h"
#include "_eventthreadpool.h"
using namespace Public::Base;
using namespace Public::Network;

//这里是异步处理IO
typedef enum {
	PoolType_Read,
	PoolType_Write,
}PoolType;
class _Pool :public Thread
{
public:
	_Pool() :Thread("IOPool",Thread::priorTop, Thread::policyRealtime){}
	virtual ~_Pool(){}
	virtual bool start(_EventThreadPool*_eventpool)
	{
		eventpool = _eventpool;

		createThread();

		return true;
	}
	virtual bool stop()
	{
		destroyThread();
		return true;
	}
	virtual void create(int sockfd,int& poolid) = 0;
	virtual void destory(int sockfd,int poolid) = 0;
	virtual void clean(int sockfd, PoolType type){}
	virtual void add(int sockfd,PoolType type,void* eventid){}
private:
	void threadProc()
	{
		while (looping())
		{
			runPool();
		}
	}
private:
	virtual void runPool() = 0;
protected:
	_EventThreadPool*	eventpool;
};