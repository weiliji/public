#pragma once
#include "rtspDefine.h"

using namespace Public::RTSP;

class RTSPProtocol : public HTTP::Parser
{
#define MAXRTPPACKGESIZE 1024 * 1024 * 2
#define PROTOCOLTIMEOUT 5 * 60 * 1000
#define MAXMTUSIZE 1500
protected:
	virtual void onRecvCommandCallback(const shared_ptr<RTSPCommandInfo> &cmd) = 0;
	virtual void onRecvMediaCallback(const String &data, const INTERLEAVEDFRAME &frame, uint32_t offset, uint32_t bufferlen) = 0;
	virtual void onNetworkErrorCallback() = 0;
public:
	RTSPProtocol(bool server) : HTTP::Parser(server)
	{
		currstatu = NetStatus_connected;
	}
	~RTSPProtocol()
	{
		stop();
	}

	virtual ErrorInfo start(const shared_ptr<Socket> &_sock, const char *buffer, uint32_t len)
	{
		sock = _sock;
		currstatu = sock->getStatus();
		prevalivetime = Time::getCurrentMilliSecond();

		recvBufferSize = MAXRTPPACKGESIZE;
		recvBufferAddr = recvBuffer.alloc(recvBufferSize + 100);
		recvBufferLen = 0;

		_sock->setSocketBuffer(1024 * 1024 * 2, 1024 * 1024 * 2);
		_sock->setDisconnectCallback(Socket::DisconnectedCallback(&RTSPProtocol::onSocketDisconnectCallback, this));

		if (buffer != NULL && len > 0)
		{
			uint32_t needsize = min(MAXRTPPACKGESIZE, (int)len);
			memcpy(recvBufferAddr, buffer, needsize);

			onSocketRecvCallback(_sock, recvBufferAddr, needsize);
		}
		else
		{
			_sock->async_recv(recvBufferAddr, recvBufferSize, Socket::ReceivedCallback(&RTSPProtocol::onSocketRecvCallback, this));
		}

		return ErrorInfo();
	}

	virtual ErrorInfo stop()
	{
        cmdinfo = NULL;
		shared_ptr<Socket> socktmp = sock;
		sock = NULL;

		if (socktmp)
			socktmp->disconnect();

		return ErrorInfo();
	}

	//协议是否超时
	bool protocolIsTimeout()
	{
		uint64_t nowtime = Time::getCurrentMilliSecond();

		if (sock && nowtime > prevalivetime && nowtime - prevalivetime >= PROTOCOLTIMEOUT)
			return true;

		return false;
	}

	uint32_t getSendListSize() 
	{
		Guard locker(mutex);

		return (uint32_t)sendList.size();
	}

	uint32_t getSendCacheSize()
	{
		uint32_t size = 0;

		Guard locker(mutex);
		for (uint32_t i = 0; i < sendList.size(); i++)
		{
			size += sendList[i]->bufferlen();
		}

		return size;
	}

	ErrorInfo cleanMediaCacheData()
	{
		std::vector<shared_ptr<Socket::SendBuffer>> tmpSendList;
		Guard locker(mutex);
		for (uint32_t i = 0; i < sendList.size(); i++)
		{
			shared_ptr<SendPackgeInfo> packInfo = dynamic_pointer_cast<SendPackgeInfo>(sendList[i]);
			if (packInfo == NULL)
			{
				assert(0);
				continue;
			}

			if (packInfo->type != SendPackgeInfo::PackgeType_RtpData)
			{
				tmpSendList.push_back(packInfo);
			}
		}

		sendList = tmpSendList;

		return ErrorInfo();
	}

