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
	AyncGetItem() :parseSuccess(false),success(false) {}
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

	OnvifResult result()
	{
		if (response == NULL || response->statusCode() == 408) return OnvifResult_ConnectError;
		else if (response && response->statusCode() == 401) return OnvifResult_AuthenError;
		else if (response && response->statusCode() != 200) return OnvifResult_RequestError;
		else if (response && response->statusCode() == 200 && !parseSuccess) return OnvifResult_ParseError;
		else if (response && response->statusCode() == 200 && !success) return OnvifResult_ResponseError;
		else if (success) return OnvifResult_OK;

		return OnvifResult_ResponseError;
	}

	virtual void doResult() = 0;
private:
	void httpCallback(const shared_ptr<HTTPClientRequest>&res, const shared_ptr<HTTPClientResponse>&_response)
	{
		response = _response;
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

			parseSuccess = true;

			success = cmdtmp->parse(body);
		} while (0);

		cmdsem.post();
		doResult();

		internal->delAsyncItem(this);
	}
private:
	shared_ptr<HTTPClientResponse>	response;
	weak_ptr<CmdObject>				cmdptr;
	Base::Semaphore					cmdsem;

	OnvifClient::OnvifClientInternal* internal;
	bool							parseSuccess;
	bool							success;
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
		callback(result(),cmd->devinfo);
	}
};

OnvifResult OnvifClient::getInfo(OnvifClientDefs::Info& info,int timeoutms)
{
	shared_ptr< AsyncGetInfoItem> cmditem = make_shared<AsyncGetInfoItem>();
	cmditem->cmd = make_shared<CMDGetDeviceInformation>();

	cmditem->start(cmditem->cmd, internal, timeoutms);

	cmditem->wait(timeoutms);
	info = cmditem->cmd->devinfo;

	return cmditem->result();
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
		callback(result(),cmd->capabilities);
	}
};


OnvifResult OnvifClient::getCapabities(OnvifClientDefs::Capabilities& cap,int timeoutms)
{
	shared_ptr< AsyncGetCapabilitiesItem> cmditem = make_shared<AsyncGetCapabilitiesItem>();
	cmditem->cmd = make_shared<CMDGetCapabilities>();

	cmditem->start(cmditem->cmd, internal, timeoutms);

	cmditem->wait(timeoutms);

	cap = cmditem->cmd->capabilities;

	return cmditem->result();
}

bool OnvifClient::asyncGetCapabities(const GetCapabilitiesCallback& callbac, int timeoutms)
{
	shared_ptr< AsyncGetCapabilitiesItem> cmditem = make_shared<AsyncGetCapabilitiesItem>();
	cmditem->cmd = make_shared<CMDGetCapabilities>();

	cmditem->start(cmditem->cmd, internal, timeoutms);

	return true;
}

struct AsyncGetProfilesItem :public AyncGetItem
{
	shared_ptr<CMDGetProfiles> cmd;

	OnvifClient::GetProfilesCallback		callback;

	void doResult()
	{
		callback(result(),cmd->profileInfo);
	}
};
OnvifResult OnvifClient::getProfiles(OnvifClientDefs::Profiles& profs,int timeoutms)
{
	shared_ptr< AsyncGetProfilesItem> cmditem = make_shared<AsyncGetProfilesItem>();
	cmditem->cmd = make_shared<CMDGetProfiles>();

	cmditem->start(cmditem->cmd, internal, timeoutms);

	cmditem->wait(timeoutms);

	profs = cmditem->cmd->profileInfo;

	return cmditem->result();
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
		callback(result(),cmd->streamurl);
	}
};
OnvifResult OnvifClient::getStreamUrl(const OnvifClientDefs::ProfileInfo& info, OnvifClientDefs::StreamUrl& streamurl,int timeoutms)
{
	shared_ptr< AsyncGetStreamUrlItem> cmditem = make_shared<AsyncGetStreamUrlItem>();
	cmditem->cmd = make_shared<CmdGetStreamURL>(info.token);

	cmditem->start(cmditem->cmd, internal, timeoutms);

	cmditem->wait(timeoutms);
	
	streamurl = cmditem->cmd->streamurl;

	return cmditem->result();
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
		callback(result(),cmd->snapurl);
	}
};

OnvifResult OnvifClient::getSnapUrl(const OnvifClientDefs::ProfileInfo& info, OnvifClientDefs::SnapUrl& snapurl,int timeoutms)
{
	shared_ptr< AsyncGetSnapUrlItem> cmditem = make_shared<AsyncGetSnapUrlItem>();
	cmditem->cmd = make_shared<CmdGetSnapURL>(info.token);

	cmditem->start(cmditem->cmd, internal, timeoutms);

	cmditem->wait(timeoutms);

	snapurl = cmditem->cmd->snapurl;

	return cmditem->result();
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
		callback(result(),cmd->network);
	}
};

