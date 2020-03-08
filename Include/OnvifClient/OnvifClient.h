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

//Onvif客户端
class OnvifClientManager;
class ONVIFCLIENT_API OnvifClient
{
	friend class OnvifClientManager;
	OnvifClient(const shared_ptr<IOWorker>& worker,const shared_ptr<HTTP::AsyncClient>& asyncclient, const URL& url,const std::string& useragent);
	void onPoolTimerProc();
public:
	typedef Function<void(const std::string&)> AlarmCallback;
public:
	~OnvifClient();

	const URL& OnvifUrl() const;

	///--------------------------------设备信息相关---------------------------------//
	//获取设备信息
	ErrorInfo getInfo(OnvifClientDefs::Info& info,int timeoutms = 10000);

	//异步获取设备信息
	typedef Function<void(const ErrorInfo&,const OnvifClientDefs::Info&)> GetInfoCalblack;
	ErrorInfo asyncGetInfo(const GetInfoCalblack& callback, int timeoutms = 10000);

	//获取系统时间
	ErrorInfo getSystemDatetime(Time& time, int timeoutms = 10000);

	//异步获取系统时间
	typedef Function<void(const ErrorInfo&, const Time&) > GetSystemDateTimeCallback;
	ErrorInfo asyncGetSystemDatetime(const GetSystemDateTimeCallback& callbck, int timeoutms = 10000);

	//设置系统时间
	ErrorInfo SetSystemDatetime(const Time& time, int timeoutms = 10000);

	//异步设置系统时间
	typedef Function<void(const ErrorInfo&)> SetSystemDatetimeCallback;
	ErrorInfo asyncSetSystemDatetime(const SetSystemDatetimeCallback& callback, const Time& time, int timeoutms = 10000);

	//获取网络信息
	ErrorInfo getNetworkInterfaces(OnvifClientDefs::NetworkInterfaces& network, int timeoutms = 10000);
	//异步获取网络信息
	typedef Function<void(const ErrorInfo&, const OnvifClientDefs::NetworkInterfaces&)> GetNetworkInterfacesCallback;
	ErrorInfo asyncGetNetworkInterfaces(const GetNetworkInterfacesCallback& callback, int timeoutms = 10000);

	//系统重启
	ErrorInfo systemReboot(int timeoutms = 10000);

	//异步系统重启
	typedef Function<void(const ErrorInfo&)> SystemRebootCallback;
	ErrorInfo asyncSystemReboot(const SystemRebootCallback& callback, int timeoutms = 10000);


	//获取设备能力集信息
	ErrorInfo getCapabities(OnvifClientDefs::Capabilities& cap,int timeoutms = 10000);

	//异步获取设备能力信息
	typedef Function<void(const ErrorInfo&, const OnvifClientDefs::Capabilities&)> GetCapabilitiesCallback;
	ErrorInfo asyncGetCapabities(const GetCapabilitiesCallback& callback, int timeoutms = 10000);
								
	
	//获取音视频/云台等描述信息
	ErrorInfo getProfiles(OnvifClientDefs::Profiles& profiles,int timeoutms = 10000);
	
	//异步获取描述信息
	typedef Function<void(const ErrorInfo&,const OnvifClientDefs::Profiles&)> GetProfilesCallback;
	ErrorInfo asyncGetProfiles(const GetProfilesCallback& callback, int timeoutms = 10000);


	///--------------------------------Media相关---------------------------------//
	//获取视频播放地址
	ErrorInfo getStreamUrl(const OnvifClientDefs::ProfileInfo& info, OnvifClientDefs::StreamUrl& streamurl,int timeoutms = 10000);
	
	//异步获取视频流地址
	typedef Function<void(const ErrorInfo&, const OnvifClientDefs::StreamUrl& )> GetStreamUrlCallback;
	ErrorInfo asyncGetStreamUrl(const GetStreamUrlCallback& callback, const OnvifClientDefs::ProfileInfo& info, int timeoutms = 10000);

	//获取非预览截图地址
	ErrorInfo getSnapUrl(const OnvifClientDefs::ProfileInfo& info, OnvifClientDefs::SnapUrl& snapurl,int timeoutms = 10000);

