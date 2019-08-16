#ifndef __ONVIFPROTOCOLOBJECT_H__
#define __ONVIFPROTOCOLOBJECT_H__

#include "OnvifClient/OnvifClientDefs.h"
#include "GSopProtocol.h"

using namespace Public::Onvif;

#define URL_ONVIF_DEVICE_SERVICE	"/onvif/device_service"
#define URL_ONVIF_MEDIA				"/onvif/media"
#define URL_ONVIF_PTZ				"/onvif/PTZ"
#define URL_ONVIF_EVENTS			"/onvif/Events"

#define MAXOVIFHEADERLEN	2048

class CmdObject:public GSop_Envelop
{
public:
	std::string requesturl;
public:
	CmdObject(const std::string& requrl){ requesturl = requrl; }
	virtual ~CmdObject() {}

	virtual std::string build(const URL& URL)
	{
		header().security.username = URL.authen.Username;
		header().security.password = URL.authen.Password;

		return buildGSopProtocol();
	}
	bool parseProtocol()
	{
		return parse(body());
	}
private:
	virtual bool parse(const XMLObject::Child& body) { return false; }
protected:
public:
	static Time onvif_parse_datetime(const std::string& datastr)
	{
		Time	nowtime;
		sscanf_s(datastr.c_str(), "%04d-%02d-%02dT%02d:%02d:%02dZ", &nowtime.year, &nowtime.month, &nowtime.day, &nowtime.hour, &nowtime.minute, &nowtime.second);

		return nowtime;
	}

	static OnvifClientDefs::VIDEO_ENCODING onvif_parse_encoding(const std::string& pdata)
	{
		if (strcasecmp(pdata.c_str(), "H264") == 0)
		{
			return OnvifClientDefs::VIDEO_ENCODING_H264;
		}
		else if (strcasecmp(pdata.c_str(), "JPEG") == 0)
		{
			return OnvifClientDefs::VIDEO_ENCODING_JPEG;
		}
		else if (strcasecmp(pdata.c_str(), "MPEG4") == 0)
		{
			return OnvifClientDefs::VIDEO_ENCODING_MPEG4;
		}
		return OnvifClientDefs::VIDEO_ENCODING_UNKNOWN;
	}

	OnvifClientDefs::H264_PROFILE onvif_parse_h264_profile(const std::string& pdata)
	{
		if (strcasecmp(pdata.c_str(), "Baseline") == 0)
		{
			return OnvifClientDefs::H264_PROFILE_Baseline;
		}
		else if (strcasecmp(pdata.c_str(), "High") == 0)
		{
			return OnvifClientDefs::H264_PROFILE_High;
		}
		else if (strcasecmp(pdata.c_str(), "Main") == 0)
		{
			return OnvifClientDefs::H264_PROFILE_Main;
		}
		else if (strcasecmp(pdata.c_str(), "Extended") == 0)
		{
			return OnvifClientDefs::H264_PROFILE_Extended;
		}
		return OnvifClientDefs::H264_PROFILE_Baseline;
	}
};


#endif //__ONVIFPROTOCOL_H__