OnvifResult OnvifClient::getNetworkInterfaces(OnvifClientDefs::NetworkInterfaces& network,int timeoutms)
{
	shared_ptr< AsyncGetNetworkInterfacesItem> cmditem = make_shared<AsyncGetNetworkInterfacesItem>();
	cmditem->cmd = make_shared<CmdGetNetworkInterfaces>();

	cmditem->start(cmditem->cmd, internal, timeoutms);

	cmditem->wait(timeoutms);

	network = cmditem->cmd->network;


	return cmditem->result();
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
		callback(result());
	}
};
OnvifResult OnvifClient::continuousMove(const OnvifClientDefs::ProfileInfo& info, const OnvifClientDefs::PTZCtrl& ptzctrl, int timeoutms)
{
	shared_ptr< AsyncContinuousMoveItem> cmditem = make_shared<AsyncContinuousMoveItem>();
	cmditem->cmd = make_shared<CmdContinuousMove>(ptzctrl, info.token);

	cmditem->start(cmditem->cmd, internal, timeoutms);

	cmditem->wait(timeoutms);

	return cmditem->result();
}
bool OnvifClient::asyncContinuousMoveCallback(const ContinuousMoveCallback& callback, const OnvifClientDefs::ProfileInfo& info, const OnvifClientDefs::PTZCtrl& ptzctrl, int timeoutms)
{
	shared_ptr< AsyncContinuousMoveItem> cmditem = make_shared<AsyncContinuousMoveItem>();
	cmditem->cmd = make_shared<CmdContinuousMove>(ptzctrl, info.token);

	cmditem->start(cmditem->cmd, internal, timeoutms);

	return true;
}

struct AsyncStopPtzItem :public AyncGetItem
{
	shared_ptr<CmdStopPTZ> cmd;

	OnvifClient::StopPTRCallback		callback;

	void doResult()
	{
		callback(result());
	}
};
OnvifResult OnvifClient::stopPTZ(const OnvifClientDefs::ProfileInfo& info, const OnvifClientDefs::PTZCtrl& ptzctrl, int timeoutms)
{
	shared_ptr< AsyncStopPtzItem> cmditem = make_shared<AsyncStopPtzItem>();
	cmditem->cmd = make_shared<CmdStopPTZ>(ptzctrl, info.token);

	cmditem->start(cmditem->cmd, internal, timeoutms);

	cmditem->wait(timeoutms);

	return cmditem->result();
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
		callback(result());
	}
};

OnvifResult OnvifClient::setPreset(const OnvifClientDefs::ProfileInfo& info, const std::string& presetname, int timeoutms)
{
	shared_ptr< AsyncSetPresestItem> cmditem = make_shared<AsyncSetPresestItem>();
	cmditem->cmd = make_shared<CmdSetPreset>(presetname, info.token);

	cmditem->start(cmditem->cmd, internal, timeoutms);

	cmditem->wait(timeoutms);

	return cmditem->result();
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
		callback(result(), cmd->preset);
	}
};

OnvifResult OnvifClient::getPreset(const OnvifClientDefs::ProfileInfo& info, OnvifClientDefs::PresetInfos& presetinfo,int timeoutms)
{
	shared_ptr< AsyncGetPresestItem> cmditem = make_shared<AsyncGetPresestItem>();
	cmditem->cmd = make_shared<CmdGetPresets>(info.token);

	cmditem->start(cmditem->cmd, internal, timeoutms);

	cmditem->wait(timeoutms);

	presetinfo = cmditem->cmd->preset;

	return cmditem->result();
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
		callback(result());
	}
};
OnvifResult  OnvifClient::gotoPreset(const OnvifClientDefs::ProfileInfo& info, const OnvifClientDefs::PresetInfo& presetinfo, int timeoutms)
{
	shared_ptr< AsyncGotoPresetItem> cmditem = make_shared<AsyncGotoPresetItem>();
	cmditem->cmd = make_shared<CmdGotoPreset>(presetinfo, info.token);

	cmditem->start(cmditem->cmd, internal, timeoutms);

	cmditem->wait(timeoutms);

	return cmditem->result();
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
		callback(result());
	}
};

OnvifResult  OnvifClient::removePreset(const OnvifClientDefs::ProfileInfo& info, const OnvifClientDefs::PresetInfo& presetinfo, int timeoutms)
{
	shared_ptr< AsyncRemovePresetItem> cmditem = make_shared<AsyncRemovePresetItem>();
	cmditem->cmd = make_shared<CmdRemovePreset>(presetinfo, info.token);

	cmditem->start(cmditem->cmd, internal, timeoutms);

	cmditem->wait(timeoutms);

	return cmditem->result();
}
bool OnvifClient::asyncRemovePreset(const RemovePresetCallback& callback, const OnvifClientDefs::ProfileInfo& info, const OnvifClientDefs::PresetInfo& presetinfo, int timeoutms)
{
	shared_ptr< AsyncRemovePresetItem> cmditem = make_shared<AsyncRemovePresetItem>();
	cmditem->cmd = make_shared<CmdRemovePreset>(presetinfo, info.token);

	cmditem->start(cmditem->cmd, internal, timeoutms);

	return true;
}

