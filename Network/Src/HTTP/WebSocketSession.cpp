#include "Network/HTTP/Server.h"
#include "WebSocketProtocol.h"
#include "HTTPCommunication.h"

namespace Public {
namespace Network {
namespace HTTP {

class WebSocketRecvContent :public IContent, public WebSocketProtocol
{
public:
	WebSocketSession*						session;

	WebSocketSession::RecvDataCallback	recvcallback;

	WebSocketRecvContent(WebSocketSession* _session)
		:WebSocketProtocol(false, WebSocketProtocol::ParseDataCallback(&WebSocketRecvContent::parseDataCallback, this)), session(_session)
	{}
	~WebSocketRecvContent() {}

	void parseDataCallback(const std::string& data, WebSocketDataType type)
	{
		recvcallback(session, data, type);
	}

	uint32_t size() { return 0; }
	uint32_t append(const char* buffer, uint32_t len, bool& endoffile)
	{
		const char* usedaddr = parseProtocol(buffer, len);

		return (uint32_t)(usedaddr - buffer);
	}
	std::string read() { return std::string(); }
};

class WebSocketSendContent :public IContent, public WebSocketProtocol
{
public:
	WebSocketSendContent() :WebSocketProtocol(false, NULL) {}
	~WebSocketSendContent() {}

	Mutex							mutex;
	std::list<std::string>			sendlist;

	uint32_t size() { return 0; }
	uint32_t append(const char* buffer, uint32_t len, bool& endoffile) { return len; }
	std::string read()
	{
		Guard locker(mutex);

		if (sendlist.size()) return std::string();

		std::string data = sendlist.front();
		sendlist.pop_front();

		return data;
	}
};

struct WebSocketSession::WebSocketSessionInternal
{
	shared_ptr<Communication>	commusession;

	DisconnectCallback				disconnectcallback;


	shared_ptr<WebSocketRecvContent>	recvcontent;

	shared_ptr<Header>				sendheader;
	shared_ptr<WebSocketSendContent>	sendcontent;
};
WebSocketSession::WebSocketSession(const shared_ptr<Communication>& commuSession)
{
	internal = new WebSocketSessionInternal;
	internal->commusession = commuSession;
}
WebSocketSession::~WebSocketSession()
{
	stop();

	SAFE_DELETE(internal);
}

bool WebSocketSession::checkWebsocketHeader(const shared_ptr<Header>& header)
{
	if (String::strcasecmp(header->method.c_str(), "POST") != 0) return false;

	Value connectionval = header->header(CONNECTION);
	if (connectionval.empty() || String::strcasecmp(connectionval.readString().c_str(), CONNECTION_Upgrade) != 0) return false;

	Value upgradval = header->header(CONNECTION_Upgrade);
	if (upgradval.empty() || String::strcasecmp(upgradval.readString().c_str(), "websocket") != 0) return false;

	Value keyval = header->header("Sec-WebSocket-Key");

	if (keyval.empty()) return false;

	return true;
}

void WebSocketSession::start(const RecvDataCallback& datacallback, const DisconnectCallback& disconnectcallback)
{
	internal->disconnectcallback = disconnectcallback;

	internal->recvcontent = make_shared<WebSocketRecvContent>(this);
	internal->recvcontent->recvcallback = datacallback;

	internal->sendcontent = make_shared<WebSocketSendContent>();
	internal->sendheader = make_shared<Header>();
	{
		Value key = internal->commusession->recvHeader->header("Sec-WebSocket-Key");
		Value protocol = internal->commusession->recvHeader->header("Sec-WebSocket-Protocol");

		std::string value = key.readString() + WEBSOCKETMASK;
		Base::Sha1 sha1;
		sha1.input(value.c_str(), value.length());

		std::map<std::string, Value> header;
		{
			internal->sendheader->headers["Access-Control-Allow-Origin"] = "*";
			internal->sendheader->headers[CONNECTION] = CONNECTION_Upgrade;
			internal->sendheader->headers[CONNECTION_Upgrade] = "websocket";
			internal->sendheader->headers["Sec-WebSocket-Accept"] = Base64::encode(sha1.report(Sha1::REPORT_BIN));
			if (!protocol.empty())
			{
				internal->sendheader->headers["Sec-WebSocket-Protocol"] = protocol;
			}
		}

		internal->sendheader->statuscode = 101;
		internal->sendheader->statusmsg = "Switching Protocols";
	}

	internal->commusession->recvContent = internal->recvcontent;
	internal->commusession->start();
	internal->commusession->setSendHeaderContentAndPostSend(internal->sendheader, internal->sendcontent);
}
void WebSocketSession::stop()
{
	if (internal->recvcontent) internal->recvcontent->recvcallback = NULL;
}

bool WebSocketSession::send(const std::string& data, WebSocketDataType type)
{
	if (!internal->sendcontent) return false;
	{
		std::string protocol = internal->sendcontent->buildProtocol(data, type);

		Guard locker(internal->sendcontent->mutex);
		internal->sendcontent->sendlist.push_back(protocol);
	}

	internal->commusession->onPoolTimerProc();

	return true;
}
shared_ptr<Header> WebSocketSession::header() const
{
	return internal->commusession->recvHeader;
}
NetAddr WebSocketSession::remoteAddr() const
{
	return internal->commusession->socket->getOtherAddr();
}

uint32_t WebSocketSession::sendListSize()
{
	if (!internal->sendcontent) return 0;

	uint32_t cachesize = 0;

	Guard locker(internal->sendcontent->mutex);
	for (std::list<std::string>::iterator iter = internal->sendcontent->sendlist.begin(); iter != internal->sendcontent->sendlist.end(); iter++)
	{
		cachesize += (uint32_t)(*iter).length();
	}

	return cachesize;
}

void WebSocketSession::disconnected()
{
	internal->disconnectcallback(this);
}

}
}
}
