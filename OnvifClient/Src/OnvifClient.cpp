#include "OnvifClient/OnvifClient.h"
#include "HTTP/HTTPClient.h"
#include "protocol/OnvifProtocol.h"

using namespace Public::HTTP;

namespace Public {
namespace Onvif {


#define ONVIFECONENTTYPE		"application/soap+xml"

struct AyncGetItem;
struct OnvifClient::OnvifClientInternal
{
	URL url;
	shared_ptr<IOWorker> worker;
	shared_ptr< HTTPAsyncClient> asyncclient;
	std::string useragent;

	Mutex mutex;
	std::map<AyncGetItem*,shared_ptr< AyncGetItem> > asynclist;

	std::set< AyncGetItem*> freeList;

	shared_ptr<Timer> poolTimer;


	void addAsyncItem(const shared_ptr<AyncGetItem>& item)
	{
		Guard locker(mutex);

		asynclist[item.get()] = item;
	}

	void delAsyncItem(AyncGetItem* item)
	{
		Guard locker(mutex);

		freeList.insert(item);
	}

	void onPoolTimerProc(unsigned long)
	{
		Guard locker(mutex);

		for (std::set< AyncGetItem*>::iterator iter = freeList.begin(); iter != freeList.end(); iter++)
		{
			asynclist.erase(*iter);
		}

		freeList.clear();
	}
};


struct AyncGetItem :public enable_shared_from_this<AyncGetItem>
{
public:
	AyncGetItem() :result(false) {}
	virtual ~AyncGetItem() {}

	bool wait(uint32_t timeout)
	{
		return cmdsem.pend(timeout) >= 0;
	}
	void start(const shared_ptr<CmdObject>& cmd, OnvifClient::OnvifClientInternal* _internal, uint32_t timeout, const URL& _requrl = URL())
	{
		internal = _internal;
		cmdptr = cmd;

		internal->addAsyncItem(shared_from_this());

		shared_ptr<HTTPClientRequest> req = make_shared<HTTPClientRequest>();

		URL requrl = _requrl;
		if (requrl.getHostname() == "") requrl = internal->url;
		if (requrl.getPath() == "" || requrl.getPath() == "/")
		{
			requrl.setPath(cmd->requesturl);
		}

		requrl.setAuthen(internal->url.getAuhen());

		{
			req->headers()["Content-Type"] = std::string(ONVIFECONENTTYPE) + "; charset=utf-8;";// action = \"" + cmd->action + "\"";
			req->headers()["Accept-Encoding"] = "gzip, deflate";
			if (internal->useragent != "")
				req->headers()["User-Agent"] = internal->useragent;
			req->headers()["Connection"] = "close";
		}

		req->content()->write(cmd->build(requrl));
		req->method() = "POST";
		req->timeout() = timeout;
		req->url() = requrl.getHost() + requrl.getPath() + requrl.getSearch();

		internal->asyncclient->request(req, HTTPAsyncClient::HTTPCallback(&AyncGetItem::httpCallback, shared_from_this()));
	}

	virtual void doResult() = 0;
private:
	void httpCallback(const shared_ptr<HTTPClientRequest>&res, const shared_ptr<HTTPClientResponse>&response)
	{
		do
		{
			if (response == NULL || response->statusCode() != 200)
			{
				break;
			}

			Value contenttypeval = response->header("Content-Type");
			if (strstr(String::tolower(contenttypeval.readString()).c_str(), ONVIFECONENTTYPE) == 0)
			{
				break;
			}

			std::string httpbody = response->content()->read();

			XMLObject xml;
			if (!xml.parseBuffer(httpbody)) break;

			if (xml.getRoot().name() != "Envelope") break;

			const XMLObject::Child& body = xml.getRoot().getChild("Body");
			if (body.isEmpty()) break;

			shared_ptr<CmdObject> cmdtmp = cmdptr.lock();
			if (cmdtmp == NULL) break;

			result = cmdtmp->parse(body);
		} while (0);

		cmdsem.post();
		doResult();

		internal->delAsyncItem(this);
	}
private:
	shared_ptr<HTTPClientRequest> client;
	weak_ptr<CmdObject>			  cmdptr;
	Base::Semaphore				  cmdsem;

