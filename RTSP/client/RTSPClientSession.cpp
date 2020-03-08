#include "RTSP/RTSPClient.h"
#include "../common/rtspSession.h"

namespace Public
{
namespace RTSP
{

class RTSPClientSession : public RTSPSession
{
private:
	shared_ptr<RTSPClientHandler> handler;
	Mutex mutex;
	std::list<shared_ptr<CommandInfo>> sendcmdlist;

	shared_ptr<Socket> socket;
	uint32_t connecttimeout = 10000;
	uint64_t startconnecttime = Time::getCurrentMilliSecond();

	weak_ptr<RTSPCommandSender> cmdsender;

public:
	//RTSPSession(const shared_ptr<IOWorker>& _worker,const RTSPUrl& _url,const shared_ptr<UDPPortAlloc>& portalloc,const std::string& _useragent,bool server)
	RTSPClientSession(const shared_ptr<RTSPClientHandler> &_handler, const shared_ptr<IOWorker> &_worker, const RTSPUrl &_url, const shared_ptr<UDPPortAlloc> &portalloc, const std::string &_useragent)
		: RTSPSession(_worker, _url, portalloc, _useragent, false), handler(_handler)
	{
	}
	virtual ~RTSPClientSession()
	{
		stop();
	}

	ErrorInfo startConnect(const shared_ptr<RTSPCommandSender> &sender, uint32_t timeout)
	{
		cmdsender = sender;
		connecttimeout = timeout;
		startconnecttime = Time::getCurrentMilliSecond();
		socket = TCPClient::create(ioworker);
		if (socket == NULL) return ErrorInfo(Error_Code_Fail);

		socket->async_connect(NetAddr(rtspurl.serverip, rtspurl.serverport), Socket::ConnectedCallback(&RTSPClientSession::socketconnectcallback, this), timeout);

		return ErrorInfo();
	}

