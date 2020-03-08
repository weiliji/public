#include "RTSP/RTSPServer.h"
#include "../common/udpPortAlloc.h"
using namespace Public::RTSP;

struct RTSPServer::RTSPServerInternal
{
	std::string							useragent;

	shared_ptr<IOWorker>				ioworker;
	shared_ptr<Socket>					tcpserver;

	uint32_t							port;

	Mutex								mutex;
	std::map<Socket*, shared_ptr<RTSPServerSession> >	serverlist;

	ListenCallback						listencallback;

	shared_ptr<Timer>					pooltimer;

	shared_ptr<UDPPortAlloc>			portalloc;

	RTSPServerInternal() {}

	void onpooltimerproc(unsigned long)
	{
		std::map<Socket*, shared_ptr<RTSPServerSession> > serverlistmp;
		{
			Guard locker(mutex);
			serverlistmp = serverlist;
		}

		for (std::map<Socket*, shared_ptr<RTSPServerSession> >::iterator iter = serverlistmp.begin(); iter != serverlistmp.end();iter++)
		{
			shared_ptr<RTSPServerSession> session = iter->second;
			
			session->onPoolTimerProc(); 
			if (session->sessionIsTimeout())
			{
				session->closedByTimeout();
				session->disconnect();

				{
					Guard locker(mutex);
					serverlist.erase(iter->first);
				}

				continue;
			}

			if (session->connectStatus() != NetStatus_connected)
			{
				session->disconnect();

				{
					Guard locker(mutex);
					serverlist.erase(iter->first);
				}
			}
		}
	}
	void inputSocket(const shared_ptr<Socket>& newsock, const char* buffer, uint32_t bufferlen)
	{
		{
			shared_ptr<RTSPServerSession> session = shared_ptr<RTSPServerSession>(new  RTSPServerSession(ioworker, listencallback, portalloc, useragent));

			{
				Guard locker(mutex);
				serverlist[newsock.get()] = session;
			}

			session->initRTSPServerSessionPtr(session,newsock,buffer,bufferlen);
		}
	}
	void onsocketaccept(const weak_ptr<Socket>& sock, const shared_ptr<Socket>& newsock)
	{
		if(newsock)
		{
			inputSocket(newsock, NULL, 0);
		}
		

		shared_ptr<Socket> socktmp = sock.lock();
		if (socktmp != NULL)
		{
			socktmp->async_accept(Socket::AcceptedCallback(&RTSPServerInternal::onsocketaccept, this));
		}
	}
};


RTSPServer::RTSPServer(const std::string& useragent)
{
	internal = new RTSPServerInternal();

	internal->useragent = useragent;
	internal->portalloc = make_shared<UDPPortAlloc>();
}
RTSPServer::~RTSPServer()
{
	stop();

	SAFE_DELETE(internal);
}
bool RTSPServer::initRTPOverUdpType(uint32_t startport, uint32_t stopport)
{
	internal->portalloc->setUdpPortInfo(startport, stopport);

	return true;
}
bool RTSPServer::run(const shared_ptr<IOWorker>& worker, uint32_t port, const ListenCallback& callback)
{
	internal->listencallback = callback;
	internal->port = port;
	internal->ioworker = worker;

	if (internal->ioworker == NULL) internal->ioworker = IOWorker::defaultWorker();

	if (internal->ioworker != NULL || internal->port != 0)
	{
		internal->tcpserver = TCPServer::create(internal->ioworker, port);
		internal->tcpserver->async_accept(Socket::AcceptedCallback(&RTSPServerInternal::onsocketaccept, internal));
	}

	internal->pooltimer = make_shared<Timer>("RTSPServer");
	internal->pooltimer->start(Timer::Proc(&RTSPServerInternal::onpooltimerproc, internal), 0, 1000);

	return true;
}
bool RTSPServer::stop()
{
	internal->pooltimer = NULL;
	if (internal->tcpserver) internal->tcpserver->disconnect();
	internal->tcpserver = NULL;

	return true;
}
uint32_t RTSPServer::listenPort() const
{
	return internal->port;
}

void RTSPServer::inputSocket(const shared_ptr<Socket>& sock, const char* buffer, uint32_t bufferlen)
{
	internal->inputSocket(sock, buffer, bufferlen);
}