	ErrorInfo sendProtocolData(const std::list<shared_ptr<SendPackgeInfo>> &sendinfo)
	{
		Guard locker(mutex); 

		if (sock == NULL || sock->getStatus() != NetStatus_connected)
			return ErrorInfo(Error_Code_Network);

		for (std::list<shared_ptr<SendPackgeInfo>>::const_iterator iter = sendinfo.begin(); iter != sendinfo.end(); iter++)
		{
			shared_ptr<SendPackgeInfo> info = *iter;
			if (info == NULL)
			{
				continue;
			}
				
			if (info->type == SendPackgeInfo::PackgeType_RtpData && sendList.size() >= 10000)
			{
				logdebug("sendMediaData size %d overloop and lost packege", sendList.size());
				continue;
			}

			sendList.push_back(info);
		}
		_checkSendData();

		return ErrorInfo();
	}
	NetAddr getOtherAddr()
	{
		if (sock == NULL)
			return NetAddr();

		return sock->getOtherAddr();
	}
	NetStatus connectStatus() const
	{
		if (sock == NULL)
			return NetStatus_disconnected;

		return currstatu;
		//return sock->getStatus();
	}

private:
	void _checkSendData()
	{
		if (isSending || sendList.size() == 0)
			return;

		std::vector<shared_ptr<Socket::SendBuffer>> sendbuf;
		std::swap(sendbuf, sendList);
		sendList.clear();

		isSending = true;

		prevalivetime = Time::getCurrentMilliSecond();

		shared_ptr<Socket> tmpSocket = sock;
		if (tmpSocket)
		{
			tmpSocket->async_send(sendbuf, Socket::SendedCallback3(&RTSPProtocol::onSocketSendCallback, this));
		}
	}
	void onSocketSendCallback(const weak_ptr<Socket> &sock, const std::vector<shared_ptr<Socket::SendBuffer>> &)
	{
		Guard locker(mutex);

		isSending = false;

		_checkSendData();
	}
	void onSocketDisconnectCallback(const weak_ptr<Socket> &, const std::string &)
	{
		onNetworkErrorCallback();
		currstatu = NetStatus_disconnected;
	}