	OnvifClient::OnvifClientInternal* internal;
public:
	bool						  result;
};


OnvifClient::OnvifClient(const shared_ptr<IOWorker>& worker, const URL& url,const std::string& useragent)
{
	internal = new OnvifClientInternal;
	internal->url = url;
	internal->useragent = useragent;
	internal->worker = worker;
	internal->asyncclient = make_shared<HTTPAsyncClient>(worker,useragent);

	internal->poolTimer = make_shared<Timer>("OnvifClient");
	internal->poolTimer->start(Timer::Proc(&OnvifClientInternal::onPoolTimerProc, internal), 0, 5000);
}
OnvifClient::~OnvifClient()
{
	internal->poolTimer = NULL;
	internal->asyncclient = NULL;
	{
		Guard locker(internal->mutex);
		internal->asynclist.clear();
		internal->freeList.clear();
	}
	SAFE_DELETE(internal);
}

struct AsyncGetInfoItem:public AyncGetItem
{
	shared_ptr<CMDGetDeviceInformation> cmd;

	OnvifClient::GetInfoCalblack		callback;

	void doResult()
	{
		callback(result ? cmd->devinfo: NULL);
	}
};

shared_ptr<OnvifClientDefs::Info> OnvifClient::getInfo(int timeoutms)
{
	shared_ptr< AsyncGetInfoItem> cmditem = make_shared<AsyncGetInfoItem>();
	cmditem->cmd = make_shared<CMDGetDeviceInformation>();

	cmditem->start(cmditem->cmd, internal, timeoutms);

	if(!cmditem->wait(timeoutms)) return shared_ptr<OnvifClientDefs::Info>();
	
	return cmditem->cmd->devinfo;
}



bool OnvifClient::asyncGetInfo(const GetInfoCalblack& callback, int timeoutms)
{
	shared_ptr< AsyncGetInfoItem> cmditem = make_shared<AsyncGetInfoItem>();
	cmditem->cmd = make_shared<CMDGetDeviceInformation>();

	cmditem->start(cmditem->cmd, internal, timeoutms);

	return true;
}


struct AsyncGetCapabilitiesItem :public AyncGetItem
{
	shared_ptr<CMDGetCapabilities> cmd;

	OnvifClient::GetCapabilitiesCallback		callback;

	void doResult()
	{
		callback(result ? cmd->capabilities : NULL);
	}
};


shared_ptr<OnvifClientDefs::Capabilities> OnvifClient::getCapabities(int timeoutms)
{
	shared_ptr< AsyncGetCapabilitiesItem> cmditem = make_shared<AsyncGetCapabilitiesItem>();
	cmditem->cmd = make_shared<CMDGetCapabilities>();

	cmditem->start(cmditem->cmd, internal, timeoutms);

	if (!cmditem->wait(timeoutms)) return shared_ptr<OnvifClientDefs::Capabilities>();

	return cmditem->cmd->capabilities;
}
bool OnvifClient::asyncGetCapabities(const GetCapabilitiesCallback& callbac, int timeoutms)
{
	shared_ptr< AsyncGetCapabilitiesItem> cmditem = make_shared<AsyncGetCapabilitiesItem>();
	cmditem->cmd = make_shared<CMDGetCapabilities>();

	cmditem->start(cmditem->cmd, internal, timeoutms);

	return true;
}
//shared_ptr<OnvifClientDefs::Scopes> OnvifClient::getScopes(int timeoutms)
//{
//	shared_ptr<CMDGetScopes> cmd = make_shared<CMDGetScopes>();
//
//	internal->sendOvifRequest(cmd.get(), timeoutms);
//
//	return cmd->scopes;
//}


struct AsyncGetProfilesItem :public AyncGetItem
{
	shared_ptr<CMDGetProfiles> cmd;

	OnvifClient::GetProfilesCallback		callback;

