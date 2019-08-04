#pragma  once
#include "_Pool.h"
#include "_EventThreadPool.h"

#ifdef WIN32
class IOCP :public Pool
{
public:
	IOCP(const shared_ptr<EventThreadPool>&_eventpool):Pool(_eventpool)
	{
		iocp = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
		if (iocp == NULL)
		{
			return;
		}
	}
	~IOCP()
	{
		cancelThread();
		
		if(iocp != NULL)
			::PostQueuedCompletionStatus(iocp, 0, NULL, NULL);
		
		destroyThread(); 

		if(iocp)
			CloseHandle(iocp);
		iocp = NULL;
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

		Event* eventptr = CONTAINING_RECORD(poverlaped, Event, overlped);

		eventpool->postEvent(eventptr, ret, bytes);
	}
private:
	HANDLE			iocp;
};
#endif