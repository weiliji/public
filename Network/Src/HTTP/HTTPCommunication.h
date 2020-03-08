#pragma once
#include "Network/HTTP/Public.h"
#include "HTTPErroCode.h"
namespace Public
{
namespace Network
{
namespace HTTP
{

class HTTPCommunicationHandler
{
public:
	HTTPCommunicationHandler() {}
	virtual ~HTTPCommunicationHandler() {}

	virtual bool onRecvHeaderOk(Communication *commu) = 0;
	virtual void onRecvContentOk(Communication *commu) = 0;
	virtual void onDisconnect(Communication *commu) = 0;
	virtual void onSessionFinish(Communication *commu) = 0;
    virtual void onRecvData(Communication *commu, const char* buffer, uint32_t len) {}
};

#define MAXHTTPCACHELEN 1024 * 56

#define HTTPTIMEOUT 60 * 1000

class Communication : public Parser, public enable_shared_from_this<Communication>
{
public:
	Communication(bool _isserver, const shared_ptr<Socket> &_sock, const shared_ptr<HTTPCommunicationHandler> &_handler, const std::string &_useragent)
		: Parser(_isserver), prevalivetime(Time::getCurrentMilliSecond()), handler(_handler), isServer(_isserver), socket(_sock), recvContentLen(-1), recvContentTotalLen(0),
		  sendContentLen(-1), sendHeaderLen(0), sendTotalLen(0), useragent(_useragent)
	{
		recvBuffer.alloc(MAXHTTPCACHELEN + 100);
	}
	~Communication() { close(); }

	void start(const char *buffer = NULL, uint32_t bufferlen = 0)
	{
		socket->setDisconnectCallback(Socket::DisconnectedCallback(&Communication::onSocketDisconnectCallback, shared_from_this()));

		char *recvBufferaddr = recvBuffer.c_str();

		if (buffer != NULL && bufferlen < MAXHTTPCACHELEN)
		{
			memcpy(recvBufferaddr, buffer, bufferlen);

			onSocketRecvCallback(socket, recvBufferaddr, bufferlen);
		}
		else
		{
			onSocketRecvCallback(socket, NULL, 0);
		}
	}
	void close()
	{
		handler = weak_ptr<HTTPCommunicationHandler>();
        shared_ptr<Socket> tmp = socket;
		if (tmp)
            tmp->disconnect();
		socket = NULL;
	}
	//session重新启动新的
	void restart()
	{
		Guard locker(mutex);

		recvHeader = NULL;
		recvContent = NULL;
		recvContentLen = -1;
		recvContentTotalLen = 0;

		sendHeader = NULL;
		sendContent = NULL;
		sendContentLen = -1;

		sendHeaderLen = 0;
		sendTotalLen = 0;

		sessionIsFinish = false;
	}
	void setSendHeaderContentAndPostSend(const shared_ptr<Header> &header, const shared_ptr<IContent> &content)
	{
		{
			Guard locker(mutex);

			if (!sendContent)
				sendContent = content;

			if (!sendHeader)
			{
				sendHeader = header;

				buildSendDefaultHeader();

				bool findcotent = false;
				sendContentLen = parseHeaderContentLen(sendHeader, findcotent);
			}
		}

		checkSendResultAndPostSend(false);
	}
	void onPoolTimerProc()
	{
		checkSendResultAndPostSend(false);
	}
	bool isTimeout()
	{
		Guard locker(mutex);
		uint64_t nowtime = Time::getCurrentMilliSecond();
		uint64_t prevtime = prevalivetime;

		if (nowtime < prevtime)
			return false;

		return nowtime - prevtime >= HTTPTIMEOUT;
	}
	void buildErrorResponse(uint32_t errcode, const std::string &errmsg)
	{
		if (sendHeader == NULL)
		{
			shared_ptr<Header> header = make_shared<Header>();
			header->statuscode = errcode;
			header->statusmsg = errmsg;

			sendHeader = header;

			setSendHeaderContentAndPostSend(header, NULL);
		}
	}

private:
	bool _sessionIsFinish()
	{
		//接收到头数据，且content数据满，有发送头数据，且发送数据已满content和header长度
		bool finish = (recvHeader && recvContentLen != (uint64_t)-1 && recvContentLen == recvContentTotalLen) &&
					  (sendHeaderLen > 0 && sendContentLen != (uint64_t)-1 && sendContentLen == sendTotalLen - sendHeaderLen);

		return finish;
	}
	void checksessionIsFinish()
	{
		bool isFinish = false;
		bool isFinishOK = false;
		{
			Guard locker(mutex);
			isFinish = _sessionIsFinish();

			isFinishOK = sessionIsFinish;
			sessionIsFinish = isFinish;
		}
		if (isFinish && !isFinishOK)
		{
			shared_ptr<HTTPCommunicationHandler> handlerobj = handler.lock();
			if (handlerobj)
				handlerobj->onSessionFinish(this);
		}
	}

