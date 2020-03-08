#pragma once
#include "mediaSession.h"
#include "rtspDefine.h"

using namespace  Public::RTSP;

//RTP包排序算法
class RTPSort
{
#define MAXRTPFRAMESIZE		50
#define FIRSTFRAMESIZE		10
	struct RTPPackgeInfo
	{
		shared_ptr <RTPPackage>		rtp;
		uint32_t		sn;

		bool operator < (const RTPPackgeInfo& info) const
		{
			return sn < info.sn;
		}
	};
public:
	//Fucntion4<bool isvideo,uint32_t timestmap,const char* buffer,uint32_t bufferlen,bool mark>RTPDataCallback
	virtual void RTPSortCallback(const shared_ptr<RTPPackage>&) {}
public:
	RTPSort(){}
	~RTPSort() {}

	void inputRtpData(const shared_ptr<RTPPackage>& rtp)
	{
		Guard locker(mutex);

		const RTPHEADER& header = rtp->rtpHeader();

		if (header.v != RTP_VERSION)
		{
			return;
		}
		RTPPackgeInfo info;
		info.rtp = rtp;
		info.sn = ntohs(header.seq);

		if (prevframesn > 60000 && info.sn < 10000 && info.sn < prevframesn && prevframesn - info.sn > 5000)
		{
			info.sn += MAXRTPSNNUM;
		}

		rtplist.push_back(info);
		rtplist.sort();

		while (rtplist.size() > 0)
		{
			RTPPackgeInfo info = rtplist.front();

			if ((prevframesn == 0 && rtplist.size() >= FIRSTFRAMESIZE) || (prevframesn != 0 && (uint16_t)(prevframesn + 1) == (uint16_t)info.sn) || rtplist.size() >= MAXRTPFRAMESIZE)
			{
				if (rtplist.size() >= MAXRTPFRAMESIZE)
				{
					//	logdebug("RTPSort %x size %d",this,rtplist.size());
					//int a = 0;
				}

				rtplist.pop_front();

				RTPSortCallback(info.rtp);

				if (prevframesn != 0 && (uint16_t)(prevframesn + 1) != (uint16_t)info.sn)
				{
					//int a = 0;
					//	logwarn("RTP start sn %d to sn :%d loss", prevframesn, info.sn);
				}

				prevframesn = info.sn;
			}
			else
			{
				break;
			}
		}

		if (rtplist.size() == 0 && prevframesn >= MAXRTPSNNUM)
		{
			prevframesn -= MAXRTPSNNUM;
		}
	}
private:
	Mutex										mutex;
	std::list<RTPPackgeInfo>					rtplist;
	uint32_t									prevframesn = 0;
};


class MediaSessionOverUdp :public MediaSession,public RTPSort
{
#define MAXUDPPACKGESIZE			1024*65
#define MAXRECVBUFFERSIZE			1024*1024

#define MAXUDPRECVITEMPOSTSIZE		5

struct RTPSendBuffer:public Socket::SendBuffer
{
	shared_ptr<RTPPackage>		rtppacket;

	virtual const char* bufferaddr() { return rtppacket->buffer(); }
	virtual uint32_t bufferlen() { return rtppacket->bufferlen(); }
};

struct RTCPSendBuffer :public Socket::SendBuffer
{
	String			data;

