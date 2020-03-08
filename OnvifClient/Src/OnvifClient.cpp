#include "OnvifClient/OnvifClient.h"
#include "protocol/OnvifProtocol.h"

namespace Public
{
namespace Onvif
{

#define ONVIFECONENTTYPE "application/soap+xml"

struct OnvifClientCmdManager;
struct AyncGetItem : public enable_shared_from_this<AyncGetItem>
{
    uint64_t startt = Time::getCurrentMilliSecond();

public:
    AyncGetItem() : parseSuccess(false), success(false) {}
    virtual ~AyncGetItem()
    {
    }

    void close()
    {
        if (httpclient)
            httpclient->disconnect();
    }

    bool wait(uint32_t timeout)
    {
        return cmdsem.pend(timeout) >= 0;
    }
    virtual void start(const shared_ptr<CmdObject> &cmdptr, const shared_ptr<OnvifClientCmdManager> &_cmdmanager, uint32_t timeout, const URL &_requrl = URL());
    virtual void start(const shared_ptr<HTTP::ClientRequest> &req, const shared_ptr<OnvifClientCmdManager> &_cmdmanager);
    ErrorInfo result()
    {
        ErrorInfo ret = checkHTTPResponse(response);
        if (ret.errcode == Error_Code_Request)
        {
            if (!parseSuccess)
                return ErrorInfo(Error_Code_Response, "回复失败");
            else if (!success)
                return ErrorInfo(Error_Code_ParseObject, "解析错误");
            else if (success)
                return ErrorInfo();

            return ErrorInfo(Error_Code_Fail, "未知错误");
        }

        return ret;
    }

    static ErrorInfo checkHTTPResponse(const shared_ptr<HTTP::ClientResponse> &response)
    {
        if (response == NULL || response->header()->statuscode == 408)
            return ErrorInfo(Error_Code_ConnectTimeout, "通讯超时");
        else if (response && response->header()->statuscode == 401)
            return ErrorInfo(Error_Code_Authen, "权限认证失败");
        else if (response && response->header()->statuscode != 200)
            return ErrorInfo(Error_Code_Request, "请求错误");

        return ErrorInfo();
    }

    virtual void doResult() = 0;
    shared_ptr<HTTP::ClientResponse> httpResponse()
    {
        if (response == NULL)
        {
            response = make_shared<HTTP::ClientResponse>(shared_ptr<HTTP::Communication>(), HTTP::CacheType_Mem);

            response->header()->statuscode = 408;
            response->header()->statusmsg = "Request Timeout";
        }
        return response;
    }
    const std::string &httpResponseBody() { return httpbody; }

private:
    void httpCallback(const shared_ptr<HTTP::ClientRequest> &res, const shared_ptr<HTTP::ClientResponse> &_response);

protected:
    shared_ptr<HTTP::ClientResponse> response;
    shared_ptr<CmdObject> cmd;
    std::string httpbody;
    Base::Semaphore cmdsem;

private:
    weak_ptr<OnvifClientCmdManager> cmdmanager;
    bool parseSuccess;
    bool success;
    shared_ptr<HTTP::Client> httpclient;
};
struct OnvifClientCmdManager
{
    shared_ptr<IOWorker> worker;
    URL url;
    shared_ptr<HTTP::AsyncClient> asyncclient;
    std::string useragent;

    Time deviceSystemTime = Time::getCurrentTime();
    uint64_t updateSystemTimeTick = Time::getCurrentMilliSecond();

