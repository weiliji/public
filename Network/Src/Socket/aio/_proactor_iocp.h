#pragma once
#include "_poll.h"

#ifdef SUPPORT_IOCP

#include <unordered_map>
#include "_proactor_event.h"

//这里是处理事件的线程池

struct _WinExternEvent : public Event
{
	IOWorker::EventCallback callback;
	void *param;

	_WinExternEvent() : Event(EventType_Read) {}
	virtual void callEvent1(const shared_ptr<Socket>& sock, int bytes, bool status){}
	virtual void callEvent(int bytes, bool status)
	{
		callback(param);
	}
};

struct _WinExternResource : public _PollResource
{
public:
	_WinExternResource() : _PollResource(0, shared_ptr<Socket>(), shared_ptr<_Poll>()) {}
    virtual ~_WinExternResource() {}	
};

struct _WinIOCPResource : public _PollResource
{
public:
	_WinIOCPResource(SOCKET socketfd, const shared_ptr<Socket> &sock, const shared_ptr<_Poll> &pool)
		: _PollResource(socketfd, sock, pool)
	{
	}
	~_WinIOCPResource() {}
};

class _Proactor_IOCP : public _Poll, public enable_shared_from_this<_Proactor_IOCP>
{
public:
	_Proactor_IOCP(uint32_t num, Thread::Priority pri)
	{
		iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
		if (iocp != INVALID_HANDLE_VALUE)
		{
			for (uint32_t i = 0; i < num; i++)
			{
				shared_ptr<Thread> thread = ThreadEx::creatThreadEx("_IOCPPoll", ThreadEx::Proc(&_Proactor_IOCP::threadDoProc, this), NULL, pri);
				thread->createThread();

				threadlist.push_back(thread);
			}
		}

		extresource = make_shared<_WinExternResource>();
	}

	~_Proactor_IOCP()
	{
		for (int i = 0; i < threadlist.size(); i++)
		{
			threadlist[i]->cancelThread();
		}
		for (int i = 0; i < threadlist.size(); i++)
		{
			// 通知所有完成端口退出
			PostQueuedCompletionStatus(iocp, 0, (DWORD)0, NULL);
		}
		threadlist.clear();
		if (iocp != INVALID_HANDLE_VALUE)
		{
			CloseHandle(iocp);

			iocp = INVALID_HANDLE_VALUE;
		}

		while (1)
		{
			shared_ptr<Event> event = extresource->popEvent(EventType_Read,NULL);
			if (event == NULL)
				break;
			event->callEvent(0, true);
		}
	}
	virtual PoolType type() { return PoolType_IOCP; }
	virtual bool postExtExentFunction(const IOWorker::EventCallback &_callback, void *param)
	{
		WinIOCP_OVERLAPPED* overlapped = new WinIOCP_OVERLAPPED();
		
		overlapped->resource = extresource;
		shared_ptr<_WinExternEvent> event = make_shared<_WinExternEvent>();
		overlapped->event = event;
		event->callback = _callback;
		event->param = param;

		extresource->postEvent(event, INFINITE);

		PostQueuedCompletionStatus(iocp, 0, (ULONG_PTR)extresource.get(), &overlapped->overlped);

		return true;
	}
	virtual shared_ptr<_PollResource> createResource(SOCKET socketfd, const shared_ptr<Socket> &_sock)
	{
		{
			HANDLE ret = CreateIoCompletionPort((HANDLE)socketfd, iocp, (ULONG_PTR)socketfd, 0);
			if (ret == INVALID_HANDLE_VALUE)
				return shared_ptr<_PollResource>();
		}

		shared_ptr<_WinIOCPResource> resource = make_shared<_WinIOCPResource>(socketfd, _sock, shared_from_this());

		return resource;
	}
	virtual bool destoryResource(SOCKET socketfd, _PollResource *res) { return true; }
	void threadDoProc(Thread *thread, void *param)
	{
		DWORD dwBytes = 0;
		OVERLAPPED *overlapped = NULL;
		ULONG_PTR key = 0;

		while (thread->looping())
		{
			BOOL bRet = GetQueuedCompletionStatus(iocp, &dwBytes, &key, &overlapped, 1000);

			WinIOCP_OVERLAPPED *winoverlapped = CONTAINING_RECORD(overlapped, WinIOCP_OVERLAPPED, overlped);
			
			if(overlapped == NULL || winoverlapped == NULL)
				continue;

			if (winoverlapped->event)
			{
				shared_ptr<_PollResource> resource = winoverlapped->resource.lock();
				if (resource && resource->callbackThreadUsedStart())
				{
					winoverlapped->event->callEvent(bRet ? (DWORD)dwBytes : 0, bRet ? true : false);
					resource->callbackThreadUsedEnd();
				}
			}
			SAFE_DELETE(winoverlapped);
		}
	}
private:
	HANDLE iocp = INVALID_HANDLE_VALUE;

	std::vector<shared_ptr<Thread>> threadlist;

	shared_ptr<_WinExternResource> extresource;
};

#endif