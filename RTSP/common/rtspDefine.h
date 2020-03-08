//
//  Copyright (c)1998-2012, Public Technology
//  All Rights Reserved.
//
//	Description:
//	$Id: File.h 252 2013-12-18 04:40:28Z  $
//


#pragma once
#include "RTSP/RTSPStructs.h"
#include "RTSP/RTSPStatistics.h"
#include "RTSP/RTPFrame.h"
#include "RTSP/RTPPackage.h"

namespace Public {
namespace RTSP {
//RTSP的会话及transport信息
struct RTSP_API RTSPMediaSessionInfo :public RTSP_Media_Infos
{
private:
	std::map<STREAM_TRANS_INFO*, shared_ptr<MediaSession> >	mediasessionlist;
	std::map<STREAM_TRANS_INFO*, shared_ptr<RTSPStatistics> >rtspstatisticslist;
public:
	shared_ptr<MediaSession> session(const shared_ptr<STREAM_TRANS_INFO>& trans)
	{
		return trans->mediasession.lock();
	}
	void setSession(const shared_ptr<STREAM_TRANS_INFO>& trans, const shared_ptr<MediaSession>& session)
	{
		shared_ptr<RTSPStatistics> rtspstatistics = make_shared<RTSPStatistics>();

		mediasessionlist[trans.get()] = session;
		rtspstatisticslist[trans.get()] = rtspstatistics;

		trans->mediasession = session;
		trans->rtspstatistics = rtspstatistics;
	}
	shared_ptr<MediaSession> querySession(const shared_ptr<STREAM_TRANS_INFO>& trans)
	{
		std::map<STREAM_TRANS_INFO*, shared_ptr<MediaSession> >::iterator iter = mediasessionlist.find(trans.get());
		if (iter == mediasessionlist.end()) return shared_ptr<MediaSession>();

		return iter->second;
	}
	uint32_t sessionsize() { return (uint32_t)mediasessionlist.size(); }
	void stop()
	{
		mediasessionlist.clear();
		rtspstatisticslist.clear();
	}
	RTSPMediaSessionInfo() {}
	RTSPMediaSessionInfo(const RTSP_Media_Infos& info)
	{
		*(RTSP_Media_Infos*)this = info;

		for (std::list< shared_ptr<STREAM_TRANS_INFO> >::iterator iter = infos.begin(); iter != infos.end(); iter++)
		{
			(*iter)->mediasession = weak_ptr<MediaSession>();
		}
	}
	~RTSPMediaSessionInfo()
	{
		stop();
	}
};

#define MAXRTPSNNUM	65536

//RTP包发送对象
class RTPContorlSender
{
public:
	RTPContorlSender() {}
	virtual ~RTPContorlSender() {}

	virtual void sendContorlData(const shared_ptr<STREAM_TRANS_INFO>&, const shared_ptr<RTCPPackage>& rtcppackage) = 0;
};

//RTP包发送对象
class RTPSender:public RTPContorlSender
{
public:
	RTPSender() {}
	virtual ~RTPSender() {}

	virtual void sendMediaData(const shared_ptr<STREAM_TRANS_INFO>&, const std::vector<shared_ptr<RTPPackage>>&) = 0;
};

struct SendPackgeInfo:public Socket::SendBuffer
{
public:
	enum PackgeType
	{
		PackgeType_RtpData = 0,
		PackgeType_RtcpData,
		packgeType_RtspCommand,
	};
private:
	shared_ptr<RTPPackage>		rtp;
	std::string						data;
public:
	const char*					buffer = NULL;
	uint32_t					len = 0;
	uint32_t					sendpos = 0;
	PackgeType					type = PackgeType_RtpData;

	SendPackgeInfo() :sendpos(0) {}
	SendPackgeInfo(const SendPackgeInfo& info)
	{
		rtp = info.rtp;
		data = info.data;
		len = info.len;
		sendpos = info.sendpos;
		type = info.type;
		buffer = data.c_str();
	}
	SendPackgeInfo(const shared_ptr<RTPPackage>& rtppackge, PackgeType _type = PackgeType_RtpData) :rtp(rtppackge), buffer(rtppackge->buffer()), len(rtppackge->bufferlen()), sendpos(0), type(_type) {}
	SendPackgeInfo(const std::string& _data, PackgeType _type = PackgeType_RtpData) :data(_data), buffer(data.c_str()), len((uint32_t)_data.length()), sendpos(0), type(_type) {}

	virtual const char* bufferaddr() { return buffer; }
	virtual uint32_t bufferlen() { return len; }
};


#define RTSPCMDFLAG			"RTSP/1.0"

#define RTSPCONENTTYPESDP	"application/sdp"

}
}