    Time getSystemTimeByDeviceTime()
    {
        Time devicetime = deviceSystemTime;
        uint64_t updateDeviceTick = updateSystemTimeTick;

        devicetime += (Time::getCurrentMilliSecond() - updateDeviceTick) / 1000;
        return devicetime;
    }
};

void AyncGetItem::start(const shared_ptr<CmdObject> &cmdptr, const shared_ptr<OnvifClientCmdManager> &_cmdmanager, uint32_t timeout, const URL &_requrl)
{
    cmd = cmdptr;

    shared_ptr<HTTP::ClientRequest> req = make_shared<HTTP::ClientRequest>();

    URL requrl = _requrl;
    if (requrl.getHostname() == "")
        requrl = _cmdmanager->url;
    if (requrl.getPath() == "" || requrl.getPath() == "/")
    {
        requrl.setPath(cmd->requesturl);
    }

    requrl.setAuthen(_cmdmanager->url.getAuhen());

    {
        req->header()->headers["Content-Type"] = std::string(ONVIFECONENTTYPE) + "; charset=utf-8"; //; action = \"" + cmd->action + "\"";
                                                                                                    //		req->header()->headers["Accept-Encoding"] = "gzip, deflate";
        if (_cmdmanager->useragent != "")
            req->header()->headers["User-Agent"] = _cmdmanager->useragent;
        req->header()->headers["Connection"] = "close";
    }

    cmd->initGSopProtocol();
    cmd->header().security.createTime = _cmdmanager->getSystemTimeByDeviceTime();
    req->content()->write(cmd->build(requrl));

    req->header()->method = "POST";
    req->timeout() = timeout;
    req->header()->url = requrl.getHost() + requrl.getPath() + requrl.getSearch();

    start(req, _cmdmanager);
}
void AyncGetItem::start(const shared_ptr<HTTP::ClientRequest> &req, const shared_ptr<OnvifClientCmdManager> &_cmdmanager)
{
    if (httpclient == NULL)
    {
        httpclient = make_shared<HTTP::Client>(_cmdmanager->worker, _cmdmanager->useragent);
    }
    cmdmanager = _cmdmanager;

    httpclient->request(_cmdmanager->asyncclient, req, HTTP::Client::HTTPCallback(&AyncGetItem::httpCallback, shared_from_this()));
}

void AyncGetItem::httpCallback(const shared_ptr<HTTP::ClientRequest> &res, const shared_ptr<HTTP::ClientResponse> &_response)
{
    response = _response;
    do
    {
        httpbody = response->content()->read();
        if (String::indexOfByCase(httpbody, "NotAuthorized") != (size_t)-1)
        {
            response->header()->statuscode = 401;
            response->header()->statusmsg = "NotAuthorized";
            break;
        }

        if (response == NULL || response->header()->statuscode != 200)
        {
            break;
        }

        Value contenttypeval = response->header()->header("Content-Type");
        if (strstr(String::tolower(contenttypeval.readString()).c_str(), ONVIFECONENTTYPE) == 0)
        {
            break;
        }

        shared_ptr<CmdObject> cmdtmp = cmd;
        if (cmdtmp)
        {
            if (!cmdtmp->parseGSopProtocol(httpbody))
            {
                break;
            }
            success = cmdtmp->parseProtocol();
        }

        parseSuccess = true;
    } while (0);

    if (cmd != NULL)
        cmdsem.post();

    doResult();
}

struct OnvifClient::OnvifClientInternal
{
    shared_ptr<OnvifClientCmdManager> cmdmanager;
};
OnvifClient::OnvifClient(const shared_ptr<IOWorker> &worker, const shared_ptr<HTTP::AsyncClient> &asyncclient, const URL &url, const std::string &useragent)
{
    internal = new OnvifClientInternal;
    internal->cmdmanager = make_shared<OnvifClientCmdManager>();
    internal->cmdmanager->worker = worker;
    internal->cmdmanager->url = url;
    internal->cmdmanager->asyncclient = asyncclient;
    internal->cmdmanager->useragent = useragent;
}
void OnvifClient::onPoolTimerProc()
{
}
OnvifClient::~OnvifClient()
{
    internal->cmdmanager->asyncclient = NULL;

    SAFE_DELETE(internal);
}
const URL &OnvifClient::OnvifUrl() const
{
    return internal->cmdmanager->url;
}
struct AsyncGetInfoItem : public AyncGetItem
{
    OnvifClient::GetInfoCalblack callback;

    void doResult()
    {
        callback(result(), ((CMDGetDeviceInformation *)cmd.get())->devinfo);
    }
};

ErrorInfo OnvifClient::getInfo(OnvifClientDefs::Info &info, int timeoutms)
{
    shared_ptr<AsyncGetInfoItem> cmditem = make_shared<AsyncGetInfoItem>();
    shared_ptr<CMDGetDeviceInformation> cmd = make_shared<CMDGetDeviceInformation>();

    cmditem->start(cmd, internal->cmdmanager, timeoutms);
    cmditem->wait(timeoutms);
    info = cmd->devinfo;

    return cmditem->result();
}

ErrorInfo OnvifClient::asyncGetInfo(const GetInfoCalblack &callback, int timeoutms)
{
    shared_ptr<AsyncGetInfoItem> cmditem = make_shared<AsyncGetInfoItem>();
    shared_ptr<CMDGetDeviceInformation> cmd = make_shared<CMDGetDeviceInformation>();
    cmditem->callback = callback;

    cmditem->start(cmd, internal->cmdmanager, timeoutms);

    return ErrorInfo();
}

struct AsyncGetCapabilitiesItem : public AyncGetItem
{
    OnvifClient::GetCapabilitiesCallback callback;

    void doResult()
    {
        callback(result(), ((CMDGetCapabilities *)cmd.get())->capabilities);
    }
};

ErrorInfo OnvifClient::getCapabities(OnvifClientDefs::Capabilities &cap, int timeoutms)
{
    shared_ptr<AsyncGetCapabilitiesItem> cmditem = make_shared<AsyncGetCapabilitiesItem>();
    shared_ptr<CMDGetCapabilities> cmd = make_shared<CMDGetCapabilities>();

    cmditem->start(cmd, internal->cmdmanager, timeoutms);
    cmditem->wait(timeoutms);

    cap = cmd->capabilities;

    return cmditem->result();
}

ErrorInfo OnvifClient::asyncGetCapabities(const GetCapabilitiesCallback &callback, int timeoutms)
{
    shared_ptr<AsyncGetCapabilitiesItem> cmditem = make_shared<AsyncGetCapabilitiesItem>();
    shared_ptr<CMDGetCapabilities> cmd = make_shared<CMDGetCapabilities>();
    cmditem->callback = callback;

    cmditem->start(cmd, internal->cmdmanager, timeoutms);

    return ErrorInfo();
}

struct AsyncGetProfilesItem : public AyncGetItem
{
    OnvifClient::GetProfilesCallback callback;

