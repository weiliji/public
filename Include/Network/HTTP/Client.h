#ifndef __XM_HTTPCLIENT_H__
#define __XM_HTTPCLIENT_H__

#include "Public.h"

namespace Public{
namespace Network{
namespace HTTP {

struct Client_IMPL;
class NETWORK_API ClientRequest
{
	friend struct Client_IMPL;
public:
	typedef Function<void(const shared_ptr<ClientRequest>&, const std::string&)> DisconnectCallback;
    typedef Function<void(const weak_ptr<Socket>&,const shared_ptr<Header>& ,const char*, uint32_t)> RecvCallback;
public:
	ClientRequest(const std::string& method = "GET", const std::string& url = "/", CacheType type = CacheType_Mem);
	virtual ~ClientRequest();

	shared_ptr<Header> header();
	shared_ptr<WriteContent> content();

	uint32_t& timeout();

	DisconnectCallback&	discallback();
    RecvCallback&       recvcallback();
private:
	struct  ClientRequestInternal;
	ClientRequestInternal* internal;
};

class NETWORK_API ClientResponse
{
	friend struct Client_IMPL;
public:
	ClientResponse(const shared_ptr<Communication>& commu, CacheType type, const std::string& filename = "");
	virtual ~ClientResponse();

	shared_ptr<Header> header() const;
	shared_ptr<ReadContent> content() const;
private:
	struct ClientResponseInternal;
	ClientResponseInternal* internal;
};

class Client;
//http 异步管理
class NETWORK_API AsyncClient
{
	friend class Client;;
public:
	AsyncClient();
	~AsyncClient();
private:
	void addClient(const shared_ptr<Client>& client);
private:
	struct AsyncClientInternal;
	AsyncClientInternal* internal;
};

///HTTP客户端封装库
class NETWORK_API Client:public enable_shared_from_this<Client>
{
	friend class AsyncClient;
public:
	typedef Function<void(const shared_ptr<ClientRequest>&, const shared_ptr<ClientResponse>&)> HTTPCallback;
public:
	Client(const shared_ptr<IOWorker>& worker, const std::string& useragent);
	virtual ~Client();

	void disconnect();

	const shared_ptr<ClientResponse> request(const shared_ptr<ClientRequest>& req, const std::string& saveasfile = "");
	bool request(const shared_ptr<AsyncClient>& async, const shared_ptr<ClientRequest>& req, const HTTPCallback& callback, const std::string& saveasfile = "");
    shared_ptr<Socket>  getSocket() const;
private:
	//return true where eof or error
	bool onPoolTimerProc();
protected:
	struct ClientInternal;
	ClientInternal* internal;
};


//web socket client
class NETWORK_API WebSocketClient
{
public:
	typedef Function<void(WebSocketClient*)> DisconnectCallback;
	typedef Function<void(WebSocketClient*, const std::string&, WebSocketDataType)> RecvDataCallback;
	typedef Function<void(WebSocketClient*, bool, const std::string&)> ConnnectCallback;
public:
	WebSocketClient(const shared_ptr<IOWorker>& worker, const std::string& useragent);
	~WebSocketClient();

	shared_ptr<Header> header();

	bool connect(const std::string& url, uint32_t timout_ms, const RecvDataCallback& datacallback, const DisconnectCallback& disconnectcallback);
	bool startConnect(const std::string& url, const ConnnectCallback& connectcallback, const RecvDataCallback& datacallback, const DisconnectCallback& disconnectcallback);
	bool disconnect();
	bool send(const std::string& data, WebSocketDataType type);
	uint32_t sendListSize();
private:
	struct WebSocketClientInternal;
	WebSocketClientInternal* internal;
};

}

namespace HTTPS {

#ifdef SUPPORT_OPENSSL
///HTTPS客户端封装库
class NETWORK_API Client :public HTTP::Client
{
public:
	Client(const shared_ptr<IOWorker>& worker, const std::string& useragent);
	virtual ~Client();
};

#endif

}

}
}

#endif //__XM_HTTPCLIENT_H__
