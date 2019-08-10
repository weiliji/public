#pragma once
#include "rtpSession.h"
#include "rtcp.h"

class rtpOverTcpSesssion :public RTPSession
{
public:
	typedef Function3<void, const shared_ptr<STREAM_TRANS_INFO>&, const char*, uint32_t > SendContrlDataCallback;
	typedef Function2<void, const shared_ptr<STREAM_TRANS_INFO>&, const RTPPackage& > SendMediaDataCallback;
public:
	rtpOverTcpSesssion(const shared_ptr<STREAM_TRANS_INFO>& _transport, 
		const SendMediaDataCallback& _sendmediacallback,const SendContrlDataCallback& _sendcontrlcallback,
		const MediaDataCallback& _datacallback, const ContorlDataCallback& _contorlcallback)
		:RTPSession(_transport, _datacallback, _contorlcallback),firsthearbeta(true)
		,sendmediacallback(_sendmediacallback),sendcontorlcallback(_sendcontrlcallback)
	{}
	~rtpOverTcpSesssion()
	{
	}
	void rtpovertcpContorlCallback(const shared_ptr<STREAM_TRANS_INFO>& mediainfo, const char*  buffer, uint32_t bufferlen)
	{
		contorlcallback(mediainfo, buffer, bufferlen);
	}
	void rtpovertcpMediaCallback(const shared_ptr<STREAM_TRANS_INFO>& mediainfo, const RTPPackage& rtppackge)
	{
		datacallback(mediainfo, rtppackge);
	}
	void sendContorlData(const shared_ptr<STREAM_TRANS_INFO>& transportinfo, const char*  buffer, uint32_t bufferlen)
	{
		sendcontorlcallback(transportinfo, buffer, bufferlen);
	}
	void sendMediaData(const shared_ptr<STREAM_TRANS_INFO>& transportinfo, const RTPPackage& rtppackge)
	{
		sendmediacallback(transportinfo, rtppackge);
	}
	bool onPoolHeartbeat()
	{
		std::string hearbeatadata = firsthearbeta ? firstRtpOverTcpRTCPHeartBeat() : normalRtpOverTcpRTCPHeartBeat();
		firsthearbeta = false;

		sendcontorlcallback(transportinfo, hearbeatadata.c_str(), (uint32_t)hearbeatadata.length());

		return true;
	}
private:
	bool					 firsthearbeta;

	SendMediaDataCallback	sendmediacallback;
	SendContrlDataCallback	sendcontorlcallback;
};