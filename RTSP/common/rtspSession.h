#pragma once
#include "RTSP/RTSPClient.h"
#include "rtspProtocol.h"
#include "StringParse/rtspHeaderSdp.h"
#include "StringParse/rtspHeaderTransport.h"
#include "mediaSessionOverTcp.h"
#include "mediaSessionOverUdp.h"
#include "udpPortAlloc.h"

using namespace Public::RTSP;

#define OPTIONCMDSTR "DESCRIBE,SETUP,TEARDOWN,PLAY,PAUSE,OPTIONS,ANNOUNCE,RECORD,GET_PARAMETER"

class RTSPSession:public RTSPProtocol,public RTPSender,public enable_shared_from_this<RTPSender>
{
public:
	struct CommandInfo
	{
		shared_ptr<RTSPCommandInfo> cmd;
		uint64_t					starttime;
		uint64_t					timeout;

		std::string					wwwauthen;

		shared_ptr<Semaphore>		waitsem;
		shared_ptr<RTSPCommandInfo> responseheader;

		CommandInfo(uint32_t _timeout = 10000) :starttime(Time::getCurrentMilliSecond()), timeout(_timeout)
		{
			cmd = make_shared<RTSPCommandInfo>();
			if (_timeout != (uint32_t)-1) waitsem = make_shared<Semaphore>();
		}
	};
protected:
	shared_ptr<IOWorker>				ioworker;
private:
	std::string							useragent;

	std::string							sessionstr;	
	shared_ptr<UDPPortAlloc>			udpportalloc;

	uint32_t							protocolstartcseq = 0;

	size_t								ssrc = (size_t)this;

	bool								isserver = false;

	bool								havdBuildAuthen = false;
	bool								sessionHaveAuthen = false;

	uint32_t							tcpinterval = 0;

	PlayInfo							playinfo;
public:
	RTSPUrl								rtspurl;
	bool								transportbytcp = false;
	shared_ptr<RTSPMediaSessionInfo>	rtspmedia;
public:
	RTSPSession(const shared_ptr<IOWorker>& _worker,const RTSPUrl& _url,const shared_ptr<UDPPortAlloc>& portalloc,const std::string& _useragent,bool server):RTSPProtocol(server),
		ioworker(_worker),useragent(_useragent), udpportalloc(portalloc),isserver(server), rtspurl(_url)
	{
		protocolstartcseq = 0;
		transportbytcp = false;
		ssrc = (size_t)this;
	}
	~RTSPSession()
	{
		stop();
	}
	virtual ErrorInfo stop()
	{
       /* shared_ptr<RTSPMediaSessionInfo> tmpRtspMedia = rtspmedia;
        if (tmpRtspMedia)
        {
            tmpRtspMedia->stop();
        }*/
		ioworker = NULL;
		rtspmedia = NULL;

		return RTSPProtocol::stop();
	}
	virtual bool sessionIsTimeout()
	{
		if (protocolIsTimeout()) return true;

		shared_ptr<RTSPMediaSessionInfo> mediatmp = rtspmedia;
		if (mediatmp)
		{
			std::list< shared_ptr<STREAM_TRANS_INFO> > infolist = mediatmp->infos;
			for (std::list< shared_ptr<STREAM_TRANS_INFO> >::iterator iter = infolist.begin(); iter != infolist.end(); iter++)
			{
				shared_ptr<MediaSession> session = (*iter)->mediasession.lock();
				if (session && session->sessionIsTimeout()) return true;
			}
		}

		return false;
	}
	virtual void onPoolTimerProc()
	{
		shared_ptr<RTSPMediaSessionInfo> mediatmp = rtspmedia;
		if (mediatmp)
		{
			std::list< shared_ptr<STREAM_TRANS_INFO> > infolist = mediatmp->infos;
			for (std::list< shared_ptr<STREAM_TRANS_INFO> >::iterator iter = infolist.begin(); iter != infolist.end(); iter++)
			{
				shared_ptr<MediaSession> session = (*iter)->mediasession.lock();
				if (session) session->onPoolTimerProc();

				shared_ptr<RTSPStatistics> statis = (*iter)->rtspstatistics.lock();
				if (statis) statis->onPoolTimerProc();
			}
		}

	}
	virtual void setAutoTimeStampEnable(const shared_ptr<STREAM_TRANS_INFO>& transport, bool enable)
	{
		shared_ptr<RTSPMediaSessionInfo> mediatmp = rtspmedia;
		if (mediatmp == NULL)
		{
			return;
		}

		shared_ptr<MediaSession> mediasession = mediatmp->session(transport);
		if (mediasession == NULL)
		{
			return;
		}

		mediasession->setAutoTimeStampEnable(enable);
	}

