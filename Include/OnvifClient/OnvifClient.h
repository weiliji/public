#ifndef __ONVIFCLIENT_H__
#define __ONVIFCLIENT_H__
#include "Base/Base.h"
#include "Network/Network.h"
#include "OnvifClientDefs.h"
using namespace Public::Base;
using namespace Public::Network;

#ifdef WIN32
#ifdef ONVIFCLIENT_EXPORTS
#define ONVIFCLIENT_API __declspec(dllexport)
#else
#define ONVIFCLIENT_API __declspec(dllimport)
#endif
#else
#define ONVIFCLIENT_API
#endif


namespace Public {
namespace Onvif {

typedef enum {
	OnvifResult_OK,					//正常
	OnvifResult_ConnectError,		//链接失败
	OnvifResult_AuthenError,		//认证失败
	OnvifResult_RequestError,		//请求失败
	OnvifResult_ResponseError,		//回复失败
	OnvifResult_ParseError,			//解析回复数据失败
	OnvifResult_NotSupport,			//不支持
}OnvifResult;


//Onvif客户端
class OnvifClientManager;
class ONVIFCLIENT_API OnvifClient
{
	friend class OnvifClientManager;
	OnvifClient(const shared_ptr<IOWorker>& worker, const URL& url,const std::string& useragent);
public:
	typedef Function<void(const std::string&)> AlarmCallback;
public:
	~OnvifClient();

	///--------------------------------设备信息相关---------------------------------//
	//获取设备信息
	OnvifResult getInfo(OnvifClientDefs::Info& info,int timeoutms = 10000);	

	//异步获取设备信息
	typedef Function<void(OnvifResult,const OnvifClientDefs::Info&)> GetInfoCalblack;
	bool asyncGetInfo(const GetInfoCalblack& callback, int timeoutms = 10000);

	//获取系统时间
	OnvifResult getSystemDatetime(Time& time, int timeoutms = 10000);

	//异步获取系统时间
	typedef Function<void(OnvifResult, const Time&) > GetSystemDateTimeCallback;
	bool asyncGetSystemDatetime(const GetSystemDateTimeCallback& callbck, int timeoutms = 10000);

	//设置系统时间
	OnvifResult SetSystemDatetime(const Time& time, int timeoutms = 10000);

	//异步设置系统时间
	typedef Function<void(OnvifResult)> SetSystemDatetimeCallback;
	bool asyncSetSystemDatetime(const SetSystemDatetimeCallback& callback, const Time& time, int timeoutms = 10000);

	//获取网络信息
	OnvifResult getNetworkInterfaces(OnvifClientDefs::NetworkInterfaces& network, int timeoutms = 10000);
	//异步获取网络信息
	typedef Function<void(OnvifResult, const OnvifClientDefs::NetworkInterfaces&)> GetNetworkInterfacesCallback;
	bool asyncGetNetworkInterfaces(const GetNetworkInterfacesCallback& callback, int timeoutms = 10000);

	//系统重启
	OnvifResult systemReboot(int timeoutms = 10000);

	//异步系统重启
	typedef Function<void(OnvifResult)> SystemRebootCallback;
	bool asyncSystemReboot(const SystemRebootCallback& callback, int timeoutms = 10000);


	//获取设备能力集信息
	OnvifResult getCapabities(OnvifClientDefs::Capabilities& cap,int timeoutms = 10000);

	//异步获取设备能力信息
	typedef Function<void(OnvifResult, const OnvifClientDefs::Capabilities&)> GetCapabilitiesCallback;
	bool asyncGetCapabities(const GetCapabilitiesCallback& callback, int timeoutms = 10000);
								
	
	//获取音视频/云台等描述信息
	OnvifResult getProfiles(OnvifClientDefs::Profiles& profiles,int timeoutms = 10000);
	
	//异步获取描述信息
	typedef Function<void(OnvifResult,const OnvifClientDefs::Profiles&)> GetProfilesCallback;
	bool asyncGetProfiles(const GetProfilesCallback& callback, int timeoutms = 10000);


	///--------------------------------Media相关---------------------------------//
	//获取视频播放地址
	OnvifResult getStreamUrl(const OnvifClientDefs::ProfileInfo& info, OnvifClientDefs::StreamUrl& streamurl,int timeoutms = 10000);
	
	//异步获取视频流地址
	typedef Function<void(OnvifResult, const OnvifClientDefs::StreamUrl& )> GetStreamUrlCallback;
	bool asyncGetStreamUrl(const GetStreamUrlCallback& callback, const OnvifClientDefs::ProfileInfo& info, int timeoutms = 10000);

	//获取非预览截图地址
	OnvifResult getSnapUrl(const OnvifClientDefs::ProfileInfo& info, OnvifClientDefs::SnapUrl& snapurl,int timeoutms = 10000);
	
	//异步获取非预览截图地址
	typedef Function<void(OnvifResult, const OnvifClientDefs::SnapUrl&)> GetSnapUrlCallback;
	bool asyncGetSnapUrl(const GetSnapUrlCallback& callback, const OnvifClientDefs::ProfileInfo& info, int timeoutms = 10000);

	
	//获取视频编码配置信息
	OnvifResult getVideoEncoderConfigurations(OnvifClientDefs::VideoEncoderConfigurations& configs,int timeoutms = 10000);
	
	typedef Function<void(OnvifResult, const OnvifClientDefs::VideoEncoderConfigurations&) > GetVideoEncoderConfigurationsCallback;
	bool asyncGetVideoEncoderConfigurations(const GetVideoEncoderConfigurationsCallback& callback, int timeoutms = 10000);


