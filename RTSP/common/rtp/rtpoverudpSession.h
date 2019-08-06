#pragma once
#include "rtpSession.h"
#include "../RTSPProtocol.h"
#include "rtpsort.h"
#include "RTSP/RTSPStructs.h"

#define MAXUDPPACKGELEN		56*1024

#define MAXRTPFRAMESIZE		100

class rtpOverUdpSession:public RTPSession
{
	struct FrameInfo
	{
		String			framedata;
		uint16_t		sn;
		uint32_t		tiemstmap;
		bool			mark;

		bool operator < (const FrameInfo& info) const
		{
			return sn < info.sn;
		}
	};
public:
	rtpOverUdpSession(bool _isserver, const shared_ptr<IOWorker>& ioworker,const std::string& _dstaddr, const shared_ptr<STREAM_TRANS_INFO>& _transport, const RTPSession::MediaDataCallback& _datacallback, const RTPSession::ContorlDataCallback& _contorlcallback)
		:RTPSession(_transport,_datacallback,_contorlcallback),isserver(_isserver), rtp_sendrtpsn(0), dstaddr(_dstaddr)
	{
		rtpsort = make_shared<RTPSort>(_transport,_datacallback);

		rtp_sock = UDP::create(ioworker);
		rtp_sock->bind(isserver ? transportinfo->transportinfo.rtp.u.server_port1 : transportinfo->transportinfo.rtp.u.client_port1);
		rtp_sock->setSocketBuffer(1024 * 1024 * 8, 0);
		rtp_sock->setSocketTimeout(1000, 1000);
		rtp_sock->nonBlocking(false);
		
		char* rtprecvbuffertmp = rtp_RecvBuffer.alloc(MAXUDPPACKGELEN);
		rtp_sock->async_recvfrom(rtprecvbuffertmp, MAXUDPPACKGELEN, Socket::RecvFromCallback(&rtpOverUdpSession::RTP_RecvCallback, this));

		rtcp_sock = UDP::create(ioworker);
		rtcp_sock->bind(isserver ? transportinfo->transportinfo.rtp.u.server_port2 : transportinfo->transportinfo.rtp.u.client_port2);
		rtcp_sock->setSocketBuffer(1024 * 1024, 0);
		rtcp_sock->setSocketTimeout(1000, 1000);
		rtcp_sock->nonBlocking(false);

		char* rtcprecvbuffertmp = rtcp_RecvBuffer.alloc(MAXUDPPACKGELEN);
		rtcp_sock->async_recvfrom(rtcprecvbuffertmp, MAXUDPPACKGELEN, Socket::RecvFromCallback(&rtpOverUdpSession::RTCP_RecvCallback, this));
	}
	~rtpOverUdpSession()
	{
		if(rtp_sock) rtp_sock->disconnect();
		if(rtcp_sock) rtcp_sock->disconnect();

		rtp_sock = NULL;
		rtcp_sock = NULL;
	}
	