	//异步获取非预览截图地址
	typedef Function<void(const ErrorInfo&, const OnvifClientDefs::SnapUrl&)> GetSnapUrlCallback;
	ErrorInfo asyncGetSnapUrl(const GetSnapUrlCallback& callback, const OnvifClientDefs::ProfileInfo& info, int timeoutms = 10000);


	//-----------音视频参数设置
	//获取视频编码配置信息
	ErrorInfo getVideoEncoderConfigurations(OnvifClientDefs::VideoEncoderConfigurations& configs,int timeoutms = 10000);
	
	typedef Function<void(const ErrorInfo&, const OnvifClientDefs::VideoEncoderConfigurations&) > GetVideoEncoderConfigurationsCallback;
	ErrorInfo asyncGetVideoEncoderConfigurations(const GetVideoEncoderConfigurationsCallback& callback, int timeoutms = 10000);

    //图像参数相关
    typedef Function<void(const ErrorInfo&, const OnvifClientDefs::ImageSettingInfo&) > GetImageSettingCallback;
    ErrorInfo asyncGetImageSetting(const OnvifClientDefs::ProfileInfo& info, const GetImageSettingCallback& callback, int timeoutms = 10000);

    typedef Function<void(const ErrorInfo&)> SetImageSettingCallback;
    ErrorInfo asyncSetImageSetting(const OnvifClientDefs::ProfileInfo& info, const OnvifClientDefs::ImageSettingInfo&, const SetImageSettingCallback& callback, int timeoutms = 10000);

	typedef Function<void(const ErrorInfo&, const OnvifClientDefs::ImageOptions)> GetImageOptionsCallback;
	ErrorInfo asyncGetImageOptions(const OnvifClientDefs::ProfileInfo& info, const GetImageOptionsCallback& callback, int timeoutms = 10000);

	///----------------------------------------云台相关--------------------------//
	//云台相关
	ErrorInfo continuousMove(const OnvifClientDefs::ProfileInfo& info, const OnvifClientDefs::PTZCtrl& ptzctrl, int timeoutms = 10000);
	
	//异步云台控制
	typedef Function<void(const ErrorInfo&)> ContinuousMoveCallback;
	ErrorInfo asyncContinuousMove(const ContinuousMoveCallback& callback, const OnvifClientDefs::ProfileInfo& info, const OnvifClientDefs::PTZCtrl& ptzctrl, int timeoutms = 10000);
	
	//停止云台控制
	ErrorInfo stopPTZ(const OnvifClientDefs::ProfileInfo& info, const OnvifClientDefs::PTZCtrl& ptzctrl, int timeoutms = 10000);
	
	//异步停止云台控制
	typedef Function<void(const ErrorInfo&)> StopPTRCallback;
	ErrorInfo asyncStopPtz(const StopPTRCallback& callback, const OnvifClientDefs::ProfileInfo& info, const OnvifClientDefs::PTZCtrl& ptzctrl, int timeoutms = 10000);

	//预置位相关
	//设置云台预置位
	ErrorInfo setPreset(uint32_t index, const std::string& presetname, const OnvifClientDefs::ProfileInfo& info, int timeoutms = 10000);
	
	//异步设置云台预置位
	typedef Function<void(const ErrorInfo&)> SetPresetCallback;
	ErrorInfo asyncSetPresest(uint32_t index, const std::string& presetname, const OnvifClientDefs::ProfileInfo& info, const SetPresetCallback& callback, int timeoutms = 10000);

	//获取云台预置位
	ErrorInfo getPreset(const OnvifClientDefs::ProfileInfo& info, OnvifClientDefs::PresetInfos& presetinfo, int timeoutms = 10000);
	
	//异步获取云台预置位
	typedef Function<void(const ErrorInfo&,const OnvifClientDefs::PresetInfos&)> GetPresetCallback;
	ErrorInfo asyncGetPreset(const OnvifClientDefs::ProfileInfo& info, const GetPresetCallback& callback, int timeoutms = 10000);

	//执行预置位
	ErrorInfo gotoPreset(uint32_t index, const OnvifClientDefs::ProfileInfo& info, int timeoutms = 10000);
	
	//异步执行预置位
	typedef Function<void(const ErrorInfo&)> GotoPresetCallback;
	ErrorInfo asyncGotoPreset(uint32_t index, const OnvifClientDefs::ProfileInfo&  presetinfo, const GotoPresetCallback& callback, int timeoutms = 10000);
	

