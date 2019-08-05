#pragma  once
#include "win32/_winevent.h"
#include "win32/_winiocp.h"

#include "common/_eventpool.h"
#include "common/_eventthreadpool.h"
#include "linux/_epool.h"


class IOServer:public _EventThreadPool,public _EventPool,public _SystemPool
{
public:
	IOServer(uint32_t threadnum)
	{
		if (threadnum <= 0) threadnum = 1;

		uint32_t poolThreadnum = 1;
		uint32_t threadPoolnum = threadnum - poolThreadnum;

#ifdef WIN32
		poolThreadnum = threadnum / 4;
		if (poolThreadnum == 0) poolThreadnum = 1;
		threadPoolnum = threadnum - poolThreadnum;
#endif

		_EventPool::start();
		_EventThreadPool::start(this, threadPoolnum);
		_SystemPool::start(this, poolThreadnum);
	}
	~IOServer()
	{
		_SystemPool::stop();
		_EventThreadPool::stop();
		_EventPool::stop();
	}
};