	void checkSendResultAndPostSend(bool sendcheck, int len = 0)
	{
		checksessionIsFinish();

		{
			Guard locker(mutex);

			if (len > 0)
			{
				sendTotalLen += len;
				if ((size_t)len > sendBuffer.length())
				{
					assert(0);
				}
				if ((size_t)len == sendBuffer.length())
					sendBuffer.resize(0);
				else
				{
					char *sendbufferaddr = (char *)sendBuffer.c_str();
					memmove(sendbufferaddr, sendbufferaddr + len, sendBuffer.length() - len);
					sendBuffer.resize(sendBuffer.length() - len);
				}
			}

			//发送缓冲区有数据，且当前属于检测状态，不发送
			if (sendBuffer.length() > 0 && !sendcheck)
			{
				return;
			}

			if (sendHeaderLen == 0)
			{
				if (!sendHeader)
					return;

				if (sendHeader->statusmsg.length() == 0)
					sendHeader->statusmsg = HTTPErrorCode::getErrorMsg(sendHeader->statuscode);

				sendBuffer = Builder::build(!isServer, *sendHeader.get());
				sendHeaderLen = sendBuffer.length();
			}
			if (((sendContentLen > 0 && sendTotalLen == 0) || (sendContentLen > sendTotalLen - sendHeaderLen)) && sendContent)
			{
				String sendbuffertmp = sendContent->read();

				if (sendBuffer.length() > 0)
					sendBuffer += sendbuffertmp;
				else
					sendBuffer = sendbuffertmp;
			}
			if (sendBuffer.length() == 0)
			{
				return;
			}
		}

		shared_ptr<Socket> socktmp = socket;
		if (socktmp)
			socktmp->async_send(sendBuffer.c_str(), (uint32_t)sendBuffer.length(), Socket::SendedCallback(&Communication::onSocketSendCallback, shared_from_this()));
	}
	void onSocketRecvCallback(const weak_ptr<Socket> &sock, const char *buffer, int len)
	{
		char *recvbufferaddr = (char *)recvBuffer.c_str();

		if (len <= 0)
		{
			Guard locker(mutex);

			if (recvHeader && (int)recvContentLen == -1)
			{
				recvContentLen = recvContentTotalLen;
			}
		}
		else
		{
			Guard locker(mutex);

			prevalivetime = Time::getCurrentMilliSecond();

			{
				if (len > 0)
					recvBuffer.resize(recvBuffer.length() + len);

				const char *buffertmp = recvbufferaddr;
				uint32_t buffertmplen = (uint32_t)recvBuffer.length();

				while (buffertmplen > 0)
				{
					if (!recvHeader)
					{
						uint32_t usedlen = 0;
						recvHeader = parse(buffertmp, buffertmplen, usedlen);

						buffertmp += usedlen;
						buffertmplen -= usedlen;

						if (recvHeader)
						{
							bool findcontent = false;
							recvContentLen = parseHeaderContentLen(recvHeader, findcontent);

							shared_ptr<HTTPCommunicationHandler> handlerobj = handler.lock();
							if (handlerobj)
							{
								bool headerresultok = handlerobj->onRecvHeaderOk(this);

								if (!headerresultok)
									break;
							}
						}
					}
					else if (recvHeader && (recvContentLen == (uint64_t)-1 || recvContentLen > recvContentTotalLen))
					{
						if (recvContent == NULL) break;

						uint32_t needsize = buffertmplen;
						if (recvContentLen != (uint64_t)-1)
							needsize = min((uint32_t)(recvContentLen - recvContentTotalLen), buffertmplen);

						bool endoffile = false;
						uint32_t appendlen = recvContent->append(buffertmp, needsize, endoffile);

						recvContentTotalLen += appendlen;
						buffertmplen -= appendlen;
						buffertmp += appendlen;

						if (recvContentLen == (uint64_t)-1 && endoffile)
							recvContentLen = recvContentTotalLen;
					}
					if (recvHeader && recvContentLen != (uint64_t)-1 && recvContentLen == recvContentTotalLen)
					{
						shared_ptr<HTTPCommunicationHandler> handlerobj = handler.lock();
						if (handlerobj)
						{
							handlerobj->onRecvContentOk(this);
						}
						break;
					}
				}

				if (buffertmplen > 0 && recvbufferaddr != buffertmp)
				{
					memmove(recvbufferaddr, buffertmp, buffertmplen);
				}
				recvBuffer.resize(buffertmplen);
			}
            
            shared_ptr<HTTPCommunicationHandler> handlerobj = handler.lock();
            if (handlerobj) handlerobj->onRecvData(this, buffer, len);
		}

		checksessionIsFinish();

		uint32_t timeout = INFINITE;
		if (recvHeader)
		{
			bool findcontent = false;
			parseHeaderContentLen(recvHeader, findcontent);

			//如果已经接收到HTTP头，没找到contentlen那么就100毫秒的超时去接收
			if (!findcontent)
				timeout = 200;
		}

		shared_ptr<Socket> socktmp = socket;
		if (socktmp)
			socktmp->async_recv(recvbufferaddr + (uint32_t)recvBuffer.length(), MAXHTTPCACHELEN - (uint32_t)recvBuffer.length(), Socket::ReceivedCallback(&Communication::onSocketRecvCallback, shared_from_this()), timeout);
	}