	bool _parseCommandBody(const char *&buffertmp, uint32_t &bufferlen)
	{
        shared_ptr<RTSPCommandInfo> tmpCmdInfo = cmdinfo;
        if (tmpCmdInfo == NULL)
        {
            return false;
        }

		if (bodylen > tmpCmdInfo->body.length())
		{
			uint32_t needlen = bodylen - (uint32_t)tmpCmdInfo->body.length();

			//数据不够
			if (bufferlen < needlen)
				return false;

            tmpCmdInfo->body += std::string(buffertmp, needlen);

			buffertmp += needlen;
			bufferlen -= needlen;
		}

		{
			onRecvCommandCallback(tmpCmdInfo);

            cmdinfo = NULL;
            tmpCmdInfo = NULL;
			bodylen = 0;
			haveFindHeaderStart = false;
		}

		return true;
	}
	bool _parseCommand(const char *&buffertmp, uint32_t &bufferlen)
	{
		uint32_t usedlen = 0;
		shared_ptr<HTTP::Header> header = HTTP::Parser::parse(buffertmp, bufferlen, usedlen);

		buffertmp += usedlen;
		bufferlen -= usedlen;

		if (header == NULL)
			return false;

		
        shared_ptr<RTSPCommandInfo> tmpCmdInfo = make_shared<RTSPCommandInfo>();
        tmpCmdInfo->method = header->method;
        tmpCmdInfo->url = header->url;
        tmpCmdInfo->verinfo.protocol = header->verinfo.protocol;
        tmpCmdInfo->verinfo.version = header->verinfo.version;
        tmpCmdInfo->statuscode = header->statuscode;
        tmpCmdInfo->statusmsg = header->statusmsg;
        tmpCmdInfo->headers = std::move(header->headers);

        tmpCmdInfo->cseq = tmpCmdInfo->header("CSeq").readInt();

		bodylen = tmpCmdInfo->header(Content_Length).readInt();

        cmdinfo = tmpCmdInfo;

		return _parseCommandBody(buffertmp, bufferlen);
	}
	bool _parseMedia(const char *&buffertmp, uint32_t &bufferlen)
	{
		if (bufferlen < sizeof(INTERLEAVEDFRAME))
			return false;

		INTERLEAVEDFRAME *frame = (INTERLEAVEDFRAME *)buffertmp;

		uint32_t datalen = ntohs(frame->rtp_len);

		if (datalen <= 0 /*|| datalen >= MAXMTUSIZE*/)
		{
			buffertmp += 1;
			bufferlen -= 1;

		//	assert(0);

			return true;
		}

		if (datalen + sizeof(INTERLEAVEDFRAME) > bufferlen)
		{

			return false;
		}

		{
			uint32_t leavelen = bufferlen - datalen - sizeof(INTERLEAVEDFRAME);
			const char *leavebuf = buffertmp + datalen + sizeof(INTERLEAVEDFRAME);

			if (leavelen > 0 && !isCanShowChar(*leavebuf) && *leavebuf != RTPOVERTCPMAGIC)
			{
				//assert(0);
			}
		}


		onRecvMediaCallback(recvBuffer, *frame, (uint32_t)(buffertmp + sizeof(INTERLEAVEDFRAME) - recvBuffer.c_str()), datalen);

		buffertmp += datalen + sizeof(INTERLEAVEDFRAME);
		bufferlen -= datalen + sizeof(INTERLEAVEDFRAME);

		return true;
	}
	bool _findCommandStart(const char *&buffertmp, uint32_t &bufferlen)
	{
		static std::string rtspcmdflag = "rtsp";
		const char *addrtmp = NULL;
		uint32_t lentmp = 0;

		while (bufferlen >= 4)
		{
			//找到media的开始了
			if (*buffertmp == RTPOVERTCPMAGIC)
				return true;

			//b不是可现实字符，不是
			if (!isCanShowChar(*buffertmp))
			{
				buffertmp++;
				bufferlen--;

			//	assert(0);

				addrtmp = buffertmp;
				lentmp = bufferlen;

				continue;
			}

			if (addrtmp == NULL)
			{
				addrtmp = buffertmp;
				lentmp = bufferlen;
			}

			if (String::strncasecmp(buffertmp, rtspcmdflag.c_str(), 4) == 0)
			{
				haveFindHeaderStart = true;
				break;
			}

			bufferlen -= 1;
			buffertmp += 1;
		}

		if (addrtmp != NULL)
		{
			buffertmp = addrtmp;
			bufferlen = lentmp;
		}

		return haveFindHeaderStart;
	}
	void onSocketRecvCallback(const weak_ptr<Socket> &, const char *buffer, int len)
	{
		prevalivetime = Time::getCurrentMilliSecond();
		if (len <= 0)
		{
			//assert(0);
			return;
		}

		assert(buffer == recvBufferAddr + recvBufferLen);
		assert(recvBufferLen + len <= recvBufferSize);

		recvBufferLen += len;

		const char *buffertmp = recvBufferAddr;
		uint32_t bufferlen = recvBufferLen;

	//	printf("onSocketRecvCallback recvlen %p %d %p %d\r\n",buffertmp,bufferlen,buffer,len);

		while (bufferlen > sizeof(INTERLEAVEDFRAME))
		{
			if (cmdinfo != NULL && !_parseCommandBody(buffertmp, bufferlen))
			{
				break;
			}
			else if (haveFindHeaderStart && !_parseCommand(buffertmp, bufferlen))
			{
				break;
			}
			else if (*buffertmp == RTPOVERTCPMAGIC && !_parseMedia(buffertmp, bufferlen))
			{
				break;
			}
			else if (!_findCommandStart(buffertmp, bufferlen))
			{
				break;
			}
		}

		recvBufferSize -= (uint32_t)(buffertmp - recvBufferAddr);
		recvBufferAddr = (char *)buffertmp;
		recvBufferLen = bufferlen;

		//当接收可用空间小于MTU时，重新分配一块空间进行接收
		//不能使用现空间，因为现空间的rtp包在复用
		if (recvBufferSize - recvBufferLen < MAXMTUSIZE)
		{
			String newbuffer;
			recvBufferSize = MAXRTPPACKGESIZE + recvBufferLen;
			recvBufferAddr = newbuffer.alloc(recvBufferSize + 100);
			if (bufferlen > 0)
			{
				memcpy(recvBufferAddr, buffertmp, bufferlen);
			}
			recvBufferLen = bufferlen;

			recvBuffer = newbuffer;
		}

		shared_ptr<Socket> socktmp = sock;
		if (socktmp != NULL)
		{
			int ret = socktmp->async_recv(recvBufferAddr + recvBufferLen, recvBufferSize - recvBufferLen, Socket::ReceivedCallback(&RTSPProtocol::onSocketRecvCallback, this));
		//	printf("async_recv %p %d %d\r\n", recvBufferAddr + recvBufferLen, recvBufferLen,recvBufferSize - recvBufferLen);
			(void)ret;
		}
	}
	//判断是否是显示字符
	bool isCanShowChar(char ch)
	{
		return ((ch >= 0x20 && ch < 0x7f) || ch == '\r' || ch == '\n');
	}

private:
	shared_ptr<Socket> sock;
	String recvBuffer;
	char *recvBufferAddr = NULL;
	uint32_t recvBufferSize = 0;
	uint32_t recvBufferLen = 0;

	std::vector<shared_ptr<Socket::SendBuffer>> sendList;
	bool isSending = false;

	Mutex mutex;

	uint64_t prevalivetime;
	NetStatus currstatu = NetStatus_disconnected;

	bool haveFindHeaderStart = false;
	shared_ptr<RTSPCommandInfo> cmdinfo;
	uint32_t bodylen = 0;
};