	void doResult()
	{
		callback(result ? cmd->profileInfo : NULL);
	}
};
shared_ptr<OnvifClientDefs::Profiles> OnvifClient::getProfiles(int timeoutms)
{
	shared_ptr< AsyncGetProfilesItem> cmditem = make_shared<AsyncGetProfilesItem>();
	cmditem->cmd = make_shared<CMDGetProfiles>();

	cmditem->start(cmditem->cmd, internal, timeoutms);

	if (!cmditem->wait(timeoutms)) return shared_ptr<OnvifClientDefs::Profiles>();

	return cmditem->cmd->profileInfo;
}
bool OnvifClient::asyncGetProfiles(const GetProfilesCallback& callback, int timeoutms)
{
	shared_ptr< AsyncGetProfilesItem> cmditem = make_shared<AsyncGetProfilesItem>();
	cmditem->cmd = make_shared<CMDGetProfiles>();

	cmditem->start(cmditem->cmd, internal, timeoutms);

	return true;
}

struct AsyncGetStreamUrlItem :public AyncGetItem
{
	shared_ptr<CmdGetStreamURL> cmd;

	OnvifClient::GetStreamUrlCallback		callback;

	void doResult()
	{
		callback(result ? cmd->streamurl : NULL);
	}
};
shared_ptr<OnvifClientDefs::StreamUrl> OnvifClient::getStreamUrl(const OnvifClientDefs::ProfileInfo& info, int timeoutms)
{
	shared_ptr< AsyncGetStreamUrlItem> cmditem = make_shared<AsyncGetStreamUrlItem>();
	cmditem->cmd = make_shared<CmdGetStreamURL>(info.token);

	cmditem->start(cmditem->cmd, internal, timeoutms);

	if (!cmditem->wait(timeoutms)) return shared_ptr<OnvifClientDefs::StreamUrl>();

	return cmditem->cmd->streamurl;
}
bool OnvifClient::asyncGetStreamUrl(const GetStreamUrlCallback& callbac, const OnvifClientDefs::ProfileInfo& info, int timeoutms)
{
	shared_ptr< AsyncGetStreamUrlItem> cmditem = make_shared<AsyncGetStreamUrlItem>();
	cmditem->cmd = make_shared<CmdGetStreamURL>(info.token);

	cmditem->start(cmditem->cmd, internal, timeoutms);

	return true;
}

struct AsyncGetSnapUrlItem :public AyncGetItem
{
	shared_ptr<CmdGetSnapURL> cmd;

	OnvifClient::GetSnapUrlCallback		callback;

	void doResult()
	{
		callback(result ? cmd->snapurl : NULL);
	}
};

shared_ptr<OnvifClientDefs::SnapUrl> OnvifClient::getSnapUrl(const OnvifClientDefs::ProfileInfo& info, int timeoutms)
{
	shared_ptr< AsyncGetSnapUrlItem> cmditem = make_shared<AsyncGetSnapUrlItem>();
	cmditem->cmd = make_shared<CmdGetSnapURL>(info.token);

	cmditem->start(cmditem->cmd, internal, timeoutms);

	if (!cmditem->wait(timeoutms)) return shared_ptr<OnvifClientDefs::SnapUrl>();

	return cmditem->cmd->snapurl;
}
bool OnvifClient::asyncGetSnapUrl(const GetSnapUrlCallback& callbac, const OnvifClientDefs::ProfileInfo& info, int timeoutms)
{
	shared_ptr< AsyncGetSnapUrlItem> cmditem = make_shared<AsyncGetSnapUrlItem>();
	cmditem->cmd = make_shared<CmdGetSnapURL>(info.token);

	cmditem->start(cmditem->cmd, internal, timeoutms);

	return true;
}

struct AsyncGetNetworkInterfacesItem :public AyncGetItem
{
	shared_ptr<CmdGetNetworkInterfaces> cmd;

	OnvifClient::GetNetworkInterfacesCallback		callback;