	virtual const char* bufferaddr() { return data.c_str(); }
	virtual uint32_t bufferlen() { return (uint32_t)data.length(); }
};

public:
	MediaSessionOverUdp(bool _isserver, const shared_ptr<IOWorker>& ioworker,const std::string& _dstaddr, const shared_ptr<STREAM_TRANS_INFO>& _transport, const shared_ptr<RTSPHandler>& handler, const shared_ptr<RTSPCommandSender>& _rtspsender)
		:MediaSession(_transport, handler,_rtspsender, _isserver),dstaddr(_dstaddr)
	{
		recvBufferSize = MAXRECVBUFFERSIZE;
		recvBufferAddr = recvBuffer.alloc(recvBufferSize);


		rtp_sock = UDP::create(ioworker);
		rtp_sock->bind(isserver ? transportinfo->transportinfo.rtp.u.server_port1 : transportinfo->transportinfo.rtp.u.client_port1);
		rtp_sock->setSocketBuffer(1024 * 1024 * 8, 0);


		rtp_sock->async_recvfrom(recvBufferAddr, recvBufferSize, Socket::RecvFromCallback(&MediaSessionOverUdp::RTP_RecvCallback, this));
		
		rtcp_sock = UDP::create(ioworker);
		rtcp_sock->bind(isserver ? transportinfo->transportinfo.rtp.u.server_port2 : transportinfo->transportinfo.rtp.u.client_port2);
		
		rtcp_sock->async_recvfrom(Socket::RecvFromCallback(&MediaSessionOverUdp::RTCP_RecvCallback, this), MAXUDPPACKGESIZE);
	}
	~MediaSessionOverUdp()
	{
		stop();
	}
	virtual void stop()
	{
		if (rtp_sock) rtp_sock->disconnect();
		if (rtcp_sock) rtcp_sock->disconnect();

		rtp_sock = NULL;
		rtcp_sock = NULL;

		MediaSession::stop();
	}
	void sendContorlData(const shared_ptr<STREAM_TRANS_INFO>& transport, const shared_ptr<RTCPPackage>& rtcppackage)
	{
		if (rtcppackage == NULL) return;

		shared_ptr<RTCPSendBuffer> sendbuffer = make_shared<RTCPSendBuffer>();
		sendbuffer->data = rtcppackage->toString();

		shared_ptr<Socket> socktmp = rtcp_sock;
		if (socktmp)
		{
			socktmp->async_sendto(sendbuffer, 
				NetAddr(dstaddr, isserver ? transportinfo->transportinfo.rtp.u.client_port2 : transportinfo->transportinfo.rtp.u.server_port2),
				Socket::SendedCallback1(&MediaSessionOverUdp::RTCP_SendCallback, this));
		}

		shared_ptr<RTSPStatistics> statistics = transport->rtspstatistics.lock();
		if (statistics) statistics->inputSendPackage(rtcppackage);
	}

	void sendMediaFrameData(const shared_ptr<STREAM_TRANS_INFO>& transport, const shared_ptr<RTPFrame>& frame)
	{
		shared_ptr<RTPBuilder> builder = rtpbuilder;
		if (builder)
		{
			std::vector<shared_ptr<RTPPackage>> rtplist = builder->inputFrame(frame);
			for (size_t i = 0; i < rtplist.size(); i++)
			{
				sendrtpPackage(rtplist[i]);
			}
		}
	}

