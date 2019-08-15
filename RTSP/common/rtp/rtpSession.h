#pragma  once
#include "Base/Base.h"
#include "RTSP/RTSPStructs.h"
using namespace Public::Base;
using namespace Public::RTSP;

struct SendPackgeInfo
{
private:
	RTPPackage		rtp;
	String			data;
public:
	const char*		buffer;
	uint32_t		len;
	uint32_t		sendpos;

	SendPackgeInfo():sendpos(0){}
	SendPackgeInfo(const SendPackgeInfo& info)
	{
		rtp = info.rtp;
		data = info.data;
		buffer = info.buffer;
		len = info.len;
		sendpos = info.sendpos;
	}
	SendPackgeInfo(const RTPPackage& rtppackge) :rtp(rtppackge), buffer(rtppackge.buffer()), len(rtppackge.bufferlen()), sendpos(0){}
	SendPackgeInfo(const String& _data):data(_data),buffer(_data.c_str()),len((uint32_t)_data.length()), sendpos(0) {}
};


class RTPSession
{
public:
	//Fucntion4<bool isvideo,uint32_t timestmap,const char* buffer,uint32_t bufferlen,bool mark>RTPDataCallback
	typedef Function<void(const shared_ptr<STREAM_TRANS_INFO>&, const RTPPackage&)> MediaDataCallback;

	//控制数据回调
	typedef Function<void(const shared_ptr<STREAM_TRANS_INFO>&,const char*, uint32_t)> ContorlDataCallback;
public:
	RTPSession(const shared_ptr<STREAM_TRANS_INFO>& _transport,const MediaDataCallback& _datacallback,const ContorlDataCallback& _contorlcallback)
		:transportinfo(_transport),datacallback(_datacallback), contorlcallback(_contorlcallback){}
	virtual ~RTPSession() {}

	virtual void rtpovertcpContorlCallback(const shared_ptr<STREAM_TRANS_INFO>& mediainfo, const char*  buffer, uint32_t bufferlen){}
	virtual void rtpovertcpMediaCallback(const shared_ptr<STREAM_TRANS_INFO>& mediainfo, const RTPPackage& rtppackge) {}

	virtual void sendContorlData(const shared_ptr<STREAM_TRANS_INFO>& transportinfo, const char*  buffer, uint32_t bufferlen) = 0;
	virtual void sendMediaData(const shared_ptr<STREAM_TRANS_INFO>& transportinfo, const RTPPackage& rtppackge) = 0;
	virtual bool onPoolHeartbeat() { return false; }
protected:
	shared_ptr<STREAM_TRANS_INFO>	transportinfo;
	MediaDataCallback				datacallback;
	ContorlDataCallback				contorlcallback;
};