	void doResult()
	{
		callback(result ? cmd->network : NULL);
	}
};

shared_ptr<OnvifClientDefs::NetworkInterfaces> OnvifClient::getNetworkInterfaces(int timeoutms)
{
	shared_ptr< AsyncGetNetworkInterfacesItem> cmditem = make_shared<AsyncGetNetworkInterfacesItem>();
	cmditem->cmd = make_shared<CmdGetNetworkInterfaces>();

	cmditem->start(cmditem->cmd, internal, timeoutms);

	if (!cmditem->wait(timeoutms)) return shared_ptr<OnvifClientDefs::NetworkInterfaces>();

	return cmditem->cmd->network;
}

bool OnvifClient::asyncGetNetworkInterfaces(const GetNetworkInterfacesCallback& callbac, int timeoutms)
{
	shared_ptr< AsyncGetNetworkInterfacesItem> cmditem = make_shared<AsyncGetNetworkInterfacesItem>();
	cmditem->cmd = make_shared<CmdGetNetworkInterfaces>();

	cmditem->start(cmditem->cmd, internal, timeoutms);

	return true;
}

//shared_ptr<OnvifClientDefs::VideoEncoderConfigurations> OnvifClient::getVideoEncoderConfigurations(int timeoutms)
//{
//	shared_ptr<CmdGetVideoEncoderConfigurations> cmd = make_shared<CmdGetVideoEncoderConfigurations>();
//
//	internal->sendOvifRequest(cmd.get(), timeoutms);
//
//	return cmd->encoder;
//}

struct AsyncContinuousMoveItem :public AyncGetItem
{
	shared_ptr<CmdContinuousMove> cmd;

	OnvifClient::ContinuousMoveCallback		callback;

	void doResult()
	{
		callback(result);
	}
};
bool OnvifClient::continuousMove(const OnvifClientDefs::ProfileInfo& info, const OnvifClientDefs::PTZCtrl& ptzctrl, int timeoutms)
{
	shared_ptr< AsyncContinuousMoveItem> cmditem = make_shared<AsyncContinuousMoveItem>();
	cmditem->cmd = make_shared<CmdContinuousMove>(ptzctrl, info.token);

	cmditem->start(cmditem->cmd, internal, timeoutms);

	if (!cmditem->wait(timeoutms)) return false;

	return cmditem->result;
}
bool OnvifClient::asyncContinuousMoveCallback(const ContinuousMoveCallback& callback, const OnvifClientDefs::ProfileInfo& info, const OnvifClientDefs::PTZCtrl& ptzctrl, int timeoutms)
{
	shared_ptr< AsyncContinuousMoveItem> cmditem = make_shared<AsyncContinuousMoveItem>();
	cmditem->cmd = make_shared<CmdContinuousMove>(ptzctrl, info.token);

	cmditem->start(cmditem->cmd, internal, timeoutms);

	return true;
}
//bool OnvifClient::absoluteMove(const OnvifClientDefs::ProfileInfo& info, const OnvifClientDefs::PTZCtrl& ptzctrl, int timeoutms)
//{
//	shared_ptr<CmdAbsoluteMove> cmd;// = make_shared<CmdAbsoluteMove>();
//
//	if (!internal->sendOvifRequest(cmd.get(), timeoutms)) return false;
//
//	return true;
//}

struct AsyncStopPtzItem :public AyncGetItem
{
	shared_ptr<CmdStopPTZ> cmd;

	OnvifClient::StopPTRCallback		callback;

	void doResult()
	{
		callback(result);
	}
};
bool OnvifClient::stopPTZ(const OnvifClientDefs::ProfileInfo& info, const OnvifClientDefs::PTZCtrl& ptzctrl, int timeoutms)
{
	shared_ptr< AsyncStopPtzItem> cmditem = make_shared<AsyncStopPtzItem>();
	cmditem->cmd = make_shared<CmdStopPTZ>(ptzctrl, info.token);

	cmditem->start(cmditem->cmd, internal, timeoutms);

	if (!cmditem->wait(timeoutms)) return false;

	return cmditem->result;
}
bool OnvifClient::asyncStopPtz(const StopPTRCallback& callbac, const OnvifClientDefs::ProfileInfo& info, const OnvifClientDefs::PTZCtrl& ptzctrl, int timeoutms)
{
	shared_ptr< AsyncStopPtzItem> cmditem = make_shared<AsyncStopPtzItem>();
	cmditem->cmd = make_shared<CmdStopPTZ>(ptzctrl, info.token);

	cmditem->start(cmditem->cmd, internal, timeoutms);

	return true;
}

struct AsyncSetPresestItem :public AyncGetItem
{
	shared_ptr<CmdSetPreset> cmd;

	OnvifClient::SetPresetCallback		callback;

