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
class _Pool
{
public:
	_Pool(){}
	virtual ~_Pool(){}
	virtual bool start(_EventThreadPool*_eventpool,uint32_t threadnum)
	{
		eventpool = _eventpool;
		for (uint32_t i = 0; i < threadnum; i++)
		{
			shared_ptr<Thread> thread = ThreadEx::creatThreadEx("_Pool", ThreadEx::Proc(&_Pool::threadProc,this), NULL, Thread::priorTop, Thread::policyRealtime);
			thread->createThread();

			threadlist.push_back(thread);
		}

		return true;
	}
	virtual bool stop()
	{
		threadlist.clear();

		return true;
	}
	virtual void create(int sockfd) = 0;
	virtual void destory(int sockfd) = 0;
	virtual void clean(int sockfd, PoolType type){}
	virtual void add(int sockfd,PoolType type,void* eventid){}
private:
	void threadProc(Thread* t,void*)
	{
		while (t->looping())
		{
			runPool();
		}
	}
private:
	virtual void runPool() = 0;
protected:
	_EventThreadPool*	eventpool;
private:
	std::list<shared_ptr<Thread> > threadlist;
};