#pragma once
#include "HTTP/HTTPParse.h"
#include "rtp/rtpSession.h"
using namespace Public::HTTP;
using namespace Public::RTSP;

#define MAXPARSERTSPBUFFERLEN	1024*1024
#define RTPOVERTCPMAGIC		'$'
#define RTSPCMDFLAG			"RTSP/1.0"

#define RTSPCONENTTYPESDP	"application/sdp"

#define MAXRTPPACKGESIZE	1024*56

class RTSPProtocol:public HTTPParse
{
public:
	struct TCPInterleaved
	{
		int dataChannel;
		int contrlChannel;
	};
public:
	typedef Function1<void, const shared_ptr<RTSPCommandInfo>&> CommandCallback;
	typedef Function0<void> DisconnectCallback;

public:
	RTSPProtocol(const shared_ptr<Socket>& sock, const CommandCallback& cmdcallback, const DisconnectCallback& disconnectCallback,bool server)
		:HTTPParse(server), m_isSending(false)
	{
		m_sock = sock;
		m_cmdcallback = cmdcallback;
		
		m_disconnect = disconnectCallback;
		m_prevalivetime = Time::getCurrentMilliSecond();
		m_bodylen = 0;
		m_haveFindHeaderStart = false;

		char* recvbuffer = m_recvBuffer.alloc(MAXRTPPACKGESIZE);

		m_sock->setSocketBuffer(1024 * 1024 * 4, 1024 * 1024 * 4);
		m_sock->setDisconnectCallback(Socket::DisconnectedCallback(&RTSPProtocol::onSocketDisconnectCallback, this));
		m_sock->async_recv(recvbuffer, MAXRTPPACKGESIZE, Socket::ReceivedCallback(&RTSPProtocol::onSocketRecvCallback, this));
	}
	~RTSPProtocol()
	{
		m_sock->disconnect();
		m_sock = NULL;
	}

	uint64_t prevalivetime() const { return m_prevalivetime; }

