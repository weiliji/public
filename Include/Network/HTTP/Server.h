#ifndef __Server_H__
#define __Server_H__
#include "Public.h"
#include "Base/Base.h"
#include "Network/HTTP/Public.h"
using namespace Public::Base;
namespace Public {
namespace Network{
namespace HTTP {

class NETWORK_API ServerRequest
{
public:
	typedef Function<void(const shared_ptr<ServerRequest>&, const std::string&)> DisconnectCallback;
public:
	ServerRequest(const shared_ptr<Header>& header,const shared_ptr<ReadContent>& content,const shared_ptr<Socket>& sock);
	virtual ~ServerRequest();

	shared_ptr<Header> header() const;

	shared_ptr<ReadContent> content() const;

	NetAddr remoteAddr() const;

	NetAddr myAddr() const;

	DisconnectCallback&	discallback();
private:
	struct  ServerRequestInternal;
	ServerRequestInternal* internal;
};

class NETWORK_API ServerResponse
{
public:
	ServerResponse(const shared_ptr<Communication>& commu, CacheType type);
	virtual ~ServerResponse();
	
	shared_ptr<Header> header();

	shared_ptr<WriteContent> content();

	void buildWWWAuthenticate(const std::string& username, const std::string& password);
private:
	struct ServerResponseInternal;
	ServerResponseInternal* internal;
};

struct Server_IMPL;
class NETWORK_API ServerSession
{
	friend struct Server_IMPL;
public:
	ServerSession(const shared_ptr<Communication>& commuSession, CacheType type);
	virtual ~ServerSession();

	shared_ptr<ServerRequest>		request;
	shared_ptr<ServerResponse>		response;
private:
	void disconnected();
};

//web socket session
class NETWORK_API WebSocketSession
{
	friend struct Server_IMPL;
public:
	typedef Function<void(WebSocketSession*)> DisconnectCallback;
	typedef Function<void(WebSocketSession*, const std::string&, WebSocketDataType)> RecvDataCallback;
public:
	WebSocketSession(const shared_ptr<Communication>& commuSession);
	virtual ~WebSocketSession();

	void start(const RecvDataCallback& datacallback, const DisconnectCallback& disconnectcallback);
	void stop();

	bool send(const std::string& data, WebSocketDataType type);

	shared_ptr<Header> header() const;

	NetAddr remoteAddr() const;

	uint32_t sendListSize();

	static bool checkWebsocketHeader(const shared_ptr<Header>& header);
private:
	void disconnected();
private:
	struct WebSocketSessionInternal;
	WebSocketSessionInternal* internal;
};

class NETWORK_API Server
{
public:
	typedef Function<void(const shared_ptr<ServerSession>&)> HTTPCallback;
	typedef Function<void(const shared_ptr<WebSocketSession>&)> WebsocketCallback;
public:
	Server(const shared_ptr<IOWorker>& worker, const std::string& useragent);
	~Server();
	
	// path 为 请求的url,*为所有  ,callback监听消息的回掉,处理线程数据，先于run启用
	// Add resources using path and method-string, and an anonymous function
	bool listen(const std::string& path,const std::string& method,const HTTPCallback& callback, CacheType type = CacheType_Mem);

	// path 为 请求的url,*为所有  ,callback监听消息的回掉,处理线程数据，先于run启用
	// Add resources using path and method-string, and an anonymous function
	bool listen(const std::string& path, const WebsocketCallback& callback);

	// path 为 请求的url,*为所有  ,callback监听消息的回掉,处理线程数据，先于run启用
	// Add resources using path and method-string, and an anonymous function
	bool defaultListen(const WebsocketCallback& callback);

	// path 为 请求的url,*为所有  ,callback监听消息的回掉,处理线程数据，先于run启用
	// Add resources using path and method-string, and an anonymous function
	bool defaultListen(const std::string& method, const HTTPCallback& callback, CacheType type = CacheType_Mem);

	//异步监听
	bool run(uint32_t httpport);

	//添加已socket到http中，完成http后续操作
	void addAcceptSocket(const shared_ptr<Socket>& sock, const char* appendBufAddr = NULL, uint32_t appendBufLen = 0);

	uint32_t listenPort() const;
private:
	struct ServerInternal;
	ServerInternal* internal;
};

}
}
}


#endif //__Server_H__