	shared_ptr<CommandInfo> sendOptionsRequest()
	{
		shared_ptr<CommandInfo> cmdinfo = make_shared<CommandInfo>();
		cmdinfo->cmd->method = "OPTIONS";
		cmdinfo->cmd->url = rtspurl.rtspurl;

		sendRequest(cmdinfo);

		return cmdinfo;
	}
	void sendOptionResponse(const shared_ptr<RTSPCommandInfo>& cmdinfo, const std::string& cmdstr)
	{
		std::string cmdstrtmp = cmdstr;
		if (cmdstrtmp.length() == 0) cmdstrtmp = OPTIONCMDSTR;

		HTTP::Header header;
		header.headers["Public"] = cmdstrtmp;

		sendResponse(cmdinfo, header);
	}
	shared_ptr<CommandInfo> sendPlayRequest(const PlayInfo& range, uint32_t timeout = -1)
	{
		playinfo = range;

		if (rtspmedia == NULL || rtspmedia->sessionsize() <= 0)
		{
			return shared_ptr<CommandInfo>();
		}

		shared_ptr<CommandInfo> cmdinfo = make_shared<CommandInfo>(timeout);
		cmdinfo->cmd->method = "PLAY";
		cmdinfo->cmd->url = rtspurl.rtspurl;

		if (range.haveRang())
		{
			cmdinfo->cmd->headers["Range"] = range.buildRang();
		}

		if (range.haveSpeed())
		{
			cmdinfo->cmd->headers["Speed"] = range.buildSpeed();
		}

		sendRequest(cmdinfo);

		return cmdinfo; 
	}
	void sendPlayResponse(const shared_ptr<RTSPCommandInfo>& cmdinfo)
	{
		sendResponse(cmdinfo, HTTP::Header());
	}
	shared_ptr<CommandInfo> sendPauseRequest(uint32_t timeout = -1)
	{
		if (rtspmedia == NULL || rtspmedia->sessionsize() <= 0)
		{
			return shared_ptr<CommandInfo>();
		}

		shared_ptr<CommandInfo> cmdinfo = make_shared<CommandInfo>(timeout);
		cmdinfo->cmd->method = "PAUSE";
		cmdinfo->cmd->url = rtspurl.rtspurl;

		sendRequest(cmdinfo);

		return cmdinfo;
	}
	void sendPauseResponse(const shared_ptr<RTSPCommandInfo>& cmdinfo)
	{
		sendResponse(cmdinfo, HTTP::Header());
	}
	shared_ptr<CommandInfo> sendGetparameterRequest(const std::string& body, uint32_t timeout = -1)
	{
		shared_ptr<CommandInfo> cmdinfo = make_shared<CommandInfo>(timeout);
		cmdinfo->cmd->method = "GET_PARAMETER";
		cmdinfo->cmd->url = rtspurl.rtspurl;

		sendRequest(cmdinfo, body);

		return cmdinfo;
	}
	void sendGetparameterResponse(const shared_ptr<RTSPCommandInfo>& cmdinfo, const std::string& content)
	{
		sendResponse(cmdinfo, HTTP::Header(), content);
	}
	shared_ptr<CommandInfo> sendTeardownRequest(uint32_t timeout = -1)
	{
		shared_ptr<CommandInfo> cmdinfo = make_shared<CommandInfo>(timeout);
		cmdinfo->cmd->method = "TERADOWN";
		cmdinfo->cmd->url = rtspurl.rtspurl;

		sendRequest(cmdinfo);

		return cmdinfo;
	}
	void sendTeardownResponse(const shared_ptr<RTSPCommandInfo>& cmdinfo)
	{
		sendResponse(cmdinfo, HTTP::Header());
	}
	shared_ptr<CommandInfo> sendDescribeRequest()
	{
		if (rtspmedia != NULL && rtspmedia->infos.size() > 0)
		{
			return shared_ptr<CommandInfo>();
		}

		shared_ptr<CommandInfo> cmdinfo = make_shared<CommandInfo>();
		cmdinfo->cmd->method = "DESCRIBE";
		cmdinfo->cmd->url = rtspurl.rtspurl;

		sendRequest(cmdinfo);

		return cmdinfo;
	}
	void sendDescribeResponse(const shared_ptr<RTSPCommandInfo>& cmdinfo, const RTSP_Media_Infos& mediainfo)
	{
		RTSP_Media_Infos mediatmp = mediainfo.cloneStreamInfo();
		rtspmedia = make_shared<RTSPMediaSessionInfo>(mediatmp);
		if(rtspmedia->ssrc.length() <= 0)
			rtspmedia->ssrc = Value(ssrc).readString();

		sendResponse(cmdinfo, HTTP::Header(), rtsp_header_build_sdp(*rtspmedia.get()), RTSPCONENTTYPESDP);
	}
	shared_ptr<CommandInfo> sendSetupRequest(const shared_ptr<STREAM_TRANS_INFO>& transport)
	{
		if (rtspmedia == NULL || rtspmedia->querySession(transport) != NULL)
		{
			return shared_ptr<CommandInfo>();
		}

		if (transport->transportinfo.ssrc == 0)
			transport->transportinfo.ssrc = (int)(ssrc | Time::getCurrentMilliSecond());

		shared_ptr<CommandInfo> cmdinfo = make_shared<CommandInfo>();
		cmdinfo->cmd->method = "SETUP";
		cmdinfo->cmd->url = rtspurl.rtspurl + "/" + transport->streaminfo.trackID;
		cmdinfo->cmd->headers["Transport"] = rtsp_header_build_transport(transport->transportinfo);

		sendRequest(cmdinfo);

		return cmdinfo;
	}
	void sendSetupResponse(const shared_ptr<RTSPCommandInfo>& cmdinfo, const shared_ptr<STREAM_TRANS_INFO>& transporttmp)
	{
		if(transporttmp->transportinfo.ssrc == 0)
			transporttmp->transportinfo.ssrc = (int)(ssrc | Time::getCurrentMilliSecond());
		
		if (transporttmp->transportinfo.transport == TRANSPORT_INFO::TRANSPORT_RTP_UDP)
		{
			uint32_t startport = udpportalloc->allockRTPPort();
			transporttmp->transportinfo.rtp.u.server_port1 = startport;
			transporttmp->transportinfo.rtp.u.server_port2 = startport + 1;
		}

		HTTP::Header header;
		header.headers["Transport"] = rtsp_header_build_transport(transporttmp->transportinfo);
		sendResponse(cmdinfo, header);
	}
	void sendErrorResponse(const shared_ptr<RTSPCommandInfo>& cmdinfo, int errcode, const std::string& errmsg)
	{
		HTTP::Header header;
		header.statuscode = errcode;
		header.statusmsg = errmsg;
		if (errcode == 401)
		{
			header.headers["WWW-Authenticate"] = WWW_Authenticate::buildWWWAuthenticate(cmdinfo->method, rtspurl.username, rtspurl.password,WWW_Authenticate::Authenticate_Type_Digest);
		}

		sendResponse(cmdinfo, header);
	}
	void sendResponse(const shared_ptr<RTSPCommandInfo>& cmdinfo,const HTTP::Header& header, const std::string& body = "", const std::string& contentype = "")
	{
		shared_ptr<RTSPCommandInfo> respcmd = make_shared<RTSPCommandInfo>();
		respcmd->statuscode = header.statuscode;
		respcmd->statusmsg = header.statusmsg;
		respcmd->headers = header.headers;
		respcmd->cseq = cmdinfo->cseq;

		_rtspBuildRtspCommand(respcmd, body, contentype);
	}