	void buildSendDefaultHeader()
	{
		{
			bool iswebsocket = false;
			Value connectionval = sendHeader->header(CONNECTION);
			if (!connectionval.empty() && String::strcasecmp(connectionval.readString().c_str(), CONNECTION_Upgrade) == 0)
			{
				iswebsocket = true;
			}

			bool ischunk = false;
			Value chunkval = sendHeader->header(Transfer_Encoding);
			if (!chunkval.empty() && String::strcasecmp(chunkval.readString().c_str(), CHUNKED) == 0)
			{
				ischunk = true;
			}

			if (!iswebsocket && !ischunk)
			{
				Value contentlenval = sendHeader->header(Content_Length);
				if (contentlenval.empty())
				{
					sendHeader->push(Content_Length, sendContent->size());
				}
				contentlenval = sendHeader->header(Content_Length);
				if (!contentlenval.empty())
					sendContentLen = contentlenval.readUint64();

				Value connectval = sendHeader->header(CONNECTION);
				if (connectval.empty())
					sendHeader->push(CONNECTION, CONNECTION_KeepAlive);
			}

			if (!isServer)
			{
				//	Value accaptval = sendHeader->header("Accept");
				//	if (accaptval.empty()) sendHeader->headers["Accept"] = "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3";

				//	Value accapenctval = sendHeader->header("Accept-Encoding");
				//	if (accapenctval.empty()) sendHeader->headers["Accept-Encoding"] = "gzip, deflate";

				//	Value accaplangtval = sendHeader->header("Accept-Language");
				//	if (accaplangtval.empty()) sendHeader->headers["Accept-Language"] = "zh-CN,zh;q=0.9";
			}
		}
		if (useragent != "")
		{
			sendHeader->push("User-Agent", useragent);
		}
	}

	uint64_t parseHeaderContentLen(const shared_ptr<Header> &header, bool &findcontent)
	{
		Value contentlenval = header->header(Content_Length);
		if (!contentlenval.empty())
		{
			findcontent = true;
			return contentlenval.readUint64();
		}
		Value connectionval = header->header(CONNECTION);
		if (!connectionval.empty() && String::strcasecmp(connectionval.readString().c_str(), CONNECTION_Upgrade) == 0)
		{
			findcontent = true;
			return -1;
		}
		Value chunkval = header->header(Transfer_Encoding);
		if (!chunkval.empty() && String::strcasecmp(chunkval.readString().c_str(), CHUNKED) == 0)
		{
			findcontent = true;
			return -1;
		}
		findcontent = false;

		return -1;
	}
	void onSocketSendCallback(const weak_ptr<Socket> &sock, const char *buffer, int len)
	{
		prevalivetime = Time::getCurrentMilliSecond();
		if (len < 0)
			return;

		checkSendResultAndPostSend(true, len);
	}
	void onSocketDisconnectCallback(const weak_ptr<Socket> &sock, const std::string &)
	{
		bool finish = false;
		{
			{
				Guard locker(mutex);

				if (recvHeader && (int)recvContentLen == -1)
				{
					recvContentLen = recvContentTotalLen;
				}
				finish = _sessionIsFinish();
			}
			checksessionIsFinish();
		}
		shared_ptr<HTTPCommunicationHandler> handlerobj = handler.lock();
		if (handlerobj)
			handlerobj->onDisconnect(this);
	}

private:
	uint64_t prevalivetime;

	Mutex mutex;
	weak_ptr<HTTPCommunicationHandler> handler;

	bool isServer;

public:
	shared_ptr<Socket> socket;

	shared_ptr<Header> recvHeader;
	shared_ptr<IContent> recvContent;

private:
	uint64_t recvContentLen;
	uint64_t recvContentTotalLen;

	String recvBuffer;

public:
	shared_ptr<Header> sendHeader;
	shared_ptr<IContent> sendContent;

private:
	uint64_t sendContentLen;

	uint64_t sendHeaderLen;
	uint64_t sendTotalLen;

	bool sessionIsFinish = false;

	String sendBuffer;

	std::string useragent;
};

} // namespace HTTP
} // namespace Network
} // namespace Public
