#ifndef __ONVIFPROTOCOLOBJECT_H__
#define __ONVIFPROTOCOLOBJECT_H__

#include "OnvifClient/OnvifClientDefs.h"
#include "GSopProtocol.h"

using namespace Public::Onvif;

#define URL_ONVIF_DEVICE_SERVICE	"/onvif/device_service"
#define URL_ONVIF_MEDIA				"/onvif/media"
#define URL_ONVIF_PTZ				"/onvif/PTZ"
#define URL_ONVIF_EVENTS			"/onvif/Events"
#define URL_ONVIF_IMAGING			"/onvif/Imaging"


#define MAXOVIFHEADERLEN	2048

class CmdObject:public GSop_Envelop
{
public:
	std::string				requesturl;
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
	virtual bool parse(const XML::Child& body) { return false; }
protected:
	std::string parseMacAddr(const std::string& macstr)
	{
        std::string macstrtmp;
        if (macstr.length() == 12)
        {
            for (int i = 0; i < 6; i++)
            {
                if (i != 0)
                {
                    macstrtmp += ":";
                }
                macstrtmp += String::toupper(std::string(macstr.c_str() + i * 2, 2));
            }
        }
		else if (macstr.length() == 17)
		{
			return String::toupper(macstr);
		}
        else
        {
            return "";
        }
		

		return macstrtmp;
	}
public:
	static Time onvif_parse_datetime(const std::string& datastr)
	{
		Time	nowtime;
		sscanf(datastr.c_str(), "%04d-%02d-%02dT%02d:%02d:%02dZ", &nowtime.year, &nowtime.month, &nowtime.day, &nowtime.hour, &nowtime.minute, &nowtime.second);

		return nowtime;
	}

	static CodeID onvif_parse_encoding(const std::string& pdata)
	{
		if (String::strcasecmp(pdata.c_str(), "H264") == 0)
		{
			return CodeID_Video_H264;
		}
		else if (String::strcasecmp(pdata.c_str(), "H265") == 0)
		{
			return CodeID_Video_H265;
		}
		else if (String::strcasecmp(pdata.c_str(), "JPEG") == 0)
		{
			return CodeID_Video_JPEG;
		}
		else if (String::strcasecmp(pdata.c_str(), "G711") == 0)
		{
			return CodeID_Audio_G711Mu;
		}
		else if (String::strcasecmp(pdata.c_str(), "AAC") == 0)
		{
			return CodeID_Audio_AAC;
		}
		return CodeID_Unknown;
	}

	OnvifClientDefs::H264_PROFILE onvif_parse_h264_profile(const std::string& pdata)
	{
		if (String::strcasecmp(pdata.c_str(), "Baseline") == 0)
		{
			return OnvifClientDefs::H264_PROFILE_Baseline;
		}
		else if (String::strcasecmp(pdata.c_str(), "High") == 0)
		{
			return OnvifClientDefs::H264_PROFILE_High;
		}
		else if (String::strcasecmp(pdata.c_str(), "Main") == 0)
		{
			return OnvifClientDefs::H264_PROFILE_Main;
		}
		else if (String::strcasecmp(pdata.c_str(), "Extended") == 0)
		{
			return OnvifClientDefs::H264_PROFILE_Extended;
		}
		return OnvifClientDefs::H264_PROFILE_Baseline;
	}
};


#endif //__ONVIFPROTOCOL_H__