	///----------------------------------------云台相关--------------------------//
	//云台相关
	OnvifResult continuousMove(const OnvifClientDefs::ProfileInfo& info, const OnvifClientDefs::PTZCtrl& ptzctrl, int timeoutms = 10000);
	
	//异步云台控制
	typedef Function<void(OnvifResult)> ContinuousMoveCallback;
	bool asyncContinuousMoveCallback(const ContinuousMoveCallback& callback, const OnvifClientDefs::ProfileInfo& info, const OnvifClientDefs::PTZCtrl& ptzctrl, int timeoutms = 10000);
	
	//停止云台控制
	OnvifResult stopPTZ(const OnvifClientDefs::ProfileInfo& info, const OnvifClientDefs::PTZCtrl& ptzctrl, int timeoutms = 10000);
	
	//异步停止云台控制
	typedef Function<void(OnvifResult)> StopPTRCallback;
	bool asyncStopPtz(const StopPTRCallback& callback, const OnvifClientDefs::ProfileInfo& info, const OnvifClientDefs::PTZCtrl& ptzctrl, int timeoutms = 10000);

	//预置位相关
	//设置云台预置位
	OnvifResult setPreset(const OnvifClientDefs::ProfileInfo& info, const std::string& presetname, int timeoutms = 10000);
	
	//异步设置云台预置位
	typedef Function<void(OnvifResult)> SetPresetCallback;
	bool asyncSetPresest(const SetPresetCallback& callback, const OnvifClientDefs::ProfileInfo& info, const std::string& presetname, int timeoutms = 10000);

	//获取云台预置位
	OnvifResult getPreset(const OnvifClientDefs::ProfileInfo& info, OnvifClientDefs::PresetInfos& presetinfo,int timeoutms = 10000);
	
	//异步获取云台预置位
	typedef Function<void(OnvifResult,const OnvifClientDefs::PresetInfos&)> GetPresetCallback;
	bool asyncGetPreset(const GetPresetCallback& callback, const OnvifClientDefs::ProfileInfo& info, int timeoutms = 10000);

	//执行预置位
	OnvifResult gotoPreset(const OnvifClientDefs::ProfileInfo& info, const OnvifClientDefs::PresetInfo& presetinfo, int timeoutms = 10000);
	
	//异步执行预置位
	typedef Function<void(OnvifResult)> GotoPresetCallback;
	bool asyncGotoPreset(const GotoPresetCallback& callback, const OnvifClientDefs::ProfileInfo& info, const OnvifClientDefs::PresetInfo& presetinfo, int timeoutms = 10000);
	

	//删除预置位
	OnvifResult removePreset(const OnvifClientDefs::ProfileInfo& info, const OnvifClientDefs::PresetInfo& presetinfo, int timeoutms = 10000);

	//异步删除云台预置位
	typedef Function<void(OnvifResult)> RemovePresetCallback;
	bool asyncRemovePreset(const RemovePresetCallback& callback, const OnvifClientDefs::ProfileInfo& info, const OnvifClientDefs::PresetInfo& presetinfo, int timeoutms = 10000);

	//---------------------------------------------EVENT相关----------------------------------------//

	//报警事件相关
	OnvifResult subscribeEvent(const OnvifClientDefs::Capabilities& capabilities, OnvifClientDefs::SubEventResponse& subeventresp,int timeoutms = 10000);
	
	typedef Function<void(OnvifResult, const OnvifClientDefs::SubEventResponse&) > SubscribeEventCallback;
	bool asyncSubscribeEvent(const SubscribeEventCallback& callback, const OnvifClientDefs::Capabilities& capabilities, int timeoutms = 10000);

	OnvifResult getEvent(const OnvifClientDefs::SubEventResponse& subeventresp,OnvifClientDefs::EventInfos& eventinfos,int timeoutms = 2*60000);

	typedef Function<void(OnvifResult,const OnvifClientDefs::EventInfos&)> GetEventCallback;
	bool asyncGetEvent(const GetEventCallback& callback, const OnvifClientDefs::SubEventResponse& subeventresp, int timeoutms = 2 * 60000);

	bool stopSubEvent();
public:
	struct OnvifClientInternal;
private:
	OnvifClientInternal* internal;
};

//OnvifClient管理器
class ONVIFCLIENT_API OnvifClientManager
{
	friend OnvifClient;
public:
	//当返回一个OnvifClientDefs::DiscoveryInfo空的包，表示搜索结束
	typedef Function<void, const shared_ptr<OnvifClientDefs::DiscoveryInfo>&> DisconveryCallback;
public:
	//userContent 用户描述信息,threadNum 线程数，根据RTSP的用户量决定
	OnvifClientManager(const shared_ptr<IOWorker>& worker,const std::string& userContent);
	~OnvifClientManager();

	//根据onvif设备地址创建一个设备,url 为设备地址，包括IP，端口，用户名，密码等信息,onvif默认请求路径为 "/onvif/device_service"
	//如:admin:admin@192.168.13.33
	shared_ptr<OnvifClient> create(const URL& url);

	//搜索设备，超时后自动停止搜索
	bool disconvery(const DisconveryCallback& callback, uint32_t timeout = 10000);
private:
	struct OnvifClientManagerInternal;
	OnvifClientManagerInternal* internal;
};


}
}



#endif //__ONVIFCLIENT_H__