	void doResult()
	{
		callback(result);
	}
};

bool OnvifClient::setPreset(const OnvifClientDefs::ProfileInfo& info, const std::string& presetname, int timeoutms)
{
	shared_ptr< AsyncSetPresestItem> cmditem = make_shared<AsyncSetPresestItem>();
	cmditem->cmd = make_shared<CmdSetPreset>(presetname, info.token);

	cmditem->start(cmditem->cmd, internal, timeoutms);

	if (!cmditem->wait(timeoutms)) return false;

	return cmditem->result;
}
bool OnvifClient::asyncSetPresest(const SetPresetCallback& callbac, const OnvifClientDefs::ProfileInfo& info, const std::string& presetname, int timeoutms)
{
	shared_ptr< AsyncSetPresestItem> cmditem = make_shared<AsyncSetPresestItem>();
	cmditem->cmd = make_shared<CmdSetPreset>(presetname, info.token);

	cmditem->start(cmditem->cmd, internal, timeoutms);

	return true;
}

struct AsyncGetPresestItem :public AyncGetItem
{
	shared_ptr<CmdGetPresets> cmd;

	OnvifClient::GetPresetCallback		callback;

	void doResult()
	{
		callback(result ? cmd->preset : NULL);
	}
};

shared_ptr<OnvifClientDefs::PresetInfos> OnvifClient::getPreset(const OnvifClientDefs::ProfileInfo& info, int timeoutms)
{
	shared_ptr< AsyncGetPresestItem> cmditem = make_shared<AsyncGetPresestItem>();
	cmditem->cmd = make_shared<CmdGetPresets>(info.token);

	cmditem->start(cmditem->cmd, internal, timeoutms);

	if (!cmditem->wait(timeoutms)) return shared_ptr<OnvifClientDefs::PresetInfos>();

	return cmditem->cmd->preset;
}
bool OnvifClient::asyncGetPreset(const GetPresetCallback& callback, const OnvifClientDefs::ProfileInfo& info, int timeoutms)
{
	shared_ptr< AsyncGetPresestItem> cmditem = make_shared<AsyncGetPresestItem>();
	cmditem->cmd = make_shared<CmdGetPresets>(info.token);

	cmditem->start(cmditem->cmd, internal, timeoutms);

	return true;
}

struct AsyncGotoPresetItem :public AyncGetItem
{
	shared_ptr<CmdGotoPreset> cmd;

	OnvifClient::GotoPresetCallback		callback;

	void doResult()
	{
		callback(result);
	}
};
bool  OnvifClient::gotoPreset(const OnvifClientDefs::ProfileInfo& info, const OnvifClientDefs::PresetInfo& presetinfo, int timeoutms)
{
	shared_ptr< AsyncGotoPresetItem> cmditem = make_shared<AsyncGotoPresetItem>();
	cmditem->cmd = make_shared<CmdGotoPreset>(presetinfo, info.token);

	cmditem->start(cmditem->cmd, internal, timeoutms);

	if (!cmditem->wait(timeoutms)) return false;

	return true;
}
bool OnvifClient::asyncGotoPreset(const GotoPresetCallback& callback, const OnvifClientDefs::ProfileInfo& info, const OnvifClientDefs::PresetInfo& presetinfo, int timeoutms)
{
	shared_ptr< AsyncGotoPresetItem> cmditem = make_shared<AsyncGotoPresetItem>();
	cmditem->cmd = make_shared<CmdGotoPreset>(presetinfo, info.token);

	cmditem->start(cmditem->cmd, internal, timeoutms);

	return true;
}

struct AsyncRemovePresetItem :public AyncGetItem
{
	shared_ptr<CmdRemovePreset> cmd;

	OnvifClient::RemovePresetCallback		callback;

