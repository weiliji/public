#pragma once
#include "Base/Base.h"
#include "RTSP/RTSPStructs.h"
#include "RTSP/RTSPHandler.h"
#include "RTSP/RTSPStatistics.h"
#include "RTSP/RTCPPackage.h"
#include "rtspDefine.h"

using namespace Public::Base;
using namespace Public::RTSP;


class rtcpSession
{
public:
	rtcpSession(const shared_ptr<STREAM_TRANS_INFO>& _transport, const shared_ptr<RTPContorlSender>& _sender,const shared_ptr<RTSPHandler>& _handler,const std::string& _useragent)
	:transport(_transport),sender(_sender), handler(_handler),useragent(_useragent)
	{
	}
	virtual ~rtcpSession(){}

	virtual void inputRecvRTCP(const shared_ptr <RTCPPackage>& rtcppage) = 0;
	//rtcp is timeout
	virtual bool sessionIsTimeout() = 0;
	virtual void onPoolTimerProc() {}
protected:
	shared_ptr<STREAM_TRANS_INFO>		transport;
	weak_ptr<RTPContorlSender>			sender;
	weak_ptr<RTSPHandler>				handler;
	std::string							useragent;
};


class rtcpSessionSender:public rtcpSession
{

#define SENDERRTCPINTERVER			5*1000
public:
	rtcpSessionSender(const shared_ptr<STREAM_TRANS_INFO>& _transport, const shared_ptr<RTPContorlSender>& _sender, const shared_ptr<RTSPHandler>& handler, const std::string& _useragent)
		:rtcpSession(_transport, _sender, handler, _useragent)
	{
	}
	~rtcpSessionSender() {}

	//放入sender的发送rtp包信息
	virtual void onPoolTimerProc()
	{
		//构造 senderreport
		uint64_t nowtime = Time::getCurrentMilliSecond();

		if (prevSendTime == 0) prevSendTime = nowtime;

		if (nowtime - prevSendTime >= SENDERRTCPINTERVER)
		{
			buildAndSendSenderReport();

			prevSendTime = nowtime;
		}
	}

	virtual bool sessionIsTimeout()
	{
		uint64_t prevtime = prevRecvRRTime;
		uint64_t nowtime = Time::getCurrentMilliSecond();

		if (nowtime > prevtime&& nowtime - prevtime >= 60 * 1000) return true;

		return false;
	}

	void inputRecvRTCP(const shared_ptr <RTCPPackage>& rtcppage)
	{
		if (rtcppage == NULL) return;

		std::vector<shared_ptr<RTCPItem>> items = rtcppage->items();
		for (size_t i = 0; i < items.size(); i++)
		{
			if (items[i]->type() == RTCPType_RR)
			{
				prevRecvRRTime = Time::getCurrentMilliSecond();
			}
		}
	}
private:
	void buildAndSendSenderReport()
	{
		shared_ptr<RTCPPackage> rtcppage = make_shared<RTCPPackage>();

		rtcppage->addSenderReport(transport);
		rtcppage->addSourceDesribe(transport,RTCP_SDES_ID_CNAME, useragent);


		shared_ptr<RTPContorlSender> sendertmp = sender.lock();
		if (sendertmp) sendertmp->sendContorlData(transport, rtcppage);
	}
private:
	uint64_t							prevSendTime = 0;
	uint64_t							prevRecvRRTime = Time::getCurrentMilliSecond();
};

class rtcpSessionReciver :public rtcpSession
{
#define MAXSENDRTCPRECVIEREPORT		5000
public:
	rtcpSessionReciver(const shared_ptr<STREAM_TRANS_INFO>& _transport, const shared_ptr<RTPContorlSender>& _sender, const shared_ptr<RTSPHandler>& handler, const std::string& _useragent)
		:rtcpSession(_transport, _sender, handler,_useragent)
	{
		memset(&prevSendReport, 0, sizeof(prevSendReport));
	}
	~rtcpSessionReciver() {}

	virtual void onPoolTimerProc()
	{
		//check and send report		
		uint64_t nowtime = Time::getCurrentMilliSecond();
		if (prevSendTime == 0) prevSendTime = nowtime;
			
		if (nowtime > prevSendTime&& nowtime - prevSendTime >= MAXSENDRTCPRECVIEREPORT)
		{
			buildAndSendRecvReport();

			prevSendTime = nowtime;
		}		
	}

	virtual bool sessionIsTimeout() { return false; }

	//放入sender的发送rtp包信息
	void inputRecvRTCP(const shared_ptr < RTCPPackage>& rtcppage)
	{
		if (rtcppage == NULL) return;

		std::vector<shared_ptr<RTCPItem>> items = rtcppage->items();
		for (size_t i = 0; i < items.size(); i++)
		{
			if (items[i]->type() == RTCPType_SR)
			{
				rtcppage->getSenderReport(items[i],prevSendReport);
				break;
			}
		}
	}
private:
	void buildAndSendRecvReport()
	{
		shared_ptr < RTCPPackage> rtcppage = make_shared<RTCPPackage>();
		rtcppage->addReciverReport(transport, &prevSendReport);
		rtcppage->addSourceDesribe(transport,RTCP_SDES_ID_CNAME, useragent);
		

		shared_ptr<RTPContorlSender> sendertmp = sender.lock();
		if (sendertmp) sendertmp->sendContorlData(transport, rtcppage);
	}	
private:
	uint64_t							prevSendTime = 0;
	RTCPSenderReport					prevSendReport;
};