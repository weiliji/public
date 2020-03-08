#pragma once
#include "mediaSession.h"

class MediaSessionOverTcp :public MediaSession
{
#define MAXOVERTCPTIMEOUT			60*1000
public:
	MediaSessionOverTcp(bool _isserver, const shared_ptr<STREAM_TRANS_INFO>& _transport, const shared_ptr<RTSPHandler>& _mediahandler, const shared_ptr<RTSPCommandSender>& _rtspsender,const shared_ptr<RTPSender>& _sender)
		:MediaSession(_transport, _mediahandler, _rtspsender, _isserver),sender(_sender)
	{}
	~MediaSessionOverTcp()
	{
	}
	void rtpovertcpContorlCallback(const shared_ptr<STREAM_TRANS_INFO>& transport, const shared_ptr<RTCPPackage>& rtcppackage)
	{
		shared_ptr<RTSPStatistics> statistics = transportinfo->rtspstatistics.lock();
		if (statistics) statistics->inputRecvPackage(rtcppackage);

		shared_ptr<rtcpSession> rtcp = rtcpsession;
		if (rtcp) rtcp->inputRecvRTCP(rtcppackage);
		
		shared_ptr<RTSPHandler> handler = rtsphandler.lock();
		shared_ptr<RTSPCommandSender> cmdsender = rtspsender.lock();
		if (handler && cmdsender) handler->onRTCPPackageCallback(cmdsender,transport, rtcppackage);
	}
	void rtpovertcpMediaCallback(const shared_ptr<STREAM_TRANS_INFO>& transport, const shared_ptr < RTPPackage>& rtppackge)
	{
		shared_ptr<RTSPStatistics> statistics = transportinfo->rtspstatistics.lock();
		if (statistics) statistics->inputRecvPackage(rtppackge);
	
		shared_ptr<RTPAnalyzer> analyzer = rtpanalyzer;
		if (analyzer)
		{
			analyzer->inputRtpPacket(transport, rtppackge);
		}
	}

	void rtpFrameCallback(const shared_ptr<RTPFrame>& frame) 
	{
		shared_ptr<RTSPHandler> handler = rtsphandler.lock();
		shared_ptr<RTSPCommandSender> cmdsender = rtspsender.lock();
		if (handler && cmdsender)
		{
			streamTime = Time::getCurrentMilliSecond();

			handler->onRTPFrameCallback(cmdsender, transportinfo, frame);
		}
	}

	void sendContorlData(const shared_ptr<STREAM_TRANS_INFO>& transport, const shared_ptr < RTCPPackage>& rtcppackage)
	{
		shared_ptr<RTPSender> sendertmp = sender.lock();
		
		if (sendertmp) sendertmp->sendContorlData(transport, rtcppackage);
		//else assert(0);

		shared_ptr<RTSPStatistics> statistics = transportinfo->rtspstatistics.lock();
		if (statistics) statistics->inputSendPackage(rtcppackage);
	}

	void sendMediaFrameData(const shared_ptr<STREAM_TRANS_INFO>& transport, const shared_ptr<RTPFrame>& frame)
	{
		shared_ptr<RTPBuilder> builder = rtpbuilder;
		shared_ptr<RTPSender> sendertmp = sender.lock();
		if (builder && sendertmp)
		{
			std::vector<shared_ptr<RTPPackage>> rtplist = builder->inputFrame(frame);
			
			sendertmp->sendMediaData(transportinfo, rtplist);

			shared_ptr<RTSPStatistics> statistics = transportinfo->rtspstatistics.lock();

			for (size_t i = 0; i < rtplist.size(); i++)
			{
				if (statistics) statistics->inputRecvPackage(rtplist[i]);
			}
		}
	}

	virtual bool hasStream() 
	{
		return Time::getCurrentMilliSecond() - streamTime < STREAM_TIMEOUT;
	};

	virtual void onPoolTimerProc()
	{
		uint64_t nowTime = Time::getCurrentMilliSecond();
		if (prevHeartbeatTime == 0) prevHeartbeatTime = nowTime;

		if (!isserver && nowTime > prevHeartbeatTime&& nowTime - prevHeartbeatTime >= MAXOVERTCPTIMEOUT)
		{
			shared_ptr<RTSPCommandSender> cmdsender = rtspsender.lock();
			if (cmdsender) cmdsender->sendGetparameterRequest("");

			prevHeartbeatTime = nowTime;
		}

		MediaSession::onPoolTimerProc();
	}
private:
	weak_ptr<RTPSender>		sender;

	uint64_t				prevHeartbeatTime = 0;

	uint64_t				streamTime = Time::getCurrentMilliSecond();
};