	void sendRequest(const shared_ptr<CommandInfo>& cmdinfo, const std::string& body = "", const std::string& contentype = "")
	{
		onSendRequestCallback(cmdinfo);

		if(cmdinfo->cmd->url.length() <= 0)	cmdinfo->cmd->url = rtspurl.rtspurl;
		if (cmdinfo->wwwauthen.length() > 0)
		{
			cmdinfo->cmd->headers["Authorization"] = WWW_Authenticate::buildAuthorization(cmdinfo->cmd->method, rtspurl.username, rtspurl.password, cmdinfo->cmd->url, cmdinfo->wwwauthen);
		}
		_rtspBuildRtspCommand(cmdinfo->cmd, body, contentype);
	}
protected:
	//查询客户端的请求命令
	virtual void onSendRequestCallback(const shared_ptr<CommandInfo>& cmd) {}
	virtual shared_ptr<CommandInfo> queryRequestCommand(uint32_t cseq) {return shared_ptr<CommandInfo>();}
	virtual shared_ptr<RTSPHandler> queryRtspHandler() = 0;
	virtual shared_ptr<RTSPCommandSender> queryCommandSender() = 0;
private:
	void onRecvCommandCallback(const shared_ptr<RTSPCommandInfo>& cmd)
	{
		if (!isserver) doClientCommandCallback(cmd);
		else doServerCommandCallback(cmd);
	}

