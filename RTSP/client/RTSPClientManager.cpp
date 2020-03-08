#include "RTSP/RTSPClient.h"
#include "../common/udpPortAlloc.h"

namespace Public {
namespace RTSP {

struct RTSPClientManager::RTSPClientManagerInternal
{
	shared_ptr<IOWorker> worker;
	std::string			useragent;
	shared_ptr<UDPPortAlloc> alloc;

	shared_ptr<Timer>		poolTimer;

	Mutex										mutex;
	std::map<RTSPClient*, weak_ptr< RTSPClient> > clientlist;

	RTSPClientManagerInternal(){}

	void onPoolTimerProc(unsigned long)
	{
		std::map<RTSPClient*, weak_ptr< RTSPClient> > clientlisttmp;
		{
			Guard locker(mutex);
			clientlisttmp = clientlist;
		}

		for (std::map<RTSPClient*, weak_ptr< RTSPClient> >::iterator iter = clientlisttmp.begin(); iter != clientlisttmp.end(); iter++)
		{
			shared_ptr< RTSPClient> client = iter->second.lock();
			if (client == NULL)
			{
				Guard locker(mutex);
				clientlist.erase(iter->first);
			}
			else
			{
				client->onPoolTimerProc();
			}
		}
	}
};

RTSPClientManager::RTSPClientManager(const shared_ptr<IOWorker>& iowrker, const std::string& useragent)
{
	internal = new RTSPClientManagerInternal;
	internal->worker = iowrker;
	internal->useragent = useragent;
	internal->alloc = make_shared<UDPPortAlloc>();

	if (internal->worker == NULL) internal->worker = IOWorker::defaultWorker();

	internal->poolTimer = make_shared<Timer>("RTSPClientManager");
	internal->poolTimer->start(Timer::Proc(&RTSPClientManagerInternal::onPoolTimerProc, internal), 0, 1000);

}
RTSPClientManager::~RTSPClientManager()
{
	if (internal->poolTimer) internal->poolTimer->stopAndWait();
	internal->poolTimer = NULL;

	SAFE_DELETE(internal);
}


shared_ptr<RTSPClient> RTSPClientManager::create(const shared_ptr<RTSPClientHandler>& handler, const RTSPUrl& pRtspUrl)
{
	if (handler == NULL) return shared_ptr<RTSPClient>();

	//const std::shared_ptr<IOWorker>& work, const shared_ptr<RTSPClientHandler>& handler, const std::string& rtspUrl,const std::string& useragent);
	shared_ptr<RTSPClient> client = shared_ptr<RTSPClient>(new RTSPClient(internal->worker,handler, internal->alloc ,pRtspUrl,internal->useragent));
	{
		Guard locker(internal->mutex);
		internal->clientlist[client.get()] = client;
	}
	return client;
}
bool RTSPClientManager::initRTPOverUdpPort(uint32_t startport, uint32_t stopport)
{
	internal->alloc->setUdpPortInfo(startport, stopport);

	return true;
}

}
}