	//删除预置位
	ErrorInfo removePreset(uint32_t index, const OnvifClientDefs::ProfileInfo& info, int timeoutms = 10000);

	//异步删除云台预置位
	typedef Function<void(const ErrorInfo&)> RemovePresetCallback;
    ErrorInfo asyncRemovePreset(uint32_t index, const OnvifClientDefs::ProfileInfo& info, const RemovePresetCallback& callback, int timeoutms = 10000);

	//---------------------------------------------EVENT相关----------------------------------------//

	//报警事件相关
	ErrorInfo subscribeEvent(const OnvifClientDefs::Capabilities& capabilities, OnvifClientDefs::SubEventResponse& subeventresp,int timeoutms = 10000);
	
	typedef Function<void(const ErrorInfo&, const OnvifClientDefs::SubEventResponse&) > SubscribeEventCallback;
	ErrorInfo asyncSubscribeEvent(const SubscribeEventCallback& callback, const OnvifClientDefs::Capabilities& capabilities, int timeoutms = 10000);

	ErrorInfo getEvent(const OnvifClientDefs::SubEventResponse& subeventresp,OnvifClientDefs::EventInfos& eventinfos,int timeoutms = 2*60000);

	typedef Function<void(const ErrorInfo&,const OnvifClientDefs::EventInfos&)> GetEventCallback;
	ErrorInfo asyncGetEvent(const GetEventCallback& callback, const OnvifClientDefs::SubEventResponse& subeventresp, int timeoutms = 2 * 60000);

	ErrorInfo stopSubEvent();

	//---------------------------------------------HTTP相关------------------------------//
	//imgtype == 0 ? jpg, == 1 ? png, == 2 ? bmp
	ErrorInfo getSnapImage(const OnvifClientDefs::SnapUrl& snapurl, std::string& imgdata, int& imgtype, int timeoutms = 10000);

	typedef Function<void(const ErrorInfo&, const std::string& imgdata, int imgtype)> GetSnapImageCallback;
	ErrorInfo asyncGetSnapImage(const GetSnapImageCallback& callback, const OnvifClientDefs::SnapUrl& snapurl, int timeoutms = 10000);



	//同步进行HTTP请求
	//method 请求方式，get/post
	//url 请求地址
	//ackdata 回复数据
	//reqdata 请求附带的数据
	//reqcontenttype，请求数据类型
	//timeoutms 超时时间
	shared_ptr<HTTP::ClientResponse> httpRequest(const std::string& method,const std::string& url,const std::string& reqdata = "",const std::string& reqcontenttype = "", int timeoutms = 10000);

	typedef Function<void(const shared_ptr<HTTP::ClientResponse>&)> HttpRequestCallback;
	ErrorInfo asyncHttpRequest(const HttpRequestCallback& callback,const std::string& method, const std::string& url, const std::string& reqdata = "", const std::string& reqcontenttype = "", int timeoutms = 10000);

	//检测设备时间和当前对比是否过期
	bool checkDeviceTimeIsOverdue(const Time& devtime);
	Time getLocalCurrDeviceTime();
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
	struct ONVIFCLIENT_API Disconvery
	{
		friend OnvifClientManager;
	public:
		Disconvery();
		~Disconvery();

		void sendDisconvery();

		void getDeviceList(std::list<OnvifClientDefs::DiscoveryInfo>& list);
	private:
		struct DisconveryInternal;
		DisconveryInternal* internal;
	};
public:
	//userContent 用户描述信息,threadNum 线程数，根据RTSP的用户量决定
	OnvifClientManager(const shared_ptr<IOWorker>& worker,const std::string& userContent);
	~OnvifClientManager();

	//根据onvif设备地址创建一个设备,url 为设备地址，包括IP，端口，用户名，密码等信息,onvif默认请求路径为 "/onvif/device_service"
	//如:admin:admin@192.168.13.33
	shared_ptr<OnvifClient> create(const URL& url);

	//搜索设备，超时后自动停止搜索
	shared_ptr<Disconvery> disconvery();
private:
	struct OnvifClientManagerInternal;
	OnvifClientManagerInternal* internal;
};


}
}



#endif //__ONVIFCLIENT_H__