	void doClientCommandCallback(const shared_ptr<RTSPCommandInfo>& cmdheader)
	{
		shared_ptr<CommandInfo> cmdinfo = queryRequestCommand(cmdheader->cseq);
		shared_ptr<RTSPHandler> handler = queryRtspHandler();
		shared_ptr<RTSPCommandSender> sender = queryCommandSender();

		if (cmdinfo == NULL || handler == NULL || sender == NULL) return;

		if (cmdheader->statuscode == 401)
		{
			if (rtspurl.username == "" || rtspurl.password == "")
			{
				handler->onErrorResponse(sender,cmdinfo->cmd, 401, "no authenticate info");
				handler->onClose(sender, ErrorInfo(Error_Code_Authen,"no authenticate info"));
			}
			else if (havdBuildAuthen)
			{
				handler->onClose(sender, ErrorInfo(Error_Code_Authen));
			}
			else
			{
				std::string wwwauthen = cmdheader->header("WWW-Authenticate").readString();
				cmdinfo->wwwauthen = wwwauthen;

				sendRequest(cmdinfo);

				havdBuildAuthen = true;
			}
		}
		else if (cmdheader->statuscode != 200)
		{
			handler->onErrorResponse(sender, cmdinfo->cmd, cmdheader->statuscode,cmdheader->statusmsg);
		}
		else if (String::strcasecmp(cmdinfo->cmd->method.c_str(), "OPTIONS") == 0)
		{
			//没建立会话
			if (rtspmedia == NULL)
			{
				sendDescribeRequest();
			}
		}
		else if (String::strcasecmp(cmdinfo->cmd->method.c_str(), "DESCRIBE") == 0)
		{
            shared_ptr<RTSPMediaSessionInfo> tmpRtspMedia = make_shared<RTSPMediaSessionInfo>();

			rtsp_header_parse_sdp(cmdheader->body.c_str(), tmpRtspMedia.get());

			handler->onDescribeResponse(sender, cmdinfo->cmd, tmpRtspMedia);

            rtspmedia = tmpRtspMedia;

			shared_ptr<STREAM_TRANS_INFO> transportinfo = checkAndBuildNotSetupTransport();
			if (transportinfo)
			{
				sendSetupRequest(transportinfo);
			}
		}
		else if (String::strcasecmp(cmdinfo->cmd->method.c_str(), "SETUP") == 0)
		{
			if (sessionstr.length() <= 0)
			{
				const std::string sessionstrtmp = cmdheader->header("Session").readString();
				const char* sessionptraddr = strchr(sessionstrtmp.c_str(), ';');
				if (sessionptraddr != NULL) sessionstr = std::string(sessionstrtmp.c_str(), sessionptraddr - sessionstrtmp.c_str());
				else sessionstr = sessionstrtmp;
			}
			std::string tranportstr = cmdheader->header("Transport").readString();

			//检查接收回来的setup回复
			checkRecvSetupResponse(cmdinfo, tranportstr);

			//发送失败说明setup已经发送全部
			shared_ptr<STREAM_TRANS_INFO> transportinfo = checkAndBuildNotSetupTransport();
			if (transportinfo)
			{
				sendSetupRequest(transportinfo);
			}
			else
			{
				buildRtpSession(false);

				sendPlayRequest(playinfo);
			}
		}
		else if (String::strcasecmp(cmdinfo->cmd->method.c_str(), "PLAY") == 0)
		{
			if (cmdinfo->waitsem == NULL)
				handler->onPlayResponse(sender, cmdinfo->cmd);
		}
		else if (String::strcasecmp(cmdinfo->cmd->method.c_str(), "PAUSE") == 0)
		{
			if (cmdinfo->waitsem == NULL)
				handler->onPauseResponse(sender, cmdinfo->cmd, ErrorInfo(cmdheader->statuscode == 200 ? Error_Code_Success : Error_Code_Fail, cmdheader->statusmsg));
		}
		else if (String::strcasecmp(cmdinfo->cmd->method.c_str(), "GET_PARAMETER") == 0)
		{
			if (cmdinfo->waitsem == NULL)
				handler->onGetparameterResponse(sender, cmdinfo->cmd, cmdheader->body);
		}
		else if (String::strcasecmp(cmdinfo->cmd->method.c_str(), "TERADOWN") == 0)
		{
			if (cmdinfo->waitsem == NULL)
				handler->onTeradownResponse(sender, cmdinfo->cmd);
		}

		if (cmdinfo->waitsem != NULL)
		{
			cmdinfo->responseheader = cmdheader;
			cmdinfo->waitsem->post();
		}
	}

