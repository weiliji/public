#pragma  once
#include "_winevent.h"
#include "../common/_Pool.h"
#include "../common/_EventThreadPool.h"

#ifdef WIN32
class _SystemPool :public _Pool
{
public:
	_SystemPool(){}
	~_SystemPool(){}

	virtual bool start(_EventThreadPool*_eventpool)
	{
		iocp = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
		if (iocp == NULL)
		{
			return false;
		}

		return _Pool::start(_eventpool);
	}
	virtual bool stop()
	{
		cancelThread();

		if (iocp != NULL)
			::PostQueuedCompletionStatus(iocp, 0, NULL, NULL);

		destroyThread();

		if (iocp)
			CloseHandle(iocp);
		iocp = NULL;

		return _Pool::stop();
	}

	virtual void create(int sockfd, int& poolid)
	{
		if (iocp == NULL) return;

		HANDLE iosock = ::CreateIoCompletionPort((HANDLE)sockfd, iocp, NULL, 0);

		poolid = (int)iosock;
	}

	virtual void destory(int sockfd,int poolid)
	{
		HANDLE iosock = (HANDLE)poolid;

		if (iosock != NULL) CloseHandle(iosock);
	}

	virtual void runPool()
	{
		if (iocp == NULL) return;

		DWORD bytes = 0, key = 0;
		OVERLAPPED* poverlaped = NULL;

		BOOL ret = ::GetQueuedCompletionStatus(iocp, &bytes, &key, &poverlaped, INFINITE);

		if (poverlaped == NULL) return;

		eventpool->postEvent(poverlaped, ret, bytes);
	}
private:
	HANDLE			iocp;
};
#endif