	ErrorInfo stop()
	{
		RTSPSession::stop();

		handler = NULL;
		socket = NULL;
		startconnecttime = connecttimeout = 0;

		return ErrorInfo();
	}
	virtual void onPoolTimerProc()
	{
		uint64_t nowtime = Time::getCurrentMilliSecond();

		if (handler && socket != NULL && nowtime > startconnecttime && nowtime - startconnecttime > connecttimeout)
		{
			shared_ptr<RTSPCommandSender> sender = cmdsender.lock();
			if (sender)
				handler->onConnectResponse(sender, ErrorInfo(Error_Code_ConnectTimeout));
			socket = NULL;
		}

		{
			shared_ptr<CommandInfo> cmdinfo;
			{
				Guard locker(mutex);
				if (sendcmdlist.size() > 0)
				{
					cmdinfo = sendcmdlist.front();
				}

				if (cmdinfo && nowtime > cmdinfo->starttime && nowtime - cmdinfo->starttime > cmdinfo->timeout)
				{
					sendcmdlist.pop_front();
				}
				else
				{
					cmdinfo = NULL;
				}
			}
			if (cmdinfo && handler)
			{
				shared_ptr<RTSPCommandSender> sender = cmdsender.lock();
				if (sender)
					handler->onErrorResponse(sender, cmdinfo->cmd, 408, "Request Timeout");
			}
		}

		RTSPSession::onPoolTimerProc();
	}

private:
	virtual shared_ptr<RTSPHandler> queryRtspHandler()
	{
		return handler;
	}
	shared_ptr<RTSPCommandSender> queryCommandSender()
	{
		return cmdsender.lock();
	}
	void onSendRequestCallback(const shared_ptr<CommandInfo> &cmd)
	{
		Guard locker(mutex);

		sendcmdlist.push_back(cmd);
	}
	virtual shared_ptr<CommandInfo> queryRequestCommand(uint32_t cseq)
	{
		Guard locker(mutex);
		while (sendcmdlist.size())
		{
			shared_ptr<CommandInfo> cmd = sendcmdlist.front();
			if (cmd->cmd->cseq > cseq)
			{
				break;
			}
			sendcmdlist.pop_front();

			if (cmd->cmd->cseq == cseq) return cmd;
		}

		return shared_ptr<CommandInfo>();
	}
	void socketconnectcallback(const weak_ptr<Socket> &, bool status, const std::string &errmsg)
	{
		shared_ptr<RTSPCommandSender> sender = cmdsender.lock();
		if (!status && handler && sender)
		{
			handler->onClose(sender, ErrorInfo(Error_Code_ConnectTimeout, errmsg));
			return;
		}

		//const shared_ptr<Socket>& sock, const CommandCallback& cmdcallback, const ExternDataCallback& datacallback, const DisconnectCallback& disconnectCallback,bool server
		if (handler && sender)
		{
			handler->onConnectResponse(sender, ErrorInfo());
		}
        
        shared_ptr<Socket> tmpSocket = socket;
        if (tmpSocket)
        {
            RTSPSession::start(tmpSocket, NULL, 0);

            sendOptionsRequest();
        }

		socket = NULL;
	}
};
struct RTSPClient::RTSPClientInternal
{
	shared_ptr<RTSPClientSession> session;
};
RTSPClient::RTSPClient(const std::shared_ptr<IOWorker> &work, const shared_ptr<RTSPClientHandler> &handler, const shared_ptr<UDPPortAlloc> &portalloc, const RTSPUrl &rtspUrl, const std::string &useragent)
{
	internal = new RTSPClientInternal;
	internal->session = make_shared<RTSPClientSession>(handler, work, rtspUrl, portalloc, useragent);
}
RTSPClient::~RTSPClient()
{
	stop();
	internal->session = NULL;
	SAFE_DELETE(internal);
}
void RTSPClient::onPoolTimerProc()
{
	shared_ptr<RTSPClientSession> session = internal->session;
	if (session)
		session->onPoolTimerProc();
}
bool RTSPClient::initRTPOverTcpType()
{
	internal->session->transportbytcp = true;

	return true;
}
bool RTSPClient::initRTPOverUdpType()
{
	internal->session->transportbytcp = false;

	return true;
}
ErrorInfo RTSPClient::start(uint32_t timeout)
{
	return internal->session->startConnect(shared_from_this(), timeout);
}

ErrorInfo RTSPClient::stop()
{
	return internal->session->stop();
}

shared_ptr<RTSPCommandInfo> RTSPClient::sendPlayRequest(const PlayInfo &range)
{
	shared_ptr<RTSPSession::CommandInfo> cmdinfo = internal->session->sendPlayRequest(range);
	if (cmdinfo == NULL)
		return shared_ptr<RTSPCommandInfo>();

	return cmdinfo->cmd;
}

ErrorInfo RTSPClient::sendPlayRequest(const PlayInfo &range, uint32_t timeout)
{
	shared_ptr<RTSPSession::CommandInfo> cmdinfo = internal->session->sendPlayRequest(range, timeout);
	if (!cmdinfo->waitsem->pend(timeout) || cmdinfo->responseheader == NULL)
	{
		return ErrorInfo(Error_Code_CommunicateTimeout);
	}

	return cmdinfo->responseheader->statuscode == 200 ? ErrorInfo() : ErrorInfo(Error_Code_Fail);
}

shared_ptr<RTSPCommandInfo> RTSPClient::sendPauseRequest()
{
	auto cmdinfo = internal->session->sendPauseRequest();
	if (cmdinfo == nullptr)
	{
		return {};
	}
	return cmdinfo->cmd;
}
ErrorInfo RTSPClient::sendPauseRequest(uint32_t timeout)
{
	shared_ptr<RTSPSession::CommandInfo> cmdinfo = internal->session->sendPauseRequest(timeout);
	if (!cmdinfo->waitsem->pend(timeout) || cmdinfo->responseheader == NULL)
	{
		return ErrorInfo(Error_Code_CommunicateTimeout);
	}

	return cmdinfo->responseheader->statuscode == 200 ? ErrorInfo() : ErrorInfo(Error_Code_Fail);
}

shared_ptr<RTSPCommandInfo> RTSPClient::sendGetparameterRequest(const std::string &body)
{
	auto cmdinfo = internal->session->sendGetparameterRequest(body);
	if (cmdinfo == nullptr)
	{
		return {};
	}
	return cmdinfo->cmd;
}
ErrorInfo RTSPClient::sendGetparameterRequest(const std::string &body, uint32_t timeout)
{
	shared_ptr<RTSPSession::CommandInfo> cmdinfo = internal->session->sendGetparameterRequest(body, timeout);
	if (!cmdinfo->waitsem->pend(timeout) || cmdinfo->responseheader == NULL)
	{
		return ErrorInfo(Error_Code_CommunicateTimeout);
	}

	return cmdinfo->responseheader->statuscode == 200 ? ErrorInfo() : ErrorInfo(Error_Code_Fail);
}
shared_ptr<RTSPCommandInfo> RTSPClient::sendTeradownRequest()
{
	auto cmdinfo = internal->session->sendTeardownRequest();
	if (cmdinfo == nullptr)
	{
		return {};
	}
	return cmdinfo->cmd;
}
ErrorInfo RTSPClient::sendTeradownRequest(uint32_t timeout)
{
	shared_ptr<RTSPSession::CommandInfo> cmdinfo = internal->session->sendTeardownRequest(timeout);
	if (!cmdinfo->waitsem->pend(timeout) || cmdinfo->responseheader == NULL)
	{
		return ErrorInfo(Error_Code_CommunicateTimeout);
	}

	return cmdinfo->responseheader->statuscode == 200 ? ErrorInfo() : ErrorInfo(Error_Code_Fail);
}

ErrorInfo RTSPClient::sendRTPFrame(const shared_ptr<STREAM_TRANS_INFO> &transport, const shared_ptr<RTPFrame> &rtpframe)
{
	shared_ptr<RTSPMediaSessionInfo> mediasession = internal->session->rtspmedia;
	if (mediasession == NULL)
		return ErrorInfo(Error_Code_Param);

	shared_ptr<MediaSession> rtpsession = mediasession->session(transport);

	if (transport == NULL || rtpsession == NULL)
		return ErrorInfo(Error_Code_Param);

	rtpsession->sendMediaFrameData(transport, rtpframe);

	return ErrorInfo();
}
ErrorInfo RTSPClient::sendRTCPPackage(const shared_ptr<STREAM_TRANS_INFO> &transport, const shared_ptr<RTCPPackage> &rtcppackage)
{
	shared_ptr<RTSPMediaSessionInfo> mediasession = internal->session->rtspmedia;
	if (mediasession == NULL)
		return ErrorInfo(Error_Code_Param);

	shared_ptr<MediaSession> rtpsession = mediasession->session(transport);

	if (transport == NULL || rtpsession == NULL)
		return ErrorInfo(Error_Code_Param);

	rtpsession->sendContorlData(transport, rtcppackage);

	return ErrorInfo();
}

} // namespace RTSP
} // namespace Public