	void doServerCommandCallback(const shared_ptr<RTSPCommandInfo>& cmdinfo)
	{
		if (cmdinfo->method == "" || cmdinfo->header("CSeq").empty())
		{
			return sendErrorResponse(cmdinfo, 400, "Bad Request");
		}
		if (String::strcasecmp(cmdinfo->verinfo.protocol.c_str(), "rtsp") != 0 || cmdinfo->verinfo.version != "1.0")
		{
			return sendErrorResponse(cmdinfo, 505, "RTSP Version Not Supported");
		}

		if (rtspurl.rtspurl == "") rtspurl = cmdinfo->url;

		shared_ptr<RTSPHandler> handler = queryRtspHandler();
		shared_ptr<RTSPCommandSender> sender = queryCommandSender();
		if (handler == NULL || sender == NULL) 
		{
			sendErrorResponse(cmdinfo, 500, "NOT SUPPORT");
			return;
		}

		std::string nowsession = cmdinfo->header("Session").readString();
		if (sessionstr.length() > 0 && sessionstr != nowsession)
		{
			sendErrorResponse(cmdinfo, 451, "Session Error");
			return;
		}

		//check authen info
		if (rtspurl.username != "" || rtspurl.password != "")
		{
			std::string authenstr = cmdinfo->header("Authorization").readString();
			if (authenstr.length() <= 0 && !sessionHaveAuthen)
			{
				return sendErrorResponse(cmdinfo, 401, "No Authorization");
			}
			else if (authenstr.length() > 0)
			{
				rtspurl.username = WWW_Authenticate::getAuthorizationUsreName(authenstr);
				ErrorInfo ret = handler->queryUserPassword(sender,rtspurl.username, rtspurl.password);
				if (ret)
				{
					return sendErrorResponse(cmdinfo, 403, ret.errorMessage());
				}

				if (!WWW_Authenticate::checkAuthenticate(cmdinfo->method, rtspurl.username, rtspurl.password, authenstr))
				{
					return sendErrorResponse(cmdinfo, 403, "Forbidden");
				}
				sessionHaveAuthen = true;
			}
		}

		if (String::strcasecmp(cmdinfo->method.c_str(), "OPTIONS") == 0)
		{
			handler->onOptionRequest(sender,cmdinfo);
		}
		else if (String::strcasecmp(cmdinfo->method.c_str(), "DESCRIBE") == 0)
		{
			handler->onDescribeRequest(sender, cmdinfo);
		}
		else if (String::strcasecmp(cmdinfo->method.c_str(), "SETUP") == 0)
		{

			if (sessionstr.length() <= 0)
			{
				sessionstr = Value(Time::getCurrentMilliSecond()).readString() + Value(Time::getCurrentTime().makeTime()).readString();
			}

			std::string tranportstr = cmdinfo->header("Transport").readString();

			TRANSPORT_INFO transport;
			rtsp_header_parse_transport(tranportstr.c_str(), &transport);

			shared_ptr<STREAM_TRANS_INFO> transportinfo;
			if (rtspmedia)
			{
				for (std::list<shared_ptr<STREAM_TRANS_INFO> >::const_iterator iter = rtspmedia->infos.begin(); iter != rtspmedia->infos.end(); iter++)
				{
					if (String::indexOfByCase(cmdinfo->url, (*iter)->streaminfo.trackID) != (size_t)-1)
					{
						transportinfo = *iter;
						break;
					}
				}
			}

			if (transportinfo == NULL)
			{
				sendErrorResponse(cmdinfo, 501, "NO THIS TRACK");
			}
			else
			{
				transportinfo->transportinfo = transport;

				handler->onSetupRequest(sender, cmdinfo, transportinfo);
			}
		}
		else if (String::strcasecmp(cmdinfo->method.c_str(), "PLAY") == 0)
		{
			std::string rangestr = cmdinfo->header("Range").readString();
			std::string speedstr = cmdinfo->header("Speed").readString();
			PlayInfo range;

			if (rangestr.length() > 0)	range.parseRang(rangestr);
			if (speedstr.length() > 0) range.parseSpeed(speedstr);

            buildRtpSession(true);

			handler->onPlayRequest(sender, cmdinfo, rtspmedia, range);
		}
		else if (String::strcasecmp(cmdinfo->method.c_str(), "PAUSE") == 0)
		{
			handler->onPauseRequest(sender, cmdinfo);
		}
		else if (String::strcasecmp(cmdinfo->method.c_str(), "GET_PARAMETER") == 0)
		{
			handler->onGetparameterRequest(sender, cmdinfo, cmdinfo->body);
		}
		else if (String::strcasecmp(cmdinfo->method.c_str(), "TEARDOWN") == 0)
		{
			handler->onTeardownRequest(sender, cmdinfo);
			//RTSPProtocol::stop();
		}
		else
		{
			sendErrorResponse(cmdinfo, 500, "NOT SUPPORT");
		}
	}
	void checkRecvSetupResponse(const shared_ptr<CommandInfo>& cmdinfo, const std::string& tranportstr)
	{
        shared_ptr<RTSPMediaSessionInfo> mediatmp = rtspmedia;
        if (mediatmp == NULL)
        {
            return;
        }

		TRANSPORT_INFO transport;
		rtsp_header_parse_transport(tranportstr.c_str(), &transport);

		if (transport.transport == TRANSPORT_INFO::TRANSPORT_NONE) return;

		shared_ptr<STREAM_TRANS_INFO> transportinfo;


		for (std::list<shared_ptr<STREAM_TRANS_INFO> >::const_iterator iter = mediatmp->infos.begin(); iter != mediatmp->infos.end(); iter++)
		{
			if (String::indexOfByCase(cmdinfo->cmd->url, (*iter)->streaminfo.trackID) != (size_t)-1)
			{
				transportinfo = *iter;

				transportinfo->transportinfo = transport;

				break;
			}
		}

		shared_ptr<RTSPHandler> handler = queryRtspHandler();
		shared_ptr<RTSPCommandSender> sender = queryCommandSender();
		if (transportinfo && handler && sender)
			handler->onSetupResponse(sender, cmdinfo->cmd, transportinfo);
	}
	shared_ptr<STREAM_TRANS_INFO> checkAndBuildNotSetupTransport()
	{
        shared_ptr<RTSPMediaSessionInfo> mediatmp = rtspmedia;
        if (mediatmp == NULL)
        {
            return shared_ptr<STREAM_TRANS_INFO>();
        }

		shared_ptr<STREAM_TRANS_INFO> transportinfo;

		for (std::list<shared_ptr<STREAM_TRANS_INFO> >::const_iterator iter = mediatmp->infos.begin(); iter != mediatmp->infos.end(); iter++)
		{
			if ((*iter)->transportinfo.transport == TRANSPORT_INFO::TRANSPORT_NONE)
			{
				transportinfo = *iter;
				break;
			}
		}

		if (!transportinfo) return shared_ptr<STREAM_TRANS_INFO>();

		if (transportbytcp)
		{
			transportinfo->transportinfo.transport = TRANSPORT_INFO::TRANSPORT_RTP_TCP;
			transportinfo->transportinfo.rtp.t.dataChannel = tcpinterval;
			transportinfo->transportinfo.rtp.t.contorlChannel = tcpinterval + 1;

			tcpinterval += 2;
		}
		else
		{
			uint32_t startport = udpportalloc->allockRTPPort();

			transportinfo->transportinfo.transport = TRANSPORT_INFO::TRANSPORT_RTP_UDP;
			transportinfo->transportinfo.rtp.u.client_port1 = startport;
			transportinfo->transportinfo.rtp.u.client_port2 = startport + 1;
		}

		return transportinfo;
	}
	void buildRtpSession(bool isserver)
	{
        shared_ptr<RTSPMediaSessionInfo> mediatmp = rtspmedia;
        shared_ptr<IOWorker> tmpIOWorker = ioworker;
        if (mediatmp == NULL || tmpIOWorker == NULL)
        {
            return;
        }

		for (std::list< shared_ptr<STREAM_TRANS_INFO> >::iterator iter = mediatmp->infos.begin(); iter != mediatmp->infos.end(); iter++)
		{
			shared_ptr<STREAM_TRANS_INFO> transport = *iter;
			shared_ptr<MediaSession> session = mediatmp->session(transport);

			if (session) break;

			shared_ptr<RTSPHandler> handler = queryRtspHandler();
			shared_ptr<RTSPCommandSender> cmdsender = queryCommandSender();
			if (handler == NULL || cmdsender == NULL) break;

			if (transport->transportinfo.transport == TRANSPORT_INFO::TRANSPORT_RTP_TCP)
			{
				//MediaSessionOverTcp(bool _isserver, const shared_ptr<STREAM_TRANS_INFO>& _transport, const shared_ptr<RTSPHandler>& _mediahandler,const shared_ptr<RTPSender>& _sender)
				session = make_shared<MediaSessionOverTcp>(isserver,transport, handler, cmdsender,shared_from_this());
			}
			else if (transport->transportinfo.transport == TRANSPORT_INFO::TRANSPORT_RTP_UDP)
			{
				std::string dstaddr;
				if (isserver)
				{
					dstaddr = getOtherAddr().getIP();
				}
				else
				{
					dstaddr = rtspurl.serverip;
				}

				//MediaSessionOverUdp(bool _isserver, const shared_ptr<IOWorker>& ioworker,const std::string& _dstaddr, const shared_ptr<STREAM_TRANS_INFO>& _transport, const shared_ptr<RTSPHandler>& handler)
				session = make_shared<MediaSessionOverUdp>(isserver, tmpIOWorker, dstaddr,transport, handler, cmdsender);
			}
			else
			{
				continue;
			}
			session->start(useragent, MediaSession::StreamTimeoutCallback(&RTSPSession::onStreamTimeOutCallback, this));
            mediatmp->setSession(transport, session);
		}
	}
private:
	void _rtspBuildRtspCommand(const shared_ptr<RTSPCommandInfo>& cmd, const std::string& body = "", const std::string& contentype = "")
	{
		if (body.length() > 0)
		{
			cmd->headers[Content_Length] = body.length();
		}
		if (body.length() > 0 && contentype.length() > 0)
		{
			cmd->headers[Content_Type] = contentype;
		}
		if (useragent.length() > 0) cmd->headers["User-Agent"] = useragent;
		if (sessionstr.length() > 0) cmd->headers["Session"] = sessionstr;

		if (cmd->cseq == 0)
		{
			do
			{
				protocolstartcseq++;
			} while (protocolstartcseq == 0);
			cmd->cseq = protocolstartcseq;
		}

		cmd->headers["CSeq"] = cmd->cseq;	

		if(cmd->url.length() <= 0) cmd->url = rtspurl.rtspurl;

		std::string cmdstr = HTTP::Builder::build(!isserver,*(HTTP::Header*)cmd.get());
		if (body.length() > 0) cmdstr += body;


		shared_ptr<SendPackgeInfo> sendinfo = make_shared<SendPackgeInfo>(cmdstr, SendPackgeInfo::packgeType_RtspCommand);
		sendProtocolData({ sendinfo });
	}
private:
	uint32_t prevsn = 0;
	virtual void onRecvMediaCallback(const String& data, const INTERLEAVEDFRAME& frame, uint32_t offset, uint32_t bufferlen)
	{
		shared_ptr<RTSPMediaSessionInfo> mediatmp = rtspmedia;

		if (mediatmp)
		{
			for (std::list<shared_ptr<STREAM_TRANS_INFO> >::iterator iter = mediatmp->infos.begin(); iter != mediatmp->infos.end(); iter++)
			{
				shared_ptr<STREAM_TRANS_INFO> transportinfo = *iter;
				if (transportinfo->transportinfo.transport != TRANSPORT_INFO::TRANSPORT_RTP_TCP) continue;
				shared_ptr<MediaSession> rtpsession = mediatmp->session(transportinfo);
				if (rtpsession == NULL) continue;

				if (frame.channel == transportinfo->transportinfo.rtp.t.dataChannel)
				{
					shared_ptr<RTPPackage> rtppackage = make_shared<RTPPackage>(data, offset, (uint32_t)bufferlen);

					const RTPHEADER& rtpheader = rtppackage->rtpHeader();
					if (rtpheader.v != RTP_VERSION)
					{
						//	assert(0);
						break;
					}
					uint32_t sn = ntohs(rtpheader.seq);
					if (prevsn != 0 && prevsn + 1 != sn)
					{
					//	printf("eeeeeeee %p offset %d %d %d %d %d %d\r\n", data.c_str(), offset, frame.channel, prevsn, sn, bufferlen, prevsn + 1 == sn);
						
					//	assert(0);
					}
				//	printf("%p offset %d %d %d %d %d %d\r\n", data.c_str(),offset, frame.channel, prevsn, sn,bufferlen,prevsn + 1 == sn);
					prevsn = sn;

					rtpsession->rtpovertcpMediaCallback(transportinfo, rtppackage);
					break;
				}
				else if (frame.channel == transportinfo->transportinfo.rtp.t.contorlChannel)
				{
					shared_ptr<RTCPPackage> rtcppackage = make_shared<RTCPPackage>();
					if (rtcppackage->parse(data.c_str() + offset, bufferlen))
					{
						rtpsession->rtpovertcpContorlCallback(transportinfo, rtcppackage);
					}
					break;
				}
			}
		}
	}
	virtual void onNetworkErrorCallback()
	{
		shared_ptr<RTSPHandler> handler = queryRtspHandler();
		shared_ptr<RTSPCommandSender> cmdsender = queryCommandSender();
		if (handler && cmdsender) handler->onClose(cmdsender,ErrorInfo(Error_Code_Network));
	}

