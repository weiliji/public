//
//  Copyright (c)1998-2012, Public Technology
//  All Rights Reserved.
//
//	Description:
//	$Id: File.h 252 2013-12-18 04:40:28Z  $
//


#pragma once
#include "Defs.h"
#include "Base/Base.h"
#include "RTSPStructs.h"
#include "RTSPUrl.h"
#include "Network/Network.h"
#include "RTSPHandler.h"
using namespace Public::Base;
using namespace Public::Network;

namespace Public {
namespace RTSP {

class RTSPClientManager;
class RTSPClientHandler;

class RTSP_API RTSPClient:public RTSPCommandSender,public enable_shared_from_this<RTSPCommandSender>
{
	friend class RTSPClientManager;
	struct RTSPClientInternal;
	RTSPClient(const std::shared_ptr<IOWorker>& work, const shared_ptr<RTSPClientHandler>& handler, const shared_ptr<UDPPortAlloc>& portalloc,const RTSPUrl& rtspUrl,const std::string& useragent);
	//定时器处理
	void onPoolTimerProc();
public:
	~RTSPClient();

	/*设置RTP数据接收方式 0:TCP，1:UDP  默认UDP*/
	bool initRTPOverTcpType();
	bool initRTPOverUdpType();

	/*准备数据接收，包括启动数据接收线程，心跳线程*/
	//timeout 连接超时时间，
	//reconnect 是否启用重连
	ErrorInfo start(uint32_t timeout = 10000);

	ErrorInfo stop();

	//异步命令，使用RTSPClientHandler->onPlayResponse接收结果
	shared_ptr<RTSPCommandInfo> sendPlayRequest(const PlayInfo& range);
	//同步命令，同步返回
	ErrorInfo sendPlayRequest(const PlayInfo& range, uint32_t timeout);

	//异步命令，使用RTSPClientHandler->onPauseResponse接收结果
	shared_ptr<RTSPCommandInfo> sendPauseRequest();
	//同步命令,同步返回
	ErrorInfo sendPauseRequest(uint32_t timeout);


	//异步命令，使用RTSPClientHandler->onGetparameterResponse接收结果
	shared_ptr<RTSPCommandInfo> sendGetparameterRequest(const std::string& body);
	//同步命令,同步返回
	ErrorInfo sendGetparameterRequest(const std::string& body, uint32_t timeout);


	//异步命令，使用RTSPClientHandler->onTeradownResponse接收结果
	shared_ptr<RTSPCommandInfo> sendTeradownRequest();
	//同步命令,同步返回
	ErrorInfo sendTeradownRequest(uint32_t timeout);

	ErrorInfo sendRTPFrame(const shared_ptr<STREAM_TRANS_INFO>& transport, const shared_ptr<RTPFrame>& rtppackge);
	ErrorInfo sendRTCPPackage(const shared_ptr<STREAM_TRANS_INFO>& transport, const shared_ptr<RTCPPackage>& rtcppackge);
private:
	RTSPClientInternal *internal;
};

class RTSP_API RTSPClientHandler:public RTSPHandler
{
public:
	RTSPClientHandler() {}
	virtual ~RTSPClientHandler() {}
	
	virtual void onConnectResponse(shared_ptr<RTSPCommandSender>& sender, const ErrorInfo& err) {}
	virtual void onErrorResponse(shared_ptr<RTSPCommandSender>& sender, const shared_ptr<RTSPCommandInfo>& cmdinfo, int errcode, const std::string& errmsg) {}

	virtual void onOptionResponse(shared_ptr<RTSPCommandSender>& sender, const shared_ptr<RTSPCommandInfo>& cmdinfo, const ErrorInfo& err, const std::string& optstr) {}
	virtual void onDescribeResponse(shared_ptr<RTSPCommandSender>& sender, const shared_ptr<RTSPCommandInfo>& cmdinfo, const shared_ptr<RTSP_Media_Infos>& info) { info->cleanExStreamInfo(); }
	virtual void onSetupResponse(shared_ptr<RTSPCommandSender>& sender, const shared_ptr<RTSPCommandInfo>& cmdinfo, const shared_ptr<STREAM_TRANS_INFO>& transport) {}
	virtual void onPlayResponse(shared_ptr<RTSPCommandSender>& sender, const shared_ptr<RTSPCommandInfo>& cmdinfo) {}
	virtual void onPauseResponse(shared_ptr<RTSPCommandSender>& sender, const shared_ptr<RTSPCommandInfo>& cmdinfo, const ErrorInfo& err) {}
	virtual void onGetparameterResponse(shared_ptr<RTSPCommandSender>& sender, const shared_ptr<RTSPCommandInfo>& cmdinfo, const std::string& content) {}
	virtual void onTeradownResponse(shared_ptr<RTSPCommandSender>& sender, const shared_ptr<RTSPCommandInfo>& cmdinfo) {}
	
	virtual void onRTPFrameCallback(shared_ptr<RTSPCommandSender>& sender, const shared_ptr<STREAM_TRANS_INFO>& transport, const shared_ptr<RTPFrame>& frame) {}
	//virtual void onRTPPackageCallback(shared_ptr<RTSPCommandSender>& sender, const shared_ptr<STREAM_TRANS_INFO>& transport, const shared_ptr<RTPPackage>& rtppackge) {}
	virtual void onRTCPPackageCallback(shared_ptr<RTSPCommandSender>& sender, const shared_ptr<STREAM_TRANS_INFO>& transport, const shared_ptr<RTCPPackage>& rtcppackge) {}

	virtual void onClose(shared_ptr<RTSPCommandSender>& sender, const ErrorInfo& err) {}
};

class RTSP_API RTSPClientManager
{
	struct RTSPClientManagerInternal;
public:
	//userContent 用户描述信息,threadNum 线程数，根据RTSP的用户量决定
	RTSPClientManager(const shared_ptr<IOWorker>& iowrker,const std::string& useragent);
	~RTSPClientManager();

	//rtp 模式地址
	bool initRTPOverUdpPort(uint32_t startport = 40000, uint32_t stopport = 41000);

	//创建一个对象
	shared_ptr<RTSPClient> create(const shared_ptr<RTSPClientHandler>& handler, const RTSPUrl& pRtspUrl);
private:
	RTSPClientManagerInternal * internal;
};


}
}
