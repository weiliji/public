#ifndef __ONVIFCLIENT_DEFS_H__
#define __ONVIFCLIENT_DEFS_H__
#include "Base/Base.h"
#include "Network/Network.h"
using namespace Public::Base;
using namespace Public::Network;

namespace Public {
namespace Onvif {
namespace OnvifClientDefs {


//设备信息
struct Info
{
	std::string manufacturer;		//工厂信息
	std::string model;				//设备信息
	std::string firmwareVersion;	//固件版本
	std::string serialNumber;		//序列号
	std::string hardwareId;			//硬件标识
//	std::string Name;				//设备名称
};

//设备能力集
struct Capabilities
{
	struct _Medis{
		URL		xaddr;
		bool	rtpMulticast = false;
		bool	rtp_tcp = false;
		bool	rtp_rtsp_tcp = false;
		bool	support = false;
	}media;
	struct _PTZ{
		URL		xaddr;
		bool    support = false;
	}ptz;
	struct _Events {
		URL		xaddr;
		bool    support = false;
	}events;

	struct _Device {
		struct _IO {
			std::vector<int>	alarminput;
			std::vector<int>	alarmoutput;
		}io;
	}device;
} ;

//暂时不解析GetScopes，太复杂，目前没需求
typedef struct {

}Scopes;

struct _VideoSource {
	int  width = 0;
	int	 height = 0;
	int  x = 0;
	int  y = 0;;

	int  use_count = 0;
	std::string token;
	std::string stream_name;
	std::string source_token;
};

typedef enum {
	H264_PROFILE_Baseline = 0,
	H264_PROFILE_Main = 1,
	H264_PROFILE_Extended = 2,
	H264_PROFILE_High = 3,
}H264_PROFILE;

struct _VideoEncoder
{
	float quality = 0;
	int  width = 0;
	int  height = 0;
	int  use_count = 0;
	int  session_timeout = 0;

	std::string name;
	std::string token;

	CodeID  encoding = CodeID_Unknown;

	int  framerate_limit = 0;
	int  encoding_interval = 0;
	int  bitrate_limit = 0;

	/* H264Configuration */
	int  gov_len = 0;
	H264_PROFILE  h264_profile;
} ;

struct _AudioSource
{
	int  use_count = 0;
	std::string token;
	std::string stream_name;
	std::string source_token;
};

struct _AudioEncoder
{
	int  use_count = 0;
	int  session_timeout = 0;

	std::string name;
	std::string token;

	CodeID  encoding = CodeID_Unknown;

	int  bitrate = 0;
	int  sample_rate = 0;
};

struct _Range
{
	float min = 0;
	float max = 0;
};

struct PTZConfig
{
	int  use_count = 0;
	int  def_timeout = 0;

	std::string name;
	std::string token;
	std::string nodeToken;

	struct _Speed
	{
		int pan_tilt_x = 0;
		int pan_tilt_y = 0;
		int zoom = 0;
	} def_speed;

	_Range pantilt_x;
	_Range pantilt_y;
	_Range zoom;
} ;

struct ConfigurationOptions
{
	int used = 0;

	_Range absolute_pantilt_x;
	_Range absolute_pantilt_y;
	_Range absolute_zoom;

	_Range relative_pantilt_x;
	_Range relative_pantilt_y;
	_Range relative_zoom;

	_Range continuous_pantilt_x;
	_Range continuous_pantilt_y;
	_Range continuous_zoom;

	_Range pantilt_speed;
	_Range zoom_speed;
	_Range timeout;
};

//配置信息
struct ProfileInfo {
	shared_ptr<_VideoSource>	videoSource;
	shared_ptr<_VideoEncoder>	videoEncoder;
	shared_ptr<_AudioSource>	audioSource;
	shared_ptr<_AudioEncoder>	audioEncoder;
	shared_ptr<PTZConfig>		ptzConfig;


	std::string name;
	std::string token;
	bool fixed = false;
};

struct Profiles {
	std::vector<ProfileInfo> infos;
};

struct NetworkInterfaces {
	std::string		name;
	std::string		macaddr;
	std::string		ipaddr;
	bool			dhcp = false;
};
typedef struct {

}VideoEncoderConfigurations;

struct PTZCtrl
{
	enum {
		PTZ_CTRL_PAN  =  0,
		PTZ_CTRL_ZOOM =  1,
	}ctrlType = PTZ_CTRL_PAN;
	double           panTiltX = 0;
	double           panTiltY = 0;
	float           zoom = 0;
	int				duration = 0;
} ;


struct StreamUrl
{
	std::string url;
};

struct SnapUrl
{
	std::string url;
};

struct SubEventResponse
{
	URL				xaddr;
	Time			currentTime;
	Time			terminationTime;
};

struct EventInfos
{
	struct EventInfo
	{
		Time			arrivalTime;
		std::string		topic;
		std::string		operation;
		std::map<std::string, std::string> sources;
		std::map<std::string, std::string> datas;
	};

	std::vector<EventInfo> eventInfos;
};

struct DiscoveryInfo
{
	std::string				 name;
	std::string				 mac;
	std::string				 model;
	std::string				 addr;
	uint32_t				 port = 0;
	std::string				 manufacturer;		//工厂信息
};

struct PresetInfo
{
	uint32_t token;
	std::string name;
};

struct PresetInfos
{
	std::vector<PresetInfo> infos;
};

struct ImageSettingInfo
{
    uint32_t brightness = 0;
    uint32_t colorSaturation = 0;
    uint32_t contrast = 0;
    uint32_t sharpness = 0;
    struct Exposure
    {
        std::string mode;
        uint32_t minIris = 0;
        uint32_t maxIris = 0;
        uint32_t iris = 0;
    } exposure;

    struct Focus
    {
        std::string mode;
        float defaultSpeed = 0;
    } focus;
};

struct ImageOptions
{
	struct Brightness
	{
		uint32_t min;
		uint32_t max;
	}brightness;

	struct ColorSaturation
	{
		uint32_t min;
		uint32_t max;
	}colorSaturation;

	struct Contrast
	{
		uint32_t min;
		uint32_t max;
	}contrast;

	struct Sharpness
	{
		uint32_t min;
		uint32_t max;
	}sharpness;
};


}
}
}



#endif //__ONVIFCLIENT_H__