	void sendMediaData(const shared_ptr<STREAM_TRANS_INFO>&, const shared_ptr<RTPPackage>&)
	{
	}

	
	void RTP_SendCallback(const weak_ptr<Socket>& sock, const shared_ptr<Socket::SendBuffer>&)
	{
	}
	void RTCP_SendCallback(const weak_ptr<Socket>& sock, const shared_ptr<Socket::SendBuffer>&)
	{
	}
	void RTP_RecvCallback(const weak_ptr<Socket>& sock, const char* buf, int len,const NetAddr& otearaddr)
	{
		if (buf == NULL || len <= 0 || len > MAXUDPPACKGESIZE) return;

		if ((size_t)len > sizeof(RTPHEADER))
		{
			if (otearaddr.getPort() != (isserver ? transportinfo->transportinfo.rtp.u.client_port1 : transportinfo->transportinfo.rtp.u.server_port1))
			{
				//assert(0);
			}
			else
			{
				shared_ptr<RTPPackage> rtp = make_shared<RTPPackage>(recvBuffer, (uint32_t)(recvBufferAddr - recvBuffer.c_str()), len);

				//RTP的数据需要放入到RTP进行排序
				inputRtpData(rtp);

				recvBufferAddr += len;
				recvBufferSize -= len;
			}
		}

		shared_ptr<Socket> socktmp = sock.lock();
		if (socktmp)
		{
			if (recvBufferSize < MAXUDPPACKGESIZE)
			{
				recvBufferSize = MAXRECVBUFFERSIZE;
				recvBufferAddr = recvBuffer.alloc(recvBufferSize);
			}

			socktmp->async_recvfrom(recvBufferAddr , recvBufferSize, Socket::RecvFromCallback(&MediaSessionOverUdp::RTP_RecvCallback, this));
		}
	}
	void RTCP_RecvCallback(const weak_ptr<Socket>& sock, const char* buffer, int len, const NetAddr& otearaddr)
	{
		if (buffer == NULL || len <= 0 || len > MAXUDPPACKGESIZE) return;

		if (len > 0)
		{
			if (otearaddr.getPort() != (isserver ? transportinfo->transportinfo.rtp.u.client_port2 : transportinfo->transportinfo.rtp.u.server_port2))
			{
			//	assert(0);
			}

			shared_ptr<RTCPPackage> rtcppackage = make_shared<RTCPPackage>();
			if (rtcppackage->parse(buffer, len))
			{
				shared_ptr<RTSPStatistics> statistics = transportinfo->rtspstatistics.lock();
				if (statistics) statistics->inputRecvPackage(rtcppackage);

				shared_ptr<rtcpSession> rtcp = rtcpsession;
				if (rtcp) rtcp->inputRecvRTCP(rtcppackage);

				shared_ptr<RTSPHandler> mediahandlertmp = rtsphandler.lock();
				shared_ptr<RTSPCommandSender> cmdsender = rtspsender.lock();
				if (mediahandlertmp && cmdsender) mediahandlertmp->onRTCPPackageCallback(cmdsender, transportinfo, rtcppackage);
			}
		}		

		shared_ptr<Socket> socktmp = sock.lock();
		if (socktmp)
		{
			socktmp->async_recvfrom(Socket::RecvFromCallback(&MediaSessionOverUdp::RTCP_RecvCallback, this), MAXUDPPACKGESIZE);
		}
	}
	void RTPSortCallback(const shared_ptr<RTPPackage>& rtp)
	{
		shared_ptr<RTSPStatistics> statistics = transportinfo->rtspstatistics.lock();
		if (statistics) statistics->inputRecvPackage(rtp);
		
		shared_ptr<RTPAnalyzer> analyzer = rtpanalyzer;
		if (analyzer)
		{
			analyzer->inputRtpPacket(transportinfo, rtp);
		}
	}

	void rtpFrameCallback(const shared_ptr<RTPFrame>& frame)
	{
		shared_ptr<RTSPHandler> mediahandlertmp = rtsphandler.lock();
		shared_ptr<RTSPCommandSender> cmdsender = rtspsender.lock();
		if (mediahandlertmp && cmdsender)
		{
			streamTime = Time::getCurrentMilliSecond();
			mediahandlertmp->onRTPFrameCallback(cmdsender, transportinfo, frame);
		}
	}

	void sendrtpPackage(const shared_ptr<RTPPackage>& package)
	{
		if (package == NULL) return;

		shared_ptr<RTPSendBuffer> sendbuffer = make_shared<RTPSendBuffer>();
		sendbuffer->rtppacket = package;

		shared_ptr<Socket> socktmp = rtp_sock;
		if (socktmp)
		{
			socktmp->async_sendto(sendbuffer,
				NetAddr(dstaddr, isserver ? transportinfo->transportinfo.rtp.u.client_port1 : transportinfo->transportinfo.rtp.u.server_port1),
				Socket::SendedCallback1(&MediaSessionOverUdp::RTP_SendCallback, this));
		}

		shared_ptr<RTSPStatistics> statistics = transportinfo->rtspstatistics.lock();
		if (statistics) statistics->inputRecvPackage(package);
	}

	virtual bool hasStream()
	{
		return Time::getCurrentMilliSecond() - streamTime < STREAM_TIMEOUT;
	};
private:
	shared_ptr<Socket>		 rtp_sock;
	shared_ptr<Socket>		 rtcp_sock;


	std::string				 dstaddr;

	String					recvBuffer;
	char*					recvBufferAddr = NULL;
	uint32_t				recvBufferSize = 0;

	uint64_t				streamTime = Time::getCurrentMilliSecond();
};