    void doResult()
    {
        callback(result(), ((CMDGetProfiles *)cmd.get())->profileInfo);
    }
};
ErrorInfo OnvifClient::getProfiles(OnvifClientDefs::Profiles &profs, int timeoutms)
{
    shared_ptr<AsyncGetProfilesItem> cmditem = make_shared<AsyncGetProfilesItem>();
    shared_ptr<CMDGetProfiles> cmd = make_shared<CMDGetProfiles>();

    cmditem->start(cmd, internal->cmdmanager, timeoutms);
    cmditem->wait(timeoutms);

    profs = cmd->profileInfo;

    return cmditem->result();
}
ErrorInfo OnvifClient::asyncGetProfiles(const GetProfilesCallback &callback, int timeoutms)
{
    shared_ptr<AsyncGetProfilesItem> cmditem = make_shared<AsyncGetProfilesItem>();
    shared_ptr<CMDGetProfiles> cmd = make_shared<CMDGetProfiles>();
    cmditem->callback = callback;

    cmditem->start(cmd, internal->cmdmanager, timeoutms);

    return ErrorInfo();
}

struct AsyncGetStreamUrlItem : public AyncGetItem
{
    OnvifClient::GetStreamUrlCallback callback;

    void doResult()
    {
        callback(result(), ((CmdGetStreamURL *)cmd.get())->streamurl);
    }
};
ErrorInfo OnvifClient::getStreamUrl(const OnvifClientDefs::ProfileInfo &info, OnvifClientDefs::StreamUrl &streamurl, int timeoutms)
{
    shared_ptr<AsyncGetStreamUrlItem> cmditem = make_shared<AsyncGetStreamUrlItem>();
    shared_ptr<CmdGetStreamURL> cmd = make_shared<CmdGetStreamURL>(info.token);

    cmditem->start(cmd, internal->cmdmanager, timeoutms);
    cmditem->wait(timeoutms);

    streamurl = cmd->streamurl;

    return cmditem->result();
}
ErrorInfo OnvifClient::asyncGetStreamUrl(const GetStreamUrlCallback &callback, const OnvifClientDefs::ProfileInfo &info, int timeoutms)
{
    shared_ptr<AsyncGetStreamUrlItem> cmditem = make_shared<AsyncGetStreamUrlItem>();
    shared_ptr<CmdGetStreamURL> cmd = make_shared<CmdGetStreamURL>(info.token);
    cmditem->callback = callback;

    cmditem->start(cmd, internal->cmdmanager, timeoutms);

    return ErrorInfo();
}

struct AsyncGetSnapUrlItem : public AyncGetItem
{
    OnvifClient::GetSnapUrlCallback callback;

    void doResult()
    {
        callback(result(), ((CmdGetSnapURL *)cmd.get())->snapurl);
    }
};

ErrorInfo OnvifClient::getSnapUrl(const OnvifClientDefs::ProfileInfo &info, OnvifClientDefs::SnapUrl &snapurl, int timeoutms)
{
    shared_ptr<AsyncGetSnapUrlItem> cmditem = make_shared<AsyncGetSnapUrlItem>();
    shared_ptr<CmdGetSnapURL> cmd = make_shared<CmdGetSnapURL>(info.token);

    cmditem->start(cmd, internal->cmdmanager, timeoutms);
    cmditem->wait(timeoutms);

    snapurl = cmd->snapurl;

    return cmditem->result();
}
ErrorInfo OnvifClient::asyncGetSnapUrl(const GetSnapUrlCallback &callback, const OnvifClientDefs::ProfileInfo &info, int timeoutms)
{
    shared_ptr<AsyncGetSnapUrlItem> cmditem = make_shared<AsyncGetSnapUrlItem>();
    shared_ptr<CmdGetSnapURL> cmd = make_shared<CmdGetSnapURL>(info.token);
    cmditem->callback = callback;

    cmditem->start(cmd, internal->cmdmanager, timeoutms);

    return ErrorInfo();
}

struct AsyncGetNetworkInterfacesItem : public AyncGetItem
{
    OnvifClient::GetNetworkInterfacesCallback callback;

    void doResult()
    {
        callback(result(), ((CmdGetNetworkInterfaces *)cmd.get())->network);
    }
};

ErrorInfo OnvifClient::getNetworkInterfaces(OnvifClientDefs::NetworkInterfaces &network, int timeoutms)
{
    shared_ptr<AsyncGetNetworkInterfacesItem> cmditem = make_shared<AsyncGetNetworkInterfacesItem>();
    shared_ptr<CmdGetNetworkInterfaces> cmd = make_shared<CmdGetNetworkInterfaces>();

    cmditem->start(cmd, internal->cmdmanager, timeoutms);
    cmditem->wait(timeoutms);

    network = cmd->network;

    return cmditem->result();
}

ErrorInfo OnvifClient::asyncGetNetworkInterfaces(const GetNetworkInterfacesCallback &callback, int timeoutms)
{
    shared_ptr<AsyncGetNetworkInterfacesItem> cmditem = make_shared<AsyncGetNetworkInterfacesItem>();
    shared_ptr<CmdGetNetworkInterfaces> cmd = make_shared<CmdGetNetworkInterfaces>();
    cmditem->callback = callback;

    cmditem->start(cmd, internal->cmdmanager, timeoutms);

    return ErrorInfo();
}

//shared_ptr<OnvifClientDefs::VideoEncoderConfigurations> OnvifClient::getVideoEncoderConfigurations(int timeoutms)
//{
//	shared_ptr<CmdGetVideoEncoderConfigurations> cmd = make_shared<CmdGetVideoEncoderConfigurations>();
//
//	internal->sendOvifRequest(cmd.get(), timeoutms);
//
//	return cmd->encoder;
//}


struct AsyncGetImageSettingItem : public AyncGetItem
{
    OnvifClient::GetImageSettingCallback callback;

