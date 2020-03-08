#include "RTSP/RTSPServer.h"
#include "../common/rtspSession.h"

using namespace Public::RTSP;


struct _RTSPServerSession:public RTSPSession
{
	RTSPServer::ListenCallback				queryhandlercallback;	
	shared_ptr<RTSPServerHandler>			handler;
	weak_ptr<RTSPServerSession>				session;

	
	_RTSPServerSession(const shared_ptr<IOWorker>& _worker, const RTSPServer::ListenCallback& queryhandle, const shared_ptr<UDPPortAlloc>& portalloc, const std::string&  _useragent)
		:RTSPSession(_worker,RTSPUrl(),portalloc,_useragent,true),queryhandlercallback(queryhandle)
	{
		
	}
	virtual ~_RTSPServerSession()
	{
		handler = NULL;
		RTSPSession::stop();
	}
	virtual shared_ptr<RTSPHandler> queryRtspHandler() 
	{
		if (handler == NULL)
		{
			handler = queryhandlercallback(session.lock());
		}

		return handler;
	}
	virtual shared_ptr<RTSPCommandSender> queryCommandSender() { return session.lock(); }

	void closedByTimeout()
	{
		shared_ptr<RTSPCommandSender> sender = queryCommandSender();
		if (handler && sender) handler->onClose(sender, ErrorInfo(Error_Code_Fail, "session timeout"));
	}
};

struct RTSPServerSession::RTSPServerSessionInternal
{
	shared_ptr<_RTSPServerSession> session;
};

RTSPServerSession::RTSPServerSession(const shared_ptr<IOWorker>& worker, const RTSPServer::ListenCallback& querycallback, const shared_ptr<UDPPortAlloc>& portalloc, const std::string& useragent)
{
	internal = new RTSPServerSessionInternal;
	internal->session = make_shared<_RTSPServerSession>(worker, querycallback, portalloc, useragent);
}

void RTSPServerSession::initRTSPServerSessionPtr(const weak_ptr<RTSPServerSession>& session, const shared_ptr<Socket>& _sock, const char* buffer, uint32_t len)
{
	internal->session->session = session;
	internal->session->start(_sock, buffer, len);
}
RTSPServerSession::~RTSPServerSession()
{
	disconnect();

	SAFE_DELETE(internal);
}
void RTSPServerSession::onPoolTimerProc()
{
	shared_ptr<_RTSPServerSession> session = internal->session;
	if (session) session->onPoolTimerProc();
}
void RTSPServerSession::closedByTimeout()
{
	shared_ptr<_RTSPServerSession> session = internal->session;
	if (session) session->closedByTimeout();
}
void RTSPServerSession::disconnect()
{
    shared_ptr<_RTSPServerSession> tmpSession = internal->session;
    if(tmpSession)
	{
        tmpSession->stop();
        tmpSession->handler = NULL;
	}
	internal->session = NULL;
}

