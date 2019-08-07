#pragma  once
#include "../common/_pool.h"

#ifdef WIN32
//这里是处理事件的线程池

struct _WinIOCP:public _PoolResource
{
public:
	_WinIOCP(int socketfd, const shared_ptr<Socket>& sock, const shared_ptr<_UserThread>& userthread, const shared_ptr<_Pool>& pool, PTP_WIN32_IO_CALLBACK callback,void* pv, TP_CALLBACK_ENVIRON* tp_env)
		:_PoolResource(socketfd,sock,userthread,pool)
	{
		tpio = CreateThreadpoolIo(reinterpret_cast<HANDLE>(socketfd), callback, pv, tp_env);
		if (tpio == NULL)
		{
			assert(0);
		}

		
	}
	virtual bool postEvent(void* eventid, const shared_ptr<Event>& event)
	{
		StartThreadpoolIo(tpio);

		if (!_PoolResource::postEvent(eventid, event))
		{
			CancelThreadpoolIo(tpio);

			return false;
		}

		return true;
	}
	~_WinIOCP()
	{
		WaitForThreadpoolIoCallbacks(tpio, true);
		CloseThreadpoolIo(tpio);

		tpio = NULL;
	}
private:
	TP_IO*				 tpio;
};

class _SystemPoll :public _Pool,public enable_shared_from_this<_SystemPoll>
{
public:
	_SystemPoll(uint32_t maxthreadnum):_Pool(maxthreadnum){}

	~_SystemPoll(){}

	virtual shared_ptr<_PoolResource> addResource(int socketfd, const shared_ptr<Socket>& _sock, const shared_ptr<_UserThread>& _userthread)
	{
		//_WinIOCP(int socketfd, const shared_ptr<Socket>& sock, const shared_ptr<_UserThread>& userthread, const shared_ptr<_Pool>& pool, PTP_WIN32_IO_CALLBACK callback,void* pv, TP_CALLBACK_ENVIRON* tp_env)
		shared_ptr< _WinIOCP> iocp = shared_ptr<_WinIOCP>(new _WinIOCP(socketfd, _sock,_userthread,shared_from_this(), _SystemPoll::IoCompletionCallback,this, (TP_CALLBACK_ENVIRON*)threadpoolHandle()));
		return iocp;
	}

	virtual bool delResource(int socketfd, _PoolResource* res) { return true; }

	virtual bool postEvent(const shared_ptr<_PoolResource>& res, const shared_ptr<Event>& event, void* eventid)
	{
		return true;
	}
	/*
	typedef VOID (WINAPI *PTP_WIN32_IO_CALLBACK)(
	__inout     PTP_CALLBACK_INSTANCE Instance,
	__inout_opt PVOID                 Context,
	__inout_opt PVOID                 Overlapped,
	__in        ULONG                 IoResult,
	__in        ULONG_PTR             NumberOfBytesTransferred,
	__inout     PTP_IO                Io
	);
	*/
	static void CALLBACK IoCompletionCallback(PTP_CALLBACK_INSTANCE /* Instance */, PVOID  Context,
		PVOID Overlapped, ULONG IoResult, ULONG_PTR NumberOfBytesTransferred, PTP_IO  Io)
	{
		_SystemPoll* pool = (_SystemPoll*)Context;
		if (pool == NULL) return;
		

		shared_ptr<Event> event = pool->_eventlist->popEvent(Overlapped);

		if (event)
		{
			event->doEvent((DWORD)NumberOfBytesTransferred, IoResult == ERROR_SUCCESS);
		}
	}
};

#endif