    void doResult()
    {
        callback(result(), ((CmdGetImageSettings *)cmd.get())->settingInfo);
    }
};

ErrorInfo OnvifClient::asyncGetImageSetting(const OnvifClientDefs::ProfileInfo& info, const GetImageSettingCallback& callback, int timeoutms)
{
    shared_ptr<AsyncGetImageSettingItem> cmditem = make_shared<AsyncGetImageSettingItem>();
    shared_ptr<CmdGetImageSettings> cmd = make_shared<CmdGetImageSettings>(info.videoSource->source_token);
    cmditem->callback = callback;

    cmditem->start(cmd, internal->cmdmanager, timeoutms);

    return ErrorInfo();
}

struct AsyncSetImageSettingItem : public AyncGetItem
{
    OnvifClient::SetImageSettingCallback callback;

    void doResult()
    {
        callback(result());
    }
};

ErrorInfo OnvifClient::asyncSetImageSetting(const OnvifClientDefs::ProfileInfo& info, const OnvifClientDefs::ImageSettingInfo& setting, const SetImageSettingCallback& callback, int timeoutms)
{
    shared_ptr<AsyncSetImageSettingItem> cmditem = make_shared<AsyncSetImageSettingItem>();
    shared_ptr<CmdSetImageSettings> cmd = make_shared<CmdSetImageSettings>(info.videoSource->source_token, setting);
    cmditem->callback = callback;

    cmditem->start(cmd, internal->cmdmanager, timeoutms);

    return ErrorInfo();
}

struct AsyncGetImageOptionsItem : public AyncGetItem
{
	OnvifClient::GetImageOptionsCallback callback;

	void doResult()
	{
		callback(result(), ((CmdGetImageOptions *)cmd.get())->options);
	}
};

ErrorInfo OnvifClient::asyncGetImageOptions(const OnvifClientDefs::ProfileInfo& info, const GetImageOptionsCallback& callback, int timeoutms)
{
	shared_ptr<AsyncGetImageOptionsItem> cmditem = make_shared<AsyncGetImageOptionsItem>();
	shared_ptr<CmdGetImageOptions> cmd = make_shared<CmdGetImageOptions>(info.videoSource->source_token);
	cmditem->callback = callback;

	cmditem->start(cmd, internal->cmdmanager, timeoutms);

	return ErrorInfo();
}

struct AsyncContinuousMoveItem : public AyncGetItem
{
    OnvifClient::ContinuousMoveCallback callback;

    void doResult()
    {
        callback(result());
    }
};
ErrorInfo OnvifClient::continuousMove(const OnvifClientDefs::ProfileInfo &info, const OnvifClientDefs::PTZCtrl &ptzctrl, int timeoutms)
{
    shared_ptr<AsyncContinuousMoveItem> cmditem = make_shared<AsyncContinuousMoveItem>();
    shared_ptr<CmdContinuousMove> cmd = make_shared<CmdContinuousMove>(ptzctrl, info.token);

    cmditem->start(cmd, internal->cmdmanager, timeoutms);
    cmditem->wait(timeoutms);

    return cmditem->result();
}
ErrorInfo OnvifClient::asyncContinuousMove(const ContinuousMoveCallback &callback, const OnvifClientDefs::ProfileInfo &info, const OnvifClientDefs::PTZCtrl &ptzctrl, int timeoutms)
{
    shared_ptr<AsyncContinuousMoveItem> cmditem = make_shared<AsyncContinuousMoveItem>();
    shared_ptr<CmdContinuousMove> cmd = make_shared<CmdContinuousMove>(ptzctrl, info.token);
    cmditem->callback = callback;

    cmditem->start(cmd, internal->cmdmanager, timeoutms);

    return ErrorInfo();
}

struct AsyncStopPtzItem : public AyncGetItem
{
    OnvifClient::StopPTRCallback callback;