	void doResult()
	{
		callback(result);
	}
};

bool  OnvifClient::removePreset(const OnvifClientDefs::ProfileInfo& info, const OnvifClientDefs::PresetInfo& presetinfo, int timeoutms)
{
	shared_ptr< AsyncRemovePresetItem> cmditem = make_shared<AsyncRemovePresetItem>();
	cmditem->cmd = make_shared<CmdRemovePreset>(presetinfo, info.token);

	cmditem->start(cmditem->cmd, internal, timeoutms);

	if (!cmditem->wait(timeoutms)) return false;

	return true;
}
bool OnvifClient::asyncRemovePreset(const RemovePresetCallback& callback, const OnvifClientDefs::ProfileInfo& info, const OnvifClientDefs::PresetInfo& presetinfo, int timeoutms)
{
	shared_ptr< AsyncRemovePresetItem> cmditem = make_shared<AsyncRemovePresetItem>();
	cmditem->cmd = make_shared<CmdRemovePreset>(presetinfo, info.token);

	cmditem->start(cmditem->cmd, internal, timeoutms);

	return true;
}
//shared_ptr<OnvifClientDefs::PTZConfig> OnvifClient::getConfigurations(int timeoutms)
//{
//	shared_ptr<CmdGetConfigurations> cmd = make_shared<CmdGetConfigurations>();
//
//	if (!internal->sendOvifRequest(cmd.get(), timeoutms)) return shared_ptr<OnvifClientDefs::PTZConfig>();
//
//	return cmd->ptzcfg;
//}
//shared_ptr<OnvifClientDefs::ConfigurationOptions> OnvifClient::getConfigurationOptions(const shared_ptr<OnvifClientDefs::PTZConfig>& ptzcfg, int timeoutms)
//{
//	if (ptzcfg == NULL) return make_shared<OnvifClientDefs::ConfigurationOptions>();
//
//	shared_ptr<CmdGetConfigurationOptions> cmd = make_shared<CmdGetConfigurationOptions>(ptzcfg->token);
//
//	if (!internal->sendOvifRequest(cmd.get(), timeoutms)) return shared_ptr<OnvifClientDefs::ConfigurationOptions>();
//
//	return cmd->options;
//}

struct AsyncGetSystemDatetimeItem :public AyncGetItem
{
	shared_ptr<CmdGetSystemDateAndTime> cmd;

	OnvifClient::GetSystemDateTimeCallback		callback;

	void doResult()
	{
		callback(result ? cmd->time : NULL);
	}
};
shared_ptr<Time> OnvifClient::getSystemDatetime(int timeoutms)
{
	shared_ptr< AsyncGetSystemDatetimeItem> cmditem = make_shared<AsyncGetSystemDatetimeItem>();
	cmditem->cmd = make_shared<CmdGetSystemDateAndTime>();

	cmditem->start(cmditem->cmd, internal, timeoutms);

	if (!cmditem->wait(timeoutms)) return shared_ptr<Time>();

	return cmditem->cmd->time;
}
bool OnvifClient::asyncGetSystemDatetime(const GetSystemDateTimeCallback& callbck, int timeoutms)
{
	shared_ptr< AsyncGetSystemDatetimeItem> cmditem = make_shared<AsyncGetSystemDatetimeItem>();
	cmditem->cmd = make_shared<CmdGetSystemDateAndTime>();

	cmditem->start(cmditem->cmd, internal, timeoutms);

	return true;
}

struct AsyncSetSystemDatetimeItem :public AyncGetItem
{
	shared_ptr<CmdSetSystemDateAndTime> cmd;

	OnvifClient::SetSystemDatetimeCallback		callback;

	void doResult()
	{
		callback(result);
	}
};

bool OnvifClient::SetSystemDatetime(const Time& time, int timeoutms)
{
	shared_ptr< AsyncSetSystemDatetimeItem> cmditem = make_shared<AsyncSetSystemDatetimeItem>();
	cmditem->cmd = make_shared<CmdSetSystemDateAndTime>(time);

	cmditem->start(cmditem->cmd, internal, timeoutms);

	if (!cmditem->wait(timeoutms)) return false;

	return true;
}
bool OnvifClient::asyncSetSystemDatetime(const SetSystemDatetimeCallback& callback, const Time& time, int timeoutms)
{
	shared_ptr< AsyncSetSystemDatetimeItem> cmditem = make_shared<AsyncSetSystemDatetimeItem>();
	cmditem->cmd = make_shared<CmdSetSystemDateAndTime>(time);

	cmditem->start(cmditem->cmd, internal, timeoutms);

	return true;
}

struct AsyncSystemRebootItem :public AyncGetItem
{
	shared_ptr<CMDSystemReboot> cmd;