	void sendContorlData(const shared_ptr<STREAM_TRANS_INFO>& transportinfo, const char*  buffer, uint32_t bufferlen)
	{
		Guard locker(mutex);

		String senddata = std::string(buffer, bufferlen);

		rtcp_sendlist.push_back(senddata);

		_sendAndCheckSend(false);
	}
	void sendMediaData(const shared_ptr<STREAM_TRANS_INFO>& transportinfo, uint32_t timestmap, const StringBuffer&  buffer, bool mark)
	{
		uint32_t readlen = 0;
		while (readlen < buffer.length())
		{
			uint32_t cansendlen =  min(MAXRTPPACKETLEN, buffer.length() - readlen);

			uint32_t sendtotallen = cansendlen + sizeof(RTPHEADER);

			String senddata;
			senddata.alloc(sendtotallen);
			
			char* senddatabuffer = (char*)senddata.c_str();

			RTPHEADER* header = (RTPHEADER*)senddatabuffer;
			memset(header, 0, sizeof(header));
			header->v = 2;
			header->ts = htonl(timestmap);
			header->seq = htons(rtp_sendrtpsn ++);
			header->pt = transportinfo->streaminfo.nPayLoad;
			header->m = (buffer.length() - readlen) == cansendlen ? mark : false;
			header->ssrc = htonl(transportinfo->transportinfo.ssrc);

			buffer.read(readlen, senddatabuffer + sizeof(RTPHEADER), cansendlen);
			
			senddata.resize(sendtotallen);

			{
				Guard locker(mutex);

				rtp_sendlist.push_back(senddata);

				_sendAndCheckSend(true);
			}

			readlen += cansendlen;
		}
	}
	void _sendAndCheckSend(bool isdata, const char* buffer = NULL,size_t len = 0)
	{
		std::list<String>& sendlist = isdata ? rtp_sendlist : rtcp_sendlist;
		
		bool needSendData = false;
		//第一次需要发送数据
		if (buffer == NULL && sendlist.size() == 1)
		{
			needSendData = true;
		}
		else if (buffer)
		{
			if (len < 0) return;

			if (sendlist.size() <= 0) return;

			{
				String& item = sendlist.front();
				if (buffer != item.c_str())
				{
					assert(0);
				}
				sendlist.pop_front();
			}

			needSendData = true;
		}

		if (!needSendData) return;

		if (sendlist.size() <= 0) return;

		String& item = sendlist.front();

		//处理数据的零拷贝问题，添加前置数据		
		const char* sendbuffer = item.c_str();
		uint32_t sendbufferlen = (uint32_t)item.length();

		if (isdata)
		{
			shared_ptr<Socket> socktmp = rtp_sock;
			if (socktmp)
			{
				socktmp->async_sendto(sendbuffer, sendbufferlen,
					NetAddr(dstaddr, isserver ? transportinfo->transportinfo.rtp.u.client_port1 : transportinfo->transportinfo.rtp.u.server_port1),
					Socket::SendedCallback(&rtpOverUdpSession::RTP_SendCallback, this));
			}	
		}
		else
		{
			shared_ptr<Socket> socktmp = rtcp_sock;
			if (socktmp)
			{
				socktmp->async_sendto(sendbuffer, sendbufferlen,
					NetAddr(dstaddr, isserver ? transportinfo->transportinfo.rtp.u.client_port2 : transportinfo->transportinfo.rtp.u.server_port2),
					Socket::SendedCallback(&rtpOverUdpSession::RTCP_SendCallback, this));
			}	
		}
	}
	void RTP_SendCallback(const weak_ptr<Socket>& sock, const char* buffer, int len)
	{
		Guard locker(mutex);

		_sendAndCheckSend(true, buffer, len);
	}
	void RTCP_SendCallback(const weak_ptr<Socket>& sock, const char* buffer, int len)
	{
		Guard locker(mutex);

		_sendAndCheckSend(false, buffer, len);
	}
	void RTP_RecvCallback(const weak_ptr<Socket>& sock, const char* buffer, int len,const NetAddr& otearaddr)
	{
		if (buffer != rtp_RecvBuffer.c_str() || len <= 0 || len > MAXUDPPACKGELEN) return;

		if (len > sizeof(RTPHEADER))
		{
			rtp_RecvBuffer.resize(len);

			if (otearaddr.getPort() != (isserver ? transportinfo->transportinfo.rtp.u.client_port1 : transportinfo->transportinfo.rtp.u.server_port1))
			{
				//assert(0);
			}

			rtpsort->inputRtpData(rtp_RecvBuffer);
		}
		
		shared_ptr<Socket> socktmp = sock.lock();
		if (socktmp)
		{
			char* buffertmp = rtp_RecvBuffer.alloc(MAXUDPPACKGELEN);
			socktmp->async_recvfrom(buffertmp, MAXUDPPACKGELEN, Socket::RecvFromCallback(&rtpOverUdpSession::RTP_RecvCallback, this));
		}
	}
	void RTCP_RecvCallback(const weak_ptr<Socket>& sock, const char* buffer, int len, const NetAddr& otearaddr)
	{
		if (buffer != rtcp_RecvBuffer.c_str() || len <= 0 || len > MAXUDPPACKGELEN) return;

		contorlcallback(transportinfo, buffer, len);

		shared_ptr<Socket> socktmp = sock.lock();
		if (socktmp)
		{
			socktmp->async_recvfrom((char*)rtcp_RecvBuffer.c_str(), MAXUDPPACKGELEN, Socket::RecvFromCallback(&rtpOverUdpSession::RTCP_RecvCallback, this));
		}
	}
private:
	bool					 isserver;

	Mutex					 mutex;
	
	uint16_t				 rtp_sendrtpsn;

	shared_ptr<Socket>		 rtp_sock;
	shared_ptr<Socket>		 rtcp_sock;

	String					 rtp_RecvBuffer;
	String 					 rtcp_RecvBuffer;

	shared_ptr<RTPSort>		 rtpsort;

	std::list<String>		 rtp_sendlist;
	std::list<String>		 rtcp_sendlist;

	std::string				 dstaddr;
};