//
//  Copyright (c)1998-2012, Public Technology
//  All Rights Reserved.
//
//	Description:
//	$Id: File.h 252 2013-12-18 04:40:28Z  $
//


#pragma once

#include "RTSP/Defs.h"
#include "RTSP/RTSPStructs.h"
#include "RTSP/RTSPUrl.h"
#include "Network/Network.h"
#include "Base/Base.h"
#include "RTSPHandler.h"

using namespace Public::Base;
using namespace Public::Network;

namespace Public {
namespace RTSP {

class RTSPServerSession;
class RTSPServerHandler;

class RTSP_API RTSPServer
{
public:
	struct RTSPServerInternal;
	typedef Function<shared_ptr<RTSPServerHandler>(const shared_ptr<RTSPServerSession>&)> ListenCallback;
public:
	RTSPServer( const std::string& useragent);
	virtual ~RTSPServer();

	bool initRTPOverUdpType(uint32_t startport = 40000, uint32_t stopport = 41000);

	bool run(const shared_ptr<IOWorker>& worker, uint32_t port, const ListenCallback& callback);
	bool stop();
	uint32_t listenPort() const;

	void inputSocket(const shared_ptr<Socket>& sock, const char* buffer = NULL, uint32_t bufferlen = 0);
private:
	RTSPServerInternal* internal;
};

class RTSP_API RTSPServerSession:public RTSPCommandSender,public enable_shared_from_this<RTSPServerSession>
{
	friend struct RTSPServer::RTSPServerInternal;
private:
	RTSPServerSession(const shared_ptr<IOWorker>& worker, const RTSPServer::ListenCallback& querycallback, const shared_ptr<UDPPortAlloc>& portalloc, const std::string& useragent);
	void initRTSPServerSessionPtr(const weak_ptr<RTSPServerSession>& session, const shared_ptr<Socket>& _sock, const char* buffer, uint32_t len);

	//定时器处理
	void onPoolTimerProc();

	void closedByTimeout();
public:
	virtual ~RTSPServerSession();

	void disconnect();

	const RTSPUrl& url() const;
	bool sessionIsTimeout() const;
	NetStatus connectStatus() const;
	shared_ptr<RTSPServerHandler> handler() const;
	shared_ptr<RTSP_Media_Infos> mediainfo() const;

	virtual void sendOptionResponse(const shared_ptr<RTSPCommandInfo>& cmdinfo, const std::string& cmdstr = "");
	virtual void sendDescribeResponse(const shared_ptr<RTSPCommandInfo>& cmdinfo, const RTSP_Media_Infos& mediainfo);
	virtual void sendSetupResponse(const shared_ptr<RTSPCommandInfo>& cmdinfo, const shared_ptr<STREAM_TRANS_INFO>& transport);
	virtual void sendPlayResponse(const shared_ptr<RTSPCommandInfo>& cmdinfo);
	virtual void sendPauseResponse(const shared_ptr<RTSPCommandInfo>& cmdinfo);
	virtual void sendGetparameterResponse(const shared_ptr<RTSPCommandInfo>& cmdinfo, const std::string& content);
	virtual void sendTeardownResponse(const shared_ptr<RTSPCommandInfo>& cmdinfo);
	virtual void sendErrorResponse(const shared_ptr<RTSPCommandInfo>& cmdinfo, int errcode, const std::string& errmsg);

	virtual ErrorInfo sendRTPFrame(const shared_ptr<STREAM_TRANS_INFO>& transport, const shared_ptr<RTPFrame>& frame);
	virtual ErrorInfo sendRTCPPackage(const shared_ptr<STREAM_TRANS_INFO>& transport, const shared_ptr<RTCPPackage>& rtcppackge);

	//获取发送队列长度
	virtual uint32_t getSendListSize();
	//获取发送缓冲区数据缓存大小
	virtual uint32_t getSendCacheSize();

	virtual ErrorInfo cleanMediaCacheData();

	virtual void setAutoTimeStampEnable(const shared_ptr<STREAM_TRANS_INFO>& transport, bool enable);
private:
	struct RTSPServerSessionInternal;
	RTSPServerSessionInternal* internal;
};

class RTSP_API RTSPServerHandler :public RTSPHandler
{
public:
	RTSPServerHandler() {}
	virtual ~RTSPServerHandler() {}

	virtual ErrorInfo queryUserPassword(const shared_ptr<RTSPCommandSender>& sender, const std::string& username, std::string& passwd) { return ErrorInfo(Error_Code_NotSupport); }

	virtual void onOptionRequest(shared_ptr<RTSPCommandSender>& sender, const shared_ptr<RTSPCommandInfo>& cmdinfo) { sender->sendOptionResponse(cmdinfo); }
	virtual void onDescribeRequest(shared_ptr<RTSPCommandSender>& sender, const shared_ptr<RTSPCommandInfo>& cmdinfo) = 0;
	virtual void onSetupRequest(shared_ptr<RTSPCommandSender>& sender, const shared_ptr<RTSPCommandInfo>& cmdinfo, const shared_ptr<STREAM_TRANS_INFO>& transport) { sender->sendSetupResponse(cmdinfo, transport); }
	virtual void onPlayRequest(shared_ptr<RTSPCommandSender>& sender, const shared_ptr<RTSPCommandInfo>& cmdinfo, const shared_ptr<RTSP_Media_Infos>& mediainfo, const PlayInfo& range) { sender->sendPauseResponse(cmdinfo); }
	virtual void onPauseRequest(shared_ptr<RTSPCommandSender>& sender, const shared_ptr<RTSPCommandInfo>& cmdinfo) { sender->sendErrorResponse(cmdinfo, 500, "NOT SUPPORT"); }
	virtual void onGetparameterRequest(shared_ptr<RTSPCommandSender>& sender, const shared_ptr<RTSPCommandInfo>& cmdinfo, const std::string& content) { sender->sendGetparameterResponse(cmdinfo,content); }
	virtual void onTeardownRequest(shared_ptr<RTSPCommandSender>& sender, const shared_ptr<RTSPCommandInfo>& cmdinfo) { sender->sendTeardownResponse(cmdinfo); }
	virtual void onRTPFrameCallback(shared_ptr<RTSPCommandSender>& sender, const shared_ptr<STREAM_TRANS_INFO>& transport, const shared_ptr<RTPFrame>& frame) {};

	//virtual void onRTPPackageCallback(shared_ptr<RTSPCommandSender>& sender, const shared_ptr<STREAM_TRANS_INFO>& transport, const shared_ptr<RTPPackage>& rtppackge) {}
	virtual void onRTCPPackageCallback(shared_ptr<RTSPCommandSender>& sender, const shared_ptr<STREAM_TRANS_INFO>& transport, const shared_ptr<RTCPPackage>& rtcppackge) {}

	//socket disconnect or session timeout or teradown
	virtual void onClose(shared_ptr<RTSPCommandSender>& sender, const ErrorInfo& err) {}
};

}
}