    void doResult()
    {
        callback(result());
    }
};
ErrorInfo OnvifClient::stopPTZ(const OnvifClientDefs::ProfileInfo &info, const OnvifClientDefs::PTZCtrl &ptzctrl, int timeoutms)
{
    shared_ptr<AsyncStopPtzItem> cmditem = make_shared<AsyncStopPtzItem>();
    shared_ptr<CmdStopPTZ> cmd = make_shared<CmdStopPTZ>(ptzctrl, info.token);

    cmditem->start(cmd, internal->cmdmanager, timeoutms);
    cmditem->wait(timeoutms);

    return cmditem->result();
}
ErrorInfo OnvifClient::asyncStopPtz(const StopPTRCallback &callback, const OnvifClientDefs::ProfileInfo &info, const OnvifClientDefs::PTZCtrl &ptzctrl, int timeoutms)
{
    shared_ptr<AsyncStopPtzItem> cmditem = make_shared<AsyncStopPtzItem>();
    shared_ptr<CmdStopPTZ> cmd = make_shared<CmdStopPTZ>(ptzctrl, info.token);
    cmditem->callback = callback;

    cmditem->start(cmd, internal->cmdmanager, timeoutms);

    return ErrorInfo();
}

struct AsyncSetPresestItem : public AyncGetItem
{
    OnvifClient::SetPresetCallback callback;

    void doResult()
    {
        callback(result());
    }
};

ErrorInfo OnvifClient::setPreset(uint32_t index, const std::string& presetname, const OnvifClientDefs::ProfileInfo& info, int timeoutms)
{
    shared_ptr<AsyncSetPresestItem> cmditem = make_shared<AsyncSetPresestItem>();
    shared_ptr<CmdSetPreset> cmd = make_shared<CmdSetPreset>(index, presetname, info.token);

    cmditem->start(cmd, internal->cmdmanager, timeoutms);
    cmditem->wait(timeoutms);

    return cmditem->result();
}

ErrorInfo OnvifClient::asyncSetPresest(uint32_t index, const std::string& presetname, const OnvifClientDefs::ProfileInfo& info, const SetPresetCallback& callback, int timeoutms)
{
    shared_ptr<AsyncSetPresestItem> cmditem = make_shared<AsyncSetPresestItem>();
    shared_ptr<CmdSetPreset> cmd = make_shared<CmdSetPreset>(index, presetname, info.token);
    cmditem->callback = callback;

    cmditem->start(cmd, internal->cmdmanager, timeoutms);

    return ErrorInfo();
}

struct AsyncGetPresestItem : public AyncGetItem
{
    OnvifClient::GetPresetCallback callback;

    void doResult()
    {
        callback(result(), ((CmdGetPresets *)cmd.get())->preset);
    }
};

ErrorInfo OnvifClient::getPreset(const OnvifClientDefs::ProfileInfo &info, OnvifClientDefs::PresetInfos &presetinfo, int timeoutms)
{
    shared_ptr<AsyncGetPresestItem> cmditem = make_shared<AsyncGetPresestItem>();
    shared_ptr<CmdGetPresets> cmd = make_shared<CmdGetPresets>(info.token);

    cmditem->start(cmd, internal->cmdmanager, timeoutms);
    cmditem->wait(timeoutms);

    presetinfo = cmd->preset;

    return cmditem->result();
}
ErrorInfo OnvifClient::asyncGetPreset(const OnvifClientDefs::ProfileInfo &info, const GetPresetCallback &callback, int timeoutms)
{
    shared_ptr<AsyncGetPresestItem> cmditem = make_shared<AsyncGetPresestItem>();
    shared_ptr<CmdGetPresets> cmd = make_shared<CmdGetPresets>(info.token);
    cmditem->callback = callback;

    cmditem->start(cmd, internal->cmdmanager, timeoutms);

    return ErrorInfo();
}

struct AsyncGotoPresetItem : public AyncGetItem
{
    OnvifClient::GotoPresetCallback callback;

    void doResult()
    {
        callback(result());
    }
};

ErrorInfo OnvifClient::gotoPreset(uint32_t index, const OnvifClientDefs::ProfileInfo& info, int timeoutms)
{
    shared_ptr<AsyncGotoPresetItem> cmditem = make_shared<AsyncGotoPresetItem>();
    shared_ptr<CmdGotoPreset> cmd = make_shared<CmdGotoPreset>(index, info.token);

    cmditem->start(cmd, internal->cmdmanager, timeoutms);
    cmditem->wait(timeoutms);

    return cmditem->result();
}

ErrorInfo OnvifClient::asyncGotoPreset(uint32_t index, const OnvifClientDefs::ProfileInfo& info, const GotoPresetCallback& callback, int timeoutms)
{
    shared_ptr<AsyncGotoPresetItem> cmditem = make_shared<AsyncGotoPresetItem>();
    shared_ptr<CmdGotoPreset> cmd = make_shared<CmdGotoPreset>(index, info.token);
    cmditem->callback = callback;

    cmditem->start(cmd, internal->cmdmanager, timeoutms);

    return ErrorInfo();
}

struct AsyncRemovePresetItem : public AyncGetItem
{
    OnvifClient::RemovePresetCallback callback;

