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
	OnvifClient(const shared_ptr<IOWorker>& worker, const URL& url,const std::string& useragent);
public:
	~OnvifClient();

	shared_ptr<OnvifClientDefs::Info> getInfo(int timeoutms = 10000);			//获取设备信息，错误信息使用XM_GetLastError捕获
	shared_ptr<OnvifClientDefs::Capabilities> getCapabities(int timeoutms = 10000);	//获取设备能力集合，错误信息使用XM_GetLastError捕获
//	shared_ptr<OnvifClientDefs::Scopes> getScopes(int timeoutms = 10000); //获取描述信息，错误信息使用XM_GetLastError捕获
	
	//获取音视频/云台等熟悉
	shared_ptr<OnvifClientDefs::Profiles> getProfiles(int timeoutms = 10000); //获取配置信息，错误信息使用XM_GetLastError捕获
	
	//获取视频播放地址
	shared_ptr<OnvifClientDefs::StreamUrl> getStreamUrl(const OnvifClientDefs::ProfileInfo& info, int timeoutms = 10000); //获取六信息,错误信息使用XM_GetLastError捕获
	
	//获取非预览截图地址
	shared_ptr<OnvifClientDefs::SnapUrl> getSnapUrl(const OnvifClientDefs::ProfileInfo& info, int timeoutms = 10000);	//获取截图信息，错误信息使用XM_GetLastError捕获
	
	//获取网络信息
	shared_ptr<OnvifClientDefs::NetworkInterfaces> getNetworkInterfaces(int timeoutms = 10000);//网络信息，错误信息使用XM_GetLastError捕获
//	shared_ptr<OnvifClientDefs::VideoEncoderConfigurations> getVideoEncoderConfigurations(int timeoutms = 10000); //获取视频编码信息，错误信息使用XM_GetLastError捕获
	
	//云台相关
	bool continuousMove(const OnvifClientDefs::ProfileInfo& info, const OnvifClientDefs::PTZCtrl& ptzctrl, int timeoutms = 10000); //错误信息使用XM_GetLastError捕获
	bool absoluteMove(const OnvifClientDefs::ProfileInfo& info, const OnvifClientDefs::PTZCtrl& ptzctrl, int timeoutms = 10000); //错误信息使用XM_GetLastError捕获
	bool stopPTZ(const OnvifClientDefs::ProfileInfo& info, const OnvifClientDefs::PTZCtrl& ptzctrl, int timeoutms = 10000); //停止云台
	
	//预置位相关
	bool setPreset(const OnvifClientDefs::ProfileInfo& info, const std::string& presetname, int timeoutms = 10000);//设置预置位
	shared_ptr<OnvifClientDefs::PresetInfos> getPreset(const OnvifClientDefs::ProfileInfo& info, int timeoutms = 10000);//获取预置位信息
	bool gotoPreset(const OnvifClientDefs::ProfileInfo& info, const OnvifClientDefs::PresetInfo& presetinfo, int timeoutms = 10000);//调用预置位
	bool removePreset(const OnvifClientDefs::ProfileInfo& info, const OnvifClientDefs::PresetInfo& presetinfo, int timeoutms = 10000);//删除预置位

	//获取云台配置信息
	shared_ptr<OnvifClientDefs::PTZConfig> getConfigurations(int timeoutms = 10000); //错误信息使用XM_GetLastError捕获
	shared_ptr<OnvifClientDefs::ConfigurationOptions> getConfigurationOptions(const shared_ptr<OnvifClientDefs::PTZConfig>& ptzcfg,int timeoutms = 10000); //错误信息使用XM_GetLastError捕获
	
	//获取系统时间
	shared_ptr<Time> GetSystemDatetime(int timeoutms = 10000); //错误信息使用XM_GetLastError捕获
	bool SetSystemDatetime(const Time& time, int timeoutms = 10000); //错误信息使用XM_GetLastError捕获
	bool SystemReboot(int timeoutms = 10000);//错误信息使用XM_GetLastError捕获

	//报警事件相关
	shared_ptr<OnvifClientDefs::StartRecvAlarm> startRecvAlarm(const shared_ptr<OnvifClientDefs::Capabilities>& capabilities,int timeoutms = 10000);
	bool recvAlarm(const shared_ptr<OnvifClientDefs::StartRecvAlarm>& alarminfo,int timeoutms = 2*60000);
	bool stopRecvAlarm();
private:
	struct OnvifClientInternal;
	OnvifClientInternal* internal;
};

//OnvifClient管理器
class ONVIFCLIENT_API OnvifClientManager
{
	friend OnvifClient;
public:
	//当返回一个OnvifClientDefs::DiscoveryInfo空的包，表示搜索结束
	typedef Function1<void, const shared_ptr<OnvifClientDefs::DiscoveryInfo>&> DisconveryCallback;
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