	virtual void onStreamTimeOutCallback()
	{
		shared_ptr<RTSPHandler> handler = queryRtspHandler();
		shared_ptr<RTSPCommandSender> cmdsender = queryCommandSender();
		if (handler && cmdsender) handler->onConnectResponse(cmdsender, ErrorInfo(Error_Code_ConnectTimeout));
	}

	virtual void sendContorlData(const shared_ptr<STREAM_TRANS_INFO>& transport, const shared_ptr <RTCPPackage>& rtcppackage)
	{
		if (rtcppackage == NULL) return;

		std::string rtcpstr = rtcppackage->toString();

		INTERLEAVEDFRAME frame;
		frame.magic = RTPOVERTCPMAGIC;
		frame.channel = transport->transportinfo.rtp.t.contorlChannel;
		frame.rtp_len = htons((u_short)rtcpstr.length());

		std::string senddata(std::string((const char*)&frame, sizeof(INTERLEAVEDFRAME)) + rtcpstr);

		sendProtocolData({ make_shared<SendPackgeInfo>(senddata, SendPackgeInfo::PackgeType_RtcpData) });
	}

	virtual void sendMediaData(const shared_ptr<STREAM_TRANS_INFO>& transport, const std::vector<shared_ptr<RTPPackage>>& rtppackage)
	{
		std::list<shared_ptr<SendPackgeInfo>> sendlist;

		for (size_t i = 0; i < rtppackage.size(); i++)
		{
			if(rtppackage[i])
			{
				INTERLEAVEDFRAME frame;
				frame.magic = RTPOVERTCPMAGIC;
				frame.channel = transport->transportinfo.rtp.t.dataChannel;
				frame.rtp_len = htons((uint16_t)(rtppackage[i]->bufferlen()));

				sendlist.push_back(make_shared<SendPackgeInfo>(String((const char*)&frame, sizeof(INTERLEAVEDFRAME))));
			}

			sendlist.push_back(make_shared<SendPackgeInfo>(rtppackage[i]));
		}

		sendProtocolData(sendlist);
	}
};