    void doResult()
    {
        callback(result());
    }
};

ErrorInfo OnvifClient::removePreset(uint32_t index, const OnvifClientDefs::ProfileInfo& info, int timeoutms)
{
    shared_ptr<AsyncRemovePresetItem> cmditem = make_shared<AsyncRemovePresetItem>();
    shared_ptr<CmdRemovePreset> cmd = make_shared<CmdRemovePreset>(index, info.token);

    cmditem->start(cmd, internal->cmdmanager, timeoutms);
    cmditem->wait(timeoutms);

    return cmditem->result();
}

ErrorInfo OnvifClient::asyncRemovePreset(uint32_t index, const OnvifClientDefs::ProfileInfo& info, const RemovePresetCallback& callback, int timeoutms)
{
    shared_ptr<AsyncRemovePresetItem> cmditem = make_shared<AsyncRemovePresetItem>();
    shared_ptr<CmdRemovePreset> cmd = make_shared<CmdRemovePreset>(index, info.token);
    cmditem->callback = callback;

    cmditem->start(cmd, internal->cmdmanager, timeoutms);

    return ErrorInfo();
}

struct AsyncGetSystemDatetimeItem : public AyncGetItem
{
    weak_ptr<OnvifClientCmdManager> cmdmanager;

    OnvifClient::GetSystemDateTimeCallback callback;

    void doResult()
    {
        shared_ptr<OnvifClientCmdManager> _cmdmamager = cmdmanager.lock();
        CmdGetSystemDateAndTime *datetimecmd = (CmdGetSystemDateAndTime *)cmd.get();
        if (!result() && _cmdmamager)
        {
            _cmdmamager->deviceSystemTime = datetimecmd->time;
            _cmdmamager->updateSystemTimeTick = Time::getCurrentMilliSecond();
        }

        callback(result(), datetimecmd->time);
    }
};
ErrorInfo OnvifClient::getSystemDatetime(Time &time, int timeoutms)
{
    shared_ptr<AsyncGetSystemDatetimeItem> cmditem = make_shared<AsyncGetSystemDatetimeItem>();
    shared_ptr<CmdGetSystemDateAndTime> cmd = make_shared<CmdGetSystemDateAndTime>();

    cmditem->start(cmd, internal->cmdmanager, timeoutms);
    cmditem->wait(timeoutms);

    time = cmd->time;

    if (!cmditem->result())
    {
        internal->cmdmanager->deviceSystemTime = time;
        internal->cmdmanager->updateSystemTimeTick = Time::getCurrentMilliSecond();
    }

    return cmditem->result();
}
ErrorInfo OnvifClient::asyncGetSystemDatetime(const GetSystemDateTimeCallback &callback, int timeoutms)
{
    shared_ptr<AsyncGetSystemDatetimeItem> cmditem = make_shared<AsyncGetSystemDatetimeItem>();
    shared_ptr<CmdGetSystemDateAndTime> cmd = make_shared<CmdGetSystemDateAndTime>();
    cmditem->callback = callback;
    cmditem->cmdmanager = internal->cmdmanager;

    cmditem->start(cmd, internal->cmdmanager, timeoutms);

    return ErrorInfo();
}

struct AsyncSetSystemDatetimeItem : public AyncGetItem
{
    weak_ptr<OnvifClientCmdManager> cmdmanager;

    OnvifClient::SetSystemDatetimeCallback callback;
    Time settime;
    void doResult()
    {
        shared_ptr<OnvifClientCmdManager> _cmdmamager = cmdmanager.lock();
        if (!result() && _cmdmamager)
        {
            _cmdmamager->deviceSystemTime = settime;
            _cmdmamager->updateSystemTimeTick = Time::getCurrentMilliSecond();
        }

        callback(result());
    }
};

ErrorInfo OnvifClient::SetSystemDatetime(const Time &time, int timeoutms)
{
    shared_ptr<AsyncSetSystemDatetimeItem> cmditem = make_shared<AsyncSetSystemDatetimeItem>();
    shared_ptr<CmdSetSystemDateAndTime> cmd = make_shared<CmdSetSystemDateAndTime>(time);

    cmditem->cmdmanager = internal->cmdmanager;
    cmditem->settime = time;

    cmditem->start(cmd, internal->cmdmanager, timeoutms);
    cmditem->wait(timeoutms);

    if (!cmditem->result())
    {
        internal->cmdmanager->deviceSystemTime = time;
        internal->cmdmanager->updateSystemTimeTick = Time::getCurrentMilliSecond();
    }

    return cmditem->result();
}
ErrorInfo OnvifClient::asyncSetSystemDatetime(const SetSystemDatetimeCallback &callback, const Time &time, int timeoutms)
{
    shared_ptr<AsyncSetSystemDatetimeItem> cmditem = make_shared<AsyncSetSystemDatetimeItem>();
    shared_ptr<CmdSetSystemDateAndTime> cmd = make_shared<CmdSetSystemDateAndTime>(time);

    cmditem->cmdmanager = internal->cmdmanager;
    cmditem->settime = time;
    cmditem->callback = callback;

    cmditem->start(cmd, internal->cmdmanager, timeoutms);

    return ErrorInfo();
}

struct AsyncSystemRebootItem : public AyncGetItem
{
    OnvifClient::SystemRebootCallback callback;

