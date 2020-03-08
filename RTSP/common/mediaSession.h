#pragma  once
#include "Base/Base.h"
#include "RTSP/RTSPStructs.h"
#include "rtspDefine.h"
#include "rtcpSession.h"
#include "RTSP/RTPAnalyzer.h"

using namespace Public::Base;
namespace Public {
namespace RTSP {

#define STREAM_TIMEOUT (10*1000)

//RTP会话对象，处理RTP数据的发送和接受
class MediaSession:public RTPContorlSender,public enable_shared_from_this<RTPContorlSender>
{
public:
	typedef Function<void()> StreamTimeoutCallback;
public:
	MediaSession(const shared_ptr<STREAM_TRANS_INFO>& _transport,const shared_ptr<RTSPHandler>& _rtsphandler,const shared_ptr<RTSPCommandSender>& _sender, bool _isserver)
		:transportinfo(_transport), rtsphandler(_rtsphandler), rtspsender(_sender),isserver(_isserver)
	{
		
	}
	virtual ~MediaSession() 
	{
		rtpanalyzer = NULL;
		rtpbuilder = NULL;
	}
	virtual void start(const std::string& useragent, const StreamTimeoutCallback& timeoutcallback)
	{
		if (isserver) rtcpsession = make_shared<rtcpSessionSender>(transportinfo,shared_from_this(), rtsphandler.lock(),useragent);
		else rtcpsession = make_shared<rtcpSessionReciver>(transportinfo, shared_from_this(), rtsphandler.lock(), useragent);
		
		streamTimeoutCallback = timeoutcallback;

		rtpanalyzer = make_shared<RTPAnalyzer>(RTPAnalyzer::FrameCallback(&MediaSession::rtpFrameCallback, this));
		rtpbuilder = make_shared<RTPBuilder>(transportinfo);
	}
	virtual void stop() 
	{
		rtcpsession = NULL;
	}
	virtual void rtpovertcpContorlCallback(const shared_ptr<STREAM_TRANS_INFO>& transport, const shared_ptr<RTCPPackage>& rtcppackage){}
	virtual void rtpovertcpMediaCallback(const shared_ptr<STREAM_TRANS_INFO>& transport, const shared_ptr<RTPPackage>& rtppackge) {}
	virtual void rtpFrameCallback(const shared_ptr<RTPFrame>& frame) {}
	
	virtual void sendMediaFrameData(const shared_ptr<STREAM_TRANS_INFO>&, const shared_ptr<RTPFrame>&) = 0;

	virtual bool hasStream() { return true; };

	virtual void onPoolTimerProc() 
	{
		shared_ptr<rtcpSession> rtcp = rtcpsession;

		if (rtcp) rtcp->onPoolTimerProc();

		if (!isserver && transportinfo->streaminfo.frametype == FrameType_Video && !hasStream())
		{
			streamTimeoutCallback();
		}
	}
	virtual bool sessionIsTimeout()
	{
		shared_ptr<rtcpSession> rtcp = rtcpsession;
		if (rtcp == NULL) return false;

		return rtcp->sessionIsTimeout();
	}

	void setAutoTimeStampEnable(bool enable) 
	{
		shared_ptr<RTPBuilder>	tmpBuilder = rtpbuilder;
		if (tmpBuilder)
		{
			 tmpBuilder->setAutoTimeStampEnable(enable);
		}
	}
	
protected:
	shared_ptr<STREAM_TRANS_INFO>	transportinfo;
	weak_ptr<RTSPHandler>			rtsphandler;
	weak_ptr<RTSPCommandSender>		rtspsender;
	shared_ptr<rtcpSession>			rtcpsession;
	shared_ptr<RTPAnalyzer>			rtpanalyzer;
	shared_ptr<RTPBuilder>			rtpbuilder;

	StreamTimeoutCallback			streamTimeoutCallback;

	bool							isserver = false;
};


}
}
