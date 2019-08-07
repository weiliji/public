#pragma once
#include "rtpSession.h"
#include "rtcp.h"

class rtpOverTcpSesssion :public RTPSession
{
public:
	typedef Function3<void, const shared_ptr<STREAM_TRANS_INFO>&, const char*, uint32_t > SendContrlDataCallback;
	typedef Function3<void, const shared_ptr<STREAM_TRANS_INFO>&, const RTPHEADER&, const StringBuffer& > SendMediaDataCallback;
public:
	rtpOverTcpSesssion(const shared_ptr<STREAM_TRANS_INFO>& _transport, 
		const SendMediaDataCallback& _sendmediacallback,const SendContrlDataCallback& _sendcontrlcallback,
		const MediaDataCallback& _datacallback, const ContorlDataCallback& _contorlcallback)
		:RTPSession(_transport, _datacallback, _contorlcallback),firsthearbeta(true), rtpsn(0)
		,sendmediacallback(_sendmediacallback),sendcontorlcallback(_sendcontrlcallback)
	{}
	~rtpOverTcpSesssion()
	{
	}
	void rtpovertcpContorlCallback(const shared_ptr<STREAM_TRANS_INFO>& mediainfo, const char*  buffer, uint32_t bufferlen)
	{
		contorlcallback(mediainfo, buffer, bufferlen);
	}
	void rtpovertcpMediaCallback(const shared_ptr<STREAM_TRANS_INFO>& mediainfo, const RTPHEADER& rtpheader, const StringBuffer& buffer)
	{
		datacallback(mediainfo, rtpheader, buffer);
	}
	void sendContorlData(const shared_ptr<STREAM_TRANS_INFO>& transportinfo, const char*  buffer, uint32_t bufferlen)
	{
		sendcontorlcallback(transportinfo, buffer, bufferlen);
	}
	void sendMediaData(const shared_ptr<STREAM_TRANS_INFO>& transportinfo, uint32_t timestmap, const StringBuffer& buffer, bool mark)
	{
		uint32_t readlen = 0;
		while (readlen < buffer.length())
		{
			uint32_t cansendlen = min(MAXRTPPACKETLEN, (int)(buffer.length() - readlen));

			RTPHEADER rtpheader;
			memset(&rtpheader, 0, sizeof(RTPHEADER));
			rtpheader.v = RTP_VERSION;
			rtpheader.ts = htonl(timestmap);
			rtpheader.seq = htons(rtpsn++);
			rtpheader.pt = transportinfo->streaminfo.nPayLoad;
			rtpheader.m = (buffer.length() - readlen) == cansendlen ? mark : false;
			rtpheader.ssrc = htonl(transportinfo->transportinfo.ssrc);
			

			StringBuffer newbuffer = buffer.readBuffer(readlen, cansendlen);

			sendmediacallback(transportinfo, rtpheader, newbuffer);

			readlen += cansendlen;
		}
		
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
	uint16_t				 rtpsn;

	SendMediaDataCallback	sendmediacallback;
	SendContrlDataCallback	sendcontorlcallback;
};