    void doResult()
    {
        callback(result());
    }
};

ErrorInfo OnvifClient::systemReboot(int timeoutms)
{
    shared_ptr<AsyncSystemRebootItem> cmditem = make_shared<AsyncSystemRebootItem>();
    shared_ptr<CMDSystemReboot> cmd = make_shared<CMDSystemReboot>();

    cmditem->start(cmd, internal->cmdmanager, timeoutms);
    cmditem->wait(timeoutms);

    return cmditem->result();
}
ErrorInfo OnvifClient::asyncSystemReboot(const SystemRebootCallback &callback, int timeoutms)
{
    shared_ptr<AsyncSystemRebootItem> cmditem = make_shared<AsyncSystemRebootItem>();
    shared_ptr<CMDSystemReboot> cmd = make_shared<CMDSystemReboot>();
    cmditem->callback = callback;

    cmditem->start(cmd, internal->cmdmanager, timeoutms);

    return ErrorInfo();
}

struct AsyncSubscribeEventItem : public AyncGetItem
{
    OnvifClient::SubscribeEventCallback callback;

    void doResult()
    {
        callback(result(), ((CMDSubEvent *)cmd.get())->subeventresp);
    }
};

ErrorInfo OnvifClient::subscribeEvent(const OnvifClientDefs::Capabilities &capabilities, OnvifClientDefs::SubEventResponse &subeventresp, int timeoutms)
{
    if (!capabilities.events.support)
    {
        return ErrorInfo(Error_Code_NotSupport, "不支持");
    }

    shared_ptr<AsyncSubscribeEventItem> cmditem = make_shared<AsyncSubscribeEventItem>();
    shared_ptr<CMDSubEvent> cmd = make_shared<CMDSubEvent>();

    cmditem->start(cmd, internal->cmdmanager, timeoutms);
    cmditem->wait(timeoutms);

    subeventresp = cmd->subeventresp;

    return cmditem->result();
}
ErrorInfo OnvifClient::asyncSubscribeEvent(const SubscribeEventCallback &callback, const OnvifClientDefs::Capabilities &capabilities, int timeoutms)
{
    if (!capabilities.events.support)
    {
        return ErrorInfo(Error_Code_NotSupport);
    }

    shared_ptr<AsyncSubscribeEventItem> cmditem = make_shared<AsyncSubscribeEventItem>();
    shared_ptr<CMDSubEvent> cmd = make_shared<CMDSubEvent>();
    cmditem->callback = callback;

    cmditem->start(cmd, internal->cmdmanager, timeoutms);

    return ErrorInfo();
}
struct AsyncGetEventItem : public AyncGetItem
{
    OnvifClient::GetEventCallback callback;

    void doResult()
    {
        callback(result(), ((CMDGetEvents *)cmd.get())->eventinfos);
    }
};

ErrorInfo OnvifClient::getEvent(const OnvifClientDefs::SubEventResponse &subeventresp, OnvifClientDefs::EventInfos &eventinfos, int timeoutms)
{
    shared_ptr<AsyncGetEventItem> cmditem = make_shared<AsyncGetEventItem>();
    shared_ptr<CMDGetEvents> cmd = make_shared<CMDGetEvents>();

    cmditem->start(cmd, internal->cmdmanager, timeoutms, subeventresp.xaddr);

    cmditem->wait(timeoutms);
    eventinfos = cmd->eventinfos;

    return cmditem->result();
}

ErrorInfo OnvifClient::asyncGetEvent(const GetEventCallback &callback, const OnvifClientDefs::SubEventResponse &subeventresp, int timeoutms)
{
    shared_ptr<AsyncGetEventItem> cmditem = make_shared<AsyncGetEventItem>();
    shared_ptr<CMDGetEvents> cmd = make_shared<CMDGetEvents>();
    cmditem->callback = callback;

    cmditem->start(cmd, internal->cmdmanager, timeoutms, subeventresp.xaddr);

    return ErrorInfo();
}

ErrorInfo OnvifClient::stopSubEvent()
{
    return ErrorInfo();
}

bool OnvifClient::checkDeviceTimeIsOverdue(const Time &devtime)
{
    Time devcurtime = internal->cmdmanager->getSystemTimeByDeviceTime();

    bool ret = devcurtime.makeTime() > devtime.makeTime();

    return ret;
}
Time OnvifClient::getLocalCurrDeviceTime()
{
    return internal->cmdmanager->getSystemTimeByDeviceTime();
}

struct AsyncHttpRequestItem : public AyncGetItem
{
    OnvifClient::HttpRequestCallback callback;
    weak_ptr<OnvifClientCmdManager> cmdmanager;

    std::string method;
    std::string url;
    std::string reqdata;
    std::string reqcontenttype;
    int timeoutms;

    void doResult()
    {
        callback(response);
        cmdsem.post();
    }

