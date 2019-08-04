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
class Pool :public Thread
{
public:
	Pool(const shared_ptr<EventThreadPool>&_eventpool) :Thread("IOPool",Thread::priorTop, Thread::policyRealtime), eventpool(_eventpool)
	{
		cancelThread();
	}
	virtual ~Pool()
	{
		destroyThread();
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
	shared_ptr<EventThreadPool>	eventpool;
};