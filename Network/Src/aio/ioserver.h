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

		uint32_t poolthreadnum = threadnum - 1;

		_EventPool::start();
		_EventThreadPool::start(this, poolthreadnum);
		_SystemPool::start(this);
	}
	~IOServer()
	{
		_SystemPool::stop();
		_EventThreadPool::stop();
		_EventPool::stop();
	}
};