    void startRequest()
    {
        shared_ptr<OnvifClientCmdManager> _cmdmamager = cmdmanager.lock();
        if (_cmdmamager == NULL)
            return;

        URL snapurlobj(url);

        if (snapurlobj.hostname.length() <= 0)
        {
            snapurlobj.hostname = _cmdmamager->url.hostname;
            snapurlobj.port = _cmdmamager->url.port;
        }

        shared_ptr<HTTP::ClientRequest> request = make_shared<HTTP::ClientRequest>();
        request->header()->method = method;
        request->header()->url = snapurlobj.href();
        request->timeout() = timeoutms;

        if (reqdata.length() > 0)
            request->content()->write(reqdata);
        if (reqcontenttype.length() > 0)
            request->header()->headers[Content_Type] = reqcontenttype;

        AyncGetItem::start(request, _cmdmamager);
    }
};

shared_ptr<HTTP::ClientResponse> OnvifClient::httpRequest(const std::string &method, const std::string &url, const std::string &reqdata, const std::string &reqcontenttype, int timeoutms)
{
    URL urlobj(url);
    urlobj.setHost(internal->cmdmanager->url.getHost());
    urlobj.authen = internal->cmdmanager->url.authen;

    shared_ptr<AsyncHttpRequestItem> cmditem = make_shared<AsyncHttpRequestItem>();
    cmditem->cmdmanager = internal->cmdmanager;
    cmditem->method = method;
    cmditem->url = urlobj.href();
    cmditem->reqdata = reqdata;
    cmditem->reqcontenttype = reqcontenttype;
    cmditem->timeoutms = timeoutms;

    cmditem->startRequest();

    cmditem->wait(timeoutms);

    return cmditem->httpResponse();
}

ErrorInfo OnvifClient::asyncHttpRequest(const HttpRequestCallback &callback, const std::string &method, const std::string &url, const std::string &reqdata, const std::string &reqcontenttype, int timeoutms)
{
    URL urlobj(url);
    urlobj.setHost(internal->cmdmanager->url.getHost());
    urlobj.authen = internal->cmdmanager->url.authen;

    shared_ptr<AsyncHttpRequestItem> cmditem = make_shared<AsyncHttpRequestItem>();
    cmditem->callback = callback;
    cmditem->cmdmanager = internal->cmdmanager;
    cmditem->method = method;
    cmditem->url = urlobj.href();
    cmditem->reqdata = reqdata;
    cmditem->reqcontenttype = reqcontenttype;
    cmditem->timeoutms = timeoutms;

    cmditem->startRequest();

    return ErrorInfo();
}

struct SnapImageHttpRequestItem : public AsyncHttpRequestItem
{
    OnvifClient::GetSnapImageCallback snapcallback;
    Base::Semaphore snapsem;

    void httpResponseCallback(const shared_ptr<HTTP::ClientResponse> &response)
    {
        if (snapcallback)
        {
            std::string imgdata;
            int imgtype = 0;

            ErrorInfo err = checkGetSnapIamgeHTTPResponse(response, imgdata, imgtype);

            snapcallback(err, imgdata, imgtype);
        }

        snapsem.post();
    }

    ErrorInfo checkGetSnapIamgeHTTPResponse(const shared_ptr<HTTP::ClientResponse> &response, std::string &imgdata, int &imgtype)
    {
        ErrorInfo ret = AyncGetItem::checkHTTPResponse(response);
        if (ret)
            return ret;

        imgdata = httpbody;

        Value contenttype = response->header()->header(Content_Type);
        std::string conttenttypestr = contenttype.readString();

        if (String::indexOfByCase(conttenttypestr.c_str(), "jpeg") != (size_t)-1 || String::indexOfByCase(conttenttypestr.c_str(), "jpg") != (size_t)-1)
        {
            imgtype = 0;
        }
        else if (String::indexOfByCase(conttenttypestr.c_str(), "png") != (size_t)-1)
        {
            imgtype = 1;
        }
        else if (String::indexOfByCase(conttenttypestr.c_str(), "bmp") != (size_t)-1)
        {
            imgtype = 2;
        }
        else
        {
            imgtype = 0;
        }

        return ErrorInfo();
    }
};

ErrorInfo OnvifClient::getSnapImage(const OnvifClientDefs::SnapUrl &snapurl, std::string &imgdata, int &imgtype, int timeoutms)
{
    URL urlobj(snapurl.url);
    urlobj.setHost(internal->cmdmanager->url.getHost());
    urlobj.authen = internal->cmdmanager->url.authen;

    shared_ptr<SnapImageHttpRequestItem> cmditem = make_shared<SnapImageHttpRequestItem>();
    cmditem->cmdmanager = internal->cmdmanager;
    cmditem->method = "get";
    cmditem->url = urlobj.href();
    cmditem->timeoutms = timeoutms;
    cmditem->callback = HttpRequestCallback(&SnapImageHttpRequestItem::httpResponseCallback, cmditem);

    cmditem->startRequest();

    cmditem->wait(timeoutms);

    return cmditem->checkGetSnapIamgeHTTPResponse(cmditem->httpResponse(), imgdata, imgtype);
}

ErrorInfo OnvifClient::asyncGetSnapImage(const GetSnapImageCallback &callback, const OnvifClientDefs::SnapUrl &snapurl, int timeoutms)
{
    URL urlobj(snapurl.url);
    urlobj.setHost(internal->cmdmanager->url.getHost());
    urlobj.authen = internal->cmdmanager->url.authen;

    shared_ptr<SnapImageHttpRequestItem> cmditem = make_shared<SnapImageHttpRequestItem>();
    cmditem->cmdmanager = internal->cmdmanager;
    cmditem->method = "get";
    cmditem->url = urlobj.href();
    cmditem->timeoutms = timeoutms;
    cmditem->callback = HttpRequestCallback(&SnapImageHttpRequestItem::httpResponseCallback, cmditem);
    cmditem->snapcallback = callback;

    cmditem->startRequest();

    return ErrorInfo();
}

} // namespace Onvif
} // namespace Public
