#include "OnvifClient/OnvifClient.h"
#include "HTTP/HTTPClient.h"
#include "protocol/OnvifProtocol.h"
#include "OnvifDiscovery.h"
using namespace Public::HTTP;

namespace Public {
namespace Onvif {


struct OnvifClientManager::OnvifClientManagerInternal
{
	std::string useragent;
	shared_ptr<IOWorker> worker;

	shared_ptr<Timer>	disconverytimer;
	shared_ptr<OnvifDisconvery> disconvery;
	uint64_t	disconverystarttime;
	uint64_t	timeout;


	void onPoolDisconveryTimerProc(unsigned long)
	{
		uint64_t nowtime = Time::getCurrentMilliSecond();

		if (nowtime > disconverystarttime && nowtime - disconverystarttime >= timeout)
		{
			disconvery->stop();
			disconvery = NULL;
			disconverytimer->stop();

			return;
		}

		disconvery->sendSearch();
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
}
OnvifClientManager::~OnvifClientManager()
{
	SAFE_DELETE(internal);
}

shared_ptr<OnvifClient> OnvifClientManager::create(const URL& url)
{
	return shared_ptr<OnvifClient>(new OnvifClient(internal->worker, url, internal->useragent));
}

bool OnvifClientManager::disconvery(const DisconveryCallback& callback, uint32_t timeout)
{
	if (internal->disconvery != NULL) return false;

	internal->disconvery = make_shared<OnvifDisconvery>();
	if (!internal->disconvery->start(internal->worker, callback))
	{
		return false;
	}

	internal->timeout = timeout;
	internal->disconverystarttime = Time::getCurrentMilliSecond();
	internal->disconverytimer = make_shared<Timer>("DisconveryTimer");
	internal->disconvery->sendSearch();
	internal->disconverytimer->start(Timer::Proc(&OnvifClientManagerInternal::onPoolDisconveryTimerProc, internal), 0, 3 * 1000);

	return true;
}

}
}