const RTSPUrl& RTSPServerSession::url() const
{
	return internal->session->rtspurl;
}
bool RTSPServerSession::sessionIsTimeout() const
{
	if (internal->session == NULL || internal->session->sessionIsTimeout()) return true;

	return false;
}
NetStatus RTSPServerSession::connectStatus() const
{
	if (internal->session == NULL) return NetStatus_disconnected;

	return internal->session->connectStatus();
}
shared_ptr<RTSPServerHandler> RTSPServerSession::handler() const
{
	if (internal->session == NULL) return shared_ptr<RTSPServerHandler>();

	return internal->session->handler;
}
shared_ptr<RTSP_Media_Infos> RTSPServerSession::mediainfo() const
{
	if (internal->session == NULL) return shared_ptr<RTSP_Media_Infos>();

	return internal->session->rtspmedia;
}
void RTSPServerSession::sendOptionResponse(const shared_ptr<RTSPCommandInfo>& cmdinfo, const std::string& cmdstr)
{
	if (internal->session)
		internal->session->sendOptionResponse(cmdinfo, cmdstr);
}
void RTSPServerSession::sendDescribeResponse(const shared_ptr<RTSPCommandInfo>& cmdinfo, const RTSP_Media_Infos& mediainfo)
{
	if (internal->session)
		internal->session->sendDescribeResponse(cmdinfo, mediainfo);
}
void RTSPServerSession::sendSetupResponse(const shared_ptr<RTSPCommandInfo>& cmdinfo, const shared_ptr<STREAM_TRANS_INFO>& transport)
{
	if (internal->session)
		internal->session->sendSetupResponse(cmdinfo, transport);
}
void RTSPServerSession::sendPlayResponse(const shared_ptr<RTSPCommandInfo>& cmdinfo)
{
	if (internal->session)
		internal->session->sendPlayResponse(cmdinfo);
}
void RTSPServerSession::sendPauseResponse(const shared_ptr<RTSPCommandInfo>& cmdinfo)
{
	if (internal->session)
		internal->session->sendPauseResponse(cmdinfo);
}
void RTSPServerSession::sendTeardownResponse(const shared_ptr<RTSPCommandInfo>& cmdinfo)
{
	if (internal->session)
		internal->session->sendTeardownResponse(cmdinfo);
}
void RTSPServerSession::sendGetparameterResponse(const shared_ptr<RTSPCommandInfo>& cmdinfo, const std::string& content)
{
	if (internal->session)
		internal->session->sendGetparameterResponse(cmdinfo, content);
}

void RTSPServerSession::sendErrorResponse(const shared_ptr<RTSPCommandInfo>& cmdinfo, int errcode, const std::string& errmsg)
{
	if(internal->session)
		internal->session->sendErrorResponse(cmdinfo, errcode, errmsg);
}

ErrorInfo RTSPServerSession::sendRTPFrame(const shared_ptr<STREAM_TRANS_INFO>& transport, const shared_ptr<RTPFrame>& frame)
{
    if (internal->session == NULL)
    {
        return ErrorInfo(Error_Code_Param);
    }

	shared_ptr<RTSPMediaSessionInfo> mediasession = internal->session->rtspmedia;
    if (mediasession == NULL)
    {
        return ErrorInfo(Error_Code_Param);
    }

	shared_ptr<MediaSession> rtpsession = mediasession->session(transport);

    if (transport == NULL || rtpsession == NULL)
    {
        return ErrorInfo(Error_Code_Param);
    }

	rtpsession->sendMediaFrameData(transport, frame);

	return ErrorInfo();
}
ErrorInfo RTSPServerSession::sendRTCPPackage(const shared_ptr<STREAM_TRANS_INFO>& transport, const shared_ptr<RTCPPackage>& rtcppackage)
{
	if (internal->session == NULL) return ErrorInfo(Error_Code_Param);

	shared_ptr<RTSPMediaSessionInfo> mediasession = internal->session->rtspmedia;
	if (mediasession == NULL) return ErrorInfo(Error_Code_Param);

	shared_ptr<MediaSession> rtpsession = mediasession->session(transport);

	if (transport == NULL || rtpsession == NULL) return ErrorInfo(Error_Code_Param);

	rtpsession->sendContorlData(transport,rtcppackage);

	return ErrorInfo();
}

uint32_t RTSPServerSession::getSendListSize()
{
	if (internal->session == NULL) return 0;

	return internal->session->getSendListSize();
}

uint32_t RTSPServerSession::getSendCacheSize()
{
	if (internal->session == NULL) return 0;

	return internal->session->getSendCacheSize();
}

ErrorInfo RTSPServerSession::cleanMediaCacheData()
{
	if (internal->session == NULL) return ErrorInfo(Error_Code_Param);

	return internal->session->cleanMediaCacheData();
}

void RTSPServerSession::setAutoTimeStampEnable(const shared_ptr<STREAM_TRANS_INFO>& transport, bool enable)
{
	if (internal->session == NULL) return;

	internal->session->setAutoTimeStampEnable(transport, enable);
}