struct AsyncGetSystemDatetimeItem :public AyncGetItem
{
	shared_ptr<CmdGetSystemDateAndTime> cmd;

	OnvifClient::GetSystemDateTimeCallback		callback;

	void doResult()
	{
		callback(result(), cmd->time);
	}
};
OnvifResult OnvifClient::getSystemDatetime(Time& time,int timeoutms)
{
	shared_ptr< AsyncGetSystemDatetimeItem> cmditem = make_shared<AsyncGetSystemDatetimeItem>();
	cmditem->cmd = make_shared<CmdGetSystemDateAndTime>();

	cmditem->start(cmditem->cmd, internal, timeoutms);

	cmditem->wait(timeoutms);

	time = cmditem->cmd->time;

	return cmditem->result();
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
		callback(result());
	}
};

OnvifResult OnvifClient::SetSystemDatetime(const Time& time, int timeoutms)
{
	shared_ptr< AsyncSetSystemDatetimeItem> cmditem = make_shared<AsyncSetSystemDatetimeItem>();
	cmditem->cmd = make_shared<CmdSetSystemDateAndTime>(time);

	cmditem->start(cmditem->cmd, internal, timeoutms);

	cmditem->wait(timeoutms);

	return cmditem->result();
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
		callback(result());
	}
};

OnvifResult OnvifClient::systemReboot(int timeoutms)
{
	shared_ptr< AsyncSystemRebootItem> cmditem = make_shared<AsyncSystemRebootItem>();
	cmditem->cmd = make_shared<CMDSystemReboot>();

	cmditem->start(cmditem->cmd, internal, timeoutms);

	cmditem->wait(timeoutms);

	return cmditem->result();
}
bool OnvifClient::asyncSystemReboot(const SystemRebootCallback& callback, int timeoutms)
{
	shared_ptr< AsyncSystemRebootItem> cmditem = make_shared<AsyncSystemRebootItem>();
	cmditem->cmd = make_shared<CMDSystemReboot>();

	cmditem->start(cmditem->cmd, internal, timeoutms);

	return true;
}

struct AsyncSubscribeEventItem :public AyncGetItem
{
	shared_ptr<CMDSubEvent> cmd;

	OnvifClient::SubscribeEventCallback		callback;

	void doResult()
	{
		callback(result(), cmd->subeventresp);
	}
};

OnvifResult  OnvifClient::subscribeEvent(const OnvifClientDefs::Capabilities& capabilities, OnvifClientDefs::SubEventResponse& subeventresp,int timeoutms)
{
	if (!capabilities.Events.Support)
	{
		return OnvifResult_NotSupport;
	}

	shared_ptr< AsyncSubscribeEventItem> cmditem = make_shared<AsyncSubscribeEventItem>();
	cmditem->cmd = make_shared<CMDSubEvent>();

	cmditem->start(cmditem->cmd, internal, timeoutms);

	cmditem->wait(timeoutms);

	subeventresp = cmditem->cmd->subeventresp;

	return cmditem->result();
}
bool OnvifClient::asyncSubscribeEvent(const SubscribeEventCallback& callback, const OnvifClientDefs::Capabilities& capabilities, int timeoutms)
{
	if (!capabilities.Events.Support)
	{
		return false;
	}

	shared_ptr< AsyncSubscribeEventItem> cmditem = make_shared<AsyncSubscribeEventItem>();
	cmditem->cmd = make_shared<CMDSubEvent>();

	cmditem->start(cmditem->cmd, internal, timeoutms);

	return true;
}
struct AsyncGetEventItem :public AyncGetItem
{
	shared_ptr<CMDGetAlarm> cmd;

	OnvifClient::GetEventCallback		callback;

	void doResult()
	{
		callback(result(),cmd->eventinfos);
	}
};


OnvifResult OnvifClient::getEvent(const OnvifClientDefs::SubEventResponse& subeventresp, OnvifClientDefs::EventInfos& eventinfos, int timeoutms)
{
	shared_ptr< AsyncGetEventItem> cmditem = make_shared<AsyncGetEventItem>();
	cmditem->cmd = make_shared<CMDGetAlarm>();

	cmditem->start(cmditem->cmd, internal, timeoutms, subeventresp.xaddr);

	cmditem->wait(timeoutms);
	eventinfos = cmditem->cmd->eventinfos;

	return cmditem->result();
}

bool OnvifClient::asyncGetEvent(const GetEventCallback& callback, const OnvifClientDefs::SubEventResponse& subeventresp, int timeoutms)
{
	shared_ptr< AsyncGetEventItem> cmditem = make_shared<AsyncGetEventItem>();
	cmditem->cmd = make_shared<CMDGetAlarm>();

	cmditem->start(cmditem->cmd, internal, timeoutms, subeventresp.xaddr);

	return true;
}

bool OnvifClient::stopSubEvent()
{
	return true;
}

}
}