	void sendProtocolData(const std::string& cmdstr)
	{
		if (cmdstr.length() == 0) return;

		logdebug("\r\n%s",cmdstr.c_str());

		Guard locker(m_mutex);

		m_sendList.push_back(shared_ptr<SendPackgeInfo>(new SendPackgeInfo(cmdstr)));

		_checkSendData();
	}
	void sendMedia(const shared_ptr<STREAM_TRANS_INFO>& mediainfo, const RTPPackage& rtppackage)
	{
		INTERLEAVEDFRAME frame;
		frame.magic = RTPOVERTCPMAGIC;
		frame.channel = mediainfo->transportinfo.rtp.t.dataChannel;
		frame.rtp_len = htons((uint16_t)(rtppackage.bufferlen()));


		Guard locker(m_mutex);

		m_sendList.push_back(shared_ptr<SendPackgeInfo>(new SendPackgeInfo(String((const char*)& frame, sizeof(INTERLEAVEDFRAME)))));
		m_sendList.push_back(shared_ptr<SendPackgeInfo>(new SendPackgeInfo(rtppackage)));

		_checkSendData();
	}
	void sendContrlData(const shared_ptr<STREAM_TRANS_INFO>& mediainfo, const char* buffer, uint32_t bufferlen)
	{
		INTERLEAVEDFRAME frame;
		frame.magic = RTPOVERTCPMAGIC;
		frame.channel = mediainfo->transportinfo.rtp.t.contorlChannel;
		frame.rtp_len = htons(bufferlen);


		Guard locker(m_mutex);
		m_sendList.push_back(shared_ptr<SendPackgeInfo>(new SendPackgeInfo(std::string((const char*)& frame, sizeof(INTERLEAVEDFRAME)))));
		m_sendList.push_back(shared_ptr<SendPackgeInfo>(new SendPackgeInfo(std::string(buffer,bufferlen))));

		_checkSendData();
	}
	void setMediaTransportInfo(const shared_ptr<MEDIA_INFO>& _rtspmedia)
	{
		m_rtspmedia = _rtspmedia;
	}
private:
	void _checkSendData()
	{
		if (m_isSending || m_sendList.size() == 0) return;


		std::deque<Socket::SBuf> sendbuf;
		for (std::list<shared_ptr<SendPackgeInfo> >::iterator iter = m_sendList.begin(); iter != m_sendList.end(); iter++)
		{
			sendbuf.push_back(Socket::SBuf((*iter)->buffer + (*iter)->sendpos, (*iter)->len - (*iter)->sendpos));
		}

		m_isSending = true;

		m_prevalivetime = Time::getCurrentMilliSecond();

		m_sock->async_send(sendbuf, Socket::SendedCallback(&RTSPProtocol::onSocketSendCallback, this));
	}
	void onSocketSendCallback(const weak_ptr<Socket>& sock, const char* buffer, int len)
	{
		if (len <= 0) return;

		Guard locker(m_mutex);
		for (std::list<shared_ptr<SendPackgeInfo> >::iterator iter = m_sendList.begin(); iter != m_sendList.end() && len > 0;)
		{
			uint32_t currpagesendlen = min((uint32_t)len, (*iter)->len - (*iter)->sendpos);

			len -= currpagesendlen;

			(*iter)->sendpos += currpagesendlen;

			if ((*iter)->sendpos == (*iter)->len)
			{
				m_sendList.erase(iter++);
			}
			else
			{
				break;
			}
		}

		m_isSending = false;

		_checkSendData();
	}
	void onSocketDisconnectCallback(const weak_ptr<Socket>&, const std::string&)
	{
		m_disconnect();
	}
	void onSocketRecvCallback(const weak_ptr<Socket>& sock, const char* buffer, int len)
	{
		m_prevalivetime = Time::getCurrentMilliSecond();
		if (len <= 0)
		{
			assert(0);
			return;
		}

		if (buffer != m_recvBuffer.c_str() + m_recvBuffer.length())
		{
			assert(0);
		}

		if (m_recvBuffer.length() + len > MAXRTPPACKGESIZE)
		{
			assert(0);
		}

		m_recvBuffer.resize(m_recvBuffer.length() + len);

		const char* bufferstartaddr = m_recvBuffer.c_str();
		const char* buffertmp = bufferstartaddr;
		uint32_t bufferlen = (uint32_t)m_recvBuffer.length();

		while (bufferlen > 0)
		{
			if (m_cmdinfo != NULL)
			{
				if (m_bodylen > m_cmdinfo->body.length())
				{
					uint32_t needlen = m_bodylen - (uint32_t)m_cmdinfo->body.length();

					//数据不够
					if (bufferlen < needlen) break;

					m_cmdinfo->body +=std::string(buffertmp,needlen);

					buffertmp += needlen;
					bufferlen -= needlen;
				}
				
				{
					m_cmdcallback(m_cmdinfo);
					m_cmdinfo = NULL;
					m_bodylen = 0;
					m_haveFindHeaderStart = false;
				}
				
			}
			else if (m_cmdinfo == NULL && m_haveFindHeaderStart)
			{
				uint32_t usedlen = 0;
				shared_ptr<HTTPHeader> header = HTTPParse::parse(buffertmp,bufferlen,usedlen);
								
				if (header != NULL)
				{
					logdebug("\r\n%s", std::string(buffertmp,usedlen).c_str());

					m_cmdinfo = make_shared<RTSPCommandInfo>();
					m_cmdinfo->method = header->method;
					m_cmdinfo->url = header->url;
					m_cmdinfo->verinfo.protocol = header->verinfo.protocol;
					m_cmdinfo->verinfo.version = header->verinfo.version;
					m_cmdinfo->statuscode = header->statuscode;
					m_cmdinfo->statusmsg = header->statusmsg;
					m_cmdinfo->headers = std::move(header->headers);

					m_cmdinfo->cseq = m_cmdinfo->header("CSeq").readInt();

					m_bodylen = m_cmdinfo->header(Content_Length).readInt();

					if (m_bodylen == 0)
					{
						m_cmdcallback(m_cmdinfo);
						m_cmdinfo = NULL;
						m_bodylen = 0;
						m_haveFindHeaderStart = false;
					}
				}
				
				buffertmp += usedlen;
				bufferlen -= usedlen;
			}
			else if (*buffertmp == RTPOVERTCPMAGIC)
			{
				if (bufferlen < sizeof(INTERLEAVEDFRAME)) break;

				INTERLEAVEDFRAME* frame = (INTERLEAVEDFRAME*)buffertmp;
				
				uint32_t datalen = ntohs(frame->rtp_len);
				
				if (datalen <= 0 || datalen >= 0xffff)
				{
					//跳过数据
					
					buffertmp += 1;
					bufferlen -= 1;

					continue;
				}

				if (datalen + sizeof(INTERLEAVEDFRAME) > bufferlen)
				{
					break;
				}

				if (m_rtspmedia)
				{
					for (std::list<shared_ptr<STREAM_TRANS_INFO> >::iterator iter = m_rtspmedia->infos.begin(); iter != m_rtspmedia->infos.end(); iter++)
					{
						shared_ptr<STREAM_TRANS_INFO> transportinfo = *iter;
						if(transportinfo->transportinfo.transport != TRANSPORT_INFO::TRANSPORT_RTP_TCP) continue;
						shared_ptr<RTPSession> rtpsession = transportinfo->rtpsession;
						if(rtpsession == NULL) continue;

						if (frame->channel == transportinfo->transportinfo.rtp.t.dataChannel)
						{
							RTPPackage rtppackage(m_recvBuffer, (uint32_t)(buffertmp - bufferstartaddr + sizeof(INTERLEAVEDFRAME)), (uint32_t)datalen);

							const RTPHEADER* rtpheader = rtppackage.header();
							if (rtpheader == NULL || rtpheader->v != RTP_VERSION)
							{
							//	assert(0);
								break;
							}

							rtpsession->rtpovertcpMediaCallback(transportinfo, rtppackage);
							break;
						}
						else if (frame->channel == transportinfo->transportinfo.rtp.t.contorlChannel)
						{
							rtpsession->rtpovertcpContorlCallback(transportinfo, buffertmp + sizeof(INTERLEAVEDFRAME),datalen);
							break;
						}
					}
				}

				buffertmp += datalen + sizeof(INTERLEAVEDFRAME);
				bufferlen -= datalen + sizeof(INTERLEAVEDFRAME);
			}
			else if(!m_haveFindHeaderStart)
			{
				uint32_t ret = checkIsRTSPCommandStart(buffertmp,bufferlen);
				if (ret == 0)
				{
					buffertmp += 1;
					bufferlen -= 1;

					continue;
				}
				else if (ret == 1)
				{
					break;
				}
				else
				{
					m_haveFindHeaderStart = true;
				}
			}
			else
			{
				assert(0);
			}
		}

		String newbuffer;
		char* bufferaddr = newbuffer.alloc(MAXRTPPACKGESIZE);

		if (bufferlen > 0)
		{
			memcpy(bufferaddr, buffertmp, bufferlen);
			newbuffer.resize(bufferlen);
		}

		m_recvBuffer = newbuffer;
		
		shared_ptr<Socket> socktmp = sock.lock();
		if (socktmp != NULL)
		{
			socktmp->async_recv(bufferaddr + bufferlen, MAXRTPPACKGESIZE - bufferlen,Socket::ReceivedCallback(&RTSPProtocol::onSocketRecvCallback, this));
		}
	}
	// return 0 not cmonnand,return 1 not sure need wait,return 2 is command
	uint32_t checkIsRTSPCommandStart(const char* buffertmp,uint32_t bufferlen)
	{
		static std::string rtspcmdflag = "rtsp";

		while (bufferlen >= 4)
		{
			//b不是可现实字符，不是
			if (!isCanShowChar(*buffertmp)) return 0;

			if (strncasecmp(buffertmp, rtspcmdflag.c_str(), 4) == 0) return 2;
			
			bufferlen -= 1;
			buffertmp += 1;
		}
		return 1;
	}
	//判断是否是显示字符
	bool isCanShowChar(char ch)
	{
		return ((ch >= 0x20 && ch < 0x7f) || ch == '\r' || ch == '\n');
	}
public:
	shared_ptr<Socket>			m_sock;
private:
	CommandCallback				m_cmdcallback;
	DisconnectCallback			m_disconnect;

	String						m_recvBuffer;


	std::list<shared_ptr<SendPackgeInfo> >m_sendList;
	bool						m_isSending;


	Mutex						m_mutex;

	uint64_t					m_prevalivetime;


	bool						m_haveFindHeaderStart;
	shared_ptr<RTSPCommandInfo>	m_cmdinfo;
	uint32_t					m_bodylen;
	
	shared_ptr<MEDIA_INFO>		m_rtspmedia;
};