	OnvifClient::SystemRebootCallback		callback;

	void doResult()
	{
		callback(result);
	}
};

bool OnvifClient::systemReboot(int timeoutms)
{
	shared_ptr< AsyncSystemRebootItem> cmditem = make_shared<AsyncSystemRebootItem>();
	cmditem->cmd = make_shared<CMDSystemReboot>();

	cmditem->start(cmditem->cmd, internal, timeoutms);

	if (!cmditem->wait(timeoutms)) return false;

	return true;
}
bool OnvifClient::asyncSystemReboot(const SystemRebootCallback& callback, int timeoutms)
{
	shared_ptr< AsyncSystemRebootItem> cmditem = make_shared<AsyncSystemRebootItem>();
	cmditem->cmd = make_shared<CMDSystemReboot>();

	cmditem->start(cmditem->cmd, internal, timeoutms);

	return true;
}

struct AsyncStartRecvAlarmItem :public AyncGetItem
{
	shared_ptr<CMDStartRecvAlarm> cmd;

	OnvifClient::StartRecvAlarmCallback		callback;

	void doResult()
	{
		callback(result ? cmd->startrecvalarm : NULL);
	}
};

shared_ptr<OnvifClientDefs::StartRecvAlarm>  OnvifClient::startRecvAlarm(const shared_ptr<OnvifClientDefs::Capabilities>& capabilities,int timeoutms)
{
	if (capabilities == NULL || !capabilities->Events.Support)
	{
		return shared_ptr<OnvifClientDefs::StartRecvAlarm>();
	}

	shared_ptr< AsyncStartRecvAlarmItem> cmditem = make_shared<AsyncStartRecvAlarmItem>();
	cmditem->cmd = make_shared<CMDStartRecvAlarm>();

	cmditem->start(cmditem->cmd, internal, timeoutms);

	if (!cmditem->wait(timeoutms)) return shared_ptr<OnvifClientDefs::StartRecvAlarm>();

	return cmditem->cmd->startrecvalarm;
}
bool OnvifClient::asyncStartRecvAlarm(const StartRecvAlarmCallback& callback, const shared_ptr<OnvifClientDefs::Capabilities>& capabilities, int timeoutms)
{
	if (capabilities == NULL || !capabilities->Events.Support)
	{
		return false;
	}

	shared_ptr< AsyncStartRecvAlarmItem> cmditem = make_shared<AsyncStartRecvAlarmItem>();
	cmditem->cmd = make_shared<CMDStartRecvAlarm>();

	cmditem->start(cmditem->cmd, internal, timeoutms);

	return true;
}
struct AsyncRecvAlarmItem :public AyncGetItem
{
	shared_ptr<CMDGetAlarm> cmd;

	OnvifClient::RecvAlarmCallback		callback;

	void doResult()
	{
		callback(result ? cmd->alarminfo : NULL);
	}
};


shared_ptr<OnvifClientDefs::RecvAlarmInfo> OnvifClient::recvAlarm(const shared_ptr<OnvifClientDefs::StartRecvAlarm>& alarminfo, int timeoutms)
{
	if (alarminfo == NULL) return shared_ptr<OnvifClientDefs::RecvAlarmInfo>();

	shared_ptr< AsyncRecvAlarmItem> cmditem = make_shared<AsyncRecvAlarmItem>();
	cmditem->cmd = make_shared<CMDGetAlarm>();

	cmditem->start(cmditem->cmd, internal, timeoutms);

	if (!cmditem->wait(timeoutms)) return shared_ptr<OnvifClientDefs::RecvAlarmInfo>();

	return cmditem->cmd->alarminfo;
}

bool OnvifClient::asyncRecvAlarm(const RecvAlarmCallback& callback, const shared_ptr<OnvifClientDefs::StartRecvAlarm>& alarminfo, int timeoutms)
{
	if (alarminfo == NULL) return false;

	shared_ptr< AsyncRecvAlarmItem> cmditem = make_shared<AsyncRecvAlarmItem>();
	cmditem->cmd = make_shared<CMDGetAlarm>();

	cmditem->start(cmditem->cmd, internal, timeoutms);

	return true;
}

bool OnvifClient::stopRecvAlarm()
{
	return true;
}

}
}

