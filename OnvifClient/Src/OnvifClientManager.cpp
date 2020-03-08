#include "OnvifClient/OnvifClient.h"
#include "protocol/OnvifProtocol.h"
#include "OnvifDiscovery.h"

namespace Public {
namespace Onvif {

struct OnvifClientManager::Disconvery::DisconveryInternal
{
	std::map<std::string, shared_ptr<OnvifDisconvery> > disconverylist;
};
OnvifClientManager::Disconvery::Disconvery()
{
	internal = new DisconveryInternal;
}
OnvifClientManager::Disconvery::~Disconvery()
{
	SAFE_DELETE(internal);
}

void OnvifClientManager::Disconvery::sendDisconvery()
{
	for (std::map<std::string, shared_ptr<OnvifDisconvery> >::iterator iter = internal->disconverylist.begin(); iter != internal->disconverylist.end(); iter++)
	{
		iter->second->sendSearch();
	}
}

void OnvifClientManager::Disconvery::getDeviceList(std::list<OnvifClientDefs::DiscoveryInfo>& list)
{
	list.clear();

	for (std::map<std::string, shared_ptr<OnvifDisconvery> >::iterator iter = internal->disconverylist.begin(); iter != internal->disconverylist.end(); iter++)
	{
		iter->second->getDeviceList(list);
	}
}

struct OnvifClientManager::OnvifClientManagerInternal
{
	std::string								useragent;
	shared_ptr<IOWorker>					worker;
	shared_ptr<HTTP::AsyncClient>			asyncClient;

	Mutex												mutex;
	std::map<OnvifClient*,weak_ptr<OnvifClient>>		clientlist;
	shared_ptr <Timer>									timer;

	void onPoolTimerProc(unsigned long)
	{
		std::map<OnvifClient*, weak_ptr<OnvifClient>> listtmp;
		{
			Guard locker(mutex);
			listtmp = clientlist;
		}

		for (std::map<OnvifClient*, weak_ptr<OnvifClient>>::iterator iter = listtmp.begin(); iter != listtmp.end(); iter++)
		{
			shared_ptr<OnvifClient> client = iter->second.lock();
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


OnvifClientManager::OnvifClientManager(const shared_ptr<IOWorker>& worker, const std::string& userContent)
{
	internal = new OnvifClientManagerInternal;
	internal->useragent = userContent;
	internal->worker = worker;
	if (internal->worker == NULL)
	{
		internal->worker = make_shared<IOWorker>(2);
	}
	internal->asyncClient = make_shared<HTTP::AsyncClient>();
	internal->timer = make_shared<Timer>("OnvifClientManager");
	internal->timer->start(Timer::Proc(&OnvifClientManagerInternal::onPoolTimerProc, internal), 0, 1000);
}
OnvifClientManager::~OnvifClientManager()
{
	internal->timer = NULL;
	SAFE_DELETE(internal);
}

shared_ptr<OnvifClient> OnvifClientManager::create(const URL& url)
{
	shared_ptr<OnvifClient> client(new OnvifClient(internal->worker, internal->asyncClient, url,internal->useragent));
	{
		Guard locker(internal->mutex);
		internal->clientlist[client.get()] = client;
	}

	return client;
}
shared_ptr<OnvifClientManager::Disconvery> OnvifClientManager::disconvery()
{
	std::map<std::string, Host::NetworkInfo> infos;
	std::string defaultMac;

	Host::getNetworkInfos(infos, defaultMac);

	shared_ptr<OnvifClientManager::Disconvery> disconverytmp = make_shared<OnvifClientManager::Disconvery>();

	for (std::map<std::string, Host::NetworkInfo>::iterator iter = infos.begin(); iter != infos.end(); iter++)
	{
		if (iter->second.ip.length() <= 0 || iter->second.netmask.length() <= 0 || iter->second.gateway.length() <= 0)
		{
			continue;
		}

		shared_ptr<OnvifDisconvery> disconvery = make_shared<OnvifDisconvery>();
		if (!disconvery->start(internal->worker, iter->second))
		{
			continue;
		}

		disconvery->sendSearch();

		disconverytmp->internal->disconverylist[iter->second.ip] = disconvery;
	}	

	return disconverytmp;
}

}
}

