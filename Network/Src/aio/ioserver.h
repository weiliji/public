#pragma  once
#include "win32/_winiocp.h"
#include "linux/_linuxepoll.h"


class IOServer:public _SystemPoll
{
public:
	IOServer(uint32_t threadnum):_SystemPoll(threadnum)
	{
		if (threadnum <= 0) threadnum = 1;
	}
	~IOServer()
	{

	}
};