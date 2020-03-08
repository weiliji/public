#pragma once 
#include "Defs.h"
#include "Base/Base.h"
#include "MSDefine.h"
using namespace Public::Base;
using namespace Milesight::Protocol;

namespace Milesight {
namespace MQ {

enum StreamUrlType
{
	StreamUrlType_None = 0,
	StreamUrlType_Live,		//live
	StreamUrlType_Vod ,		//storage
    StreamUrlType_Talkback,  //talkback
};

class MSMQ_API MSStreamUrl
{
public:
	//rtsp://admin:1111@192.168.10.11/live/1/0
	//rtsp://admin:1111@192.168.10.11/vod/1/1/20150819111111/20150820111111
	static std::string buildLiveStreamUrl(const std::string& serveraddr, uint32_t rtspport, const std::string& username, const std::string& password, const DeviceId& devid, MSProtoStreamType type);
	static std::string buildVodStreamUrl(const std::string& serveraddr, uint32_t rtspport, const std::string& username, const std::string& password, const DeviceId& devid, MSProtoStreamType type, uint64_t starttime, uint64_t stoptime);
    static std::string buildTalkbackStreamUrl(const std::string& serveraddr, uint32_t rtspport, const std::string& username, const std::string& password, const DeviceId& devid);
	
	static StreamUrlType parseStreamUrlType(const std::string& streamurl);
	
	static bool parseLiveStreamUrl(const std::string& streamurl, DeviceId& devid, MSProtoStreamType& type);
	static bool parseVodStreamUrl(const std::string& streamurl, DeviceId& devid, MSProtoStreamType& type, uint64_t& starttime, uint64_t& stoptime);
    static bool parseTalkbackStreamUrl(const std::string& streamurl, DeviceId& devide);
};

}
}
