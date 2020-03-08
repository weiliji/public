#include "Network/HTTP/Server.h"
#include "HTTPDefine.h"
#include "HTTPCache.h"
#include "HTTPCommunication.h"
namespace Public {
namespace Network{
namespace HTTP {

struct  ServerRequest::ServerRequestInternal
{
	DisconnectCallback		disconnectcallback;
	shared_ptr<Header>	header;
	shared_ptr<ReadContent>	content;

	weak_ptr<Socket>		sock;
};

ServerRequest::ServerRequest(const shared_ptr<Header>& header, const shared_ptr<ReadContent>& content, const shared_ptr<Socket>& sock)
{
	internal = new ServerRequestInternal;
	internal->header = header;
	internal->content = content;
	internal->sock = sock;
}
ServerRequest::~ServerRequest()
{
	SAFE_DELETE(internal);
}

shared_ptr<Header> ServerRequest::header() const
{
	return internal->header;
}
shared_ptr<ReadContent> ServerRequest::content() const
{
	return internal->content;
}

NetAddr ServerRequest::remoteAddr() const
{
	shared_ptr<Socket> sock = internal->sock.lock();
	if (sock == NULL) return NetAddr();

	return sock->getOtherAddr();
}

NetAddr ServerRequest::myAddr() const
{
	shared_ptr<Socket> sock = internal->sock.lock();
	if (sock == NULL) return NetAddr();

	return sock->getMyAddr();
}

ServerRequest::DisconnectCallback&	ServerRequest::discallback()
{
	return internal->disconnectcallback;
}




struct ServerResponse::ServerResponseInternal :public WriteContenNotify
{
	shared_ptr<Header> header;
	shared_ptr<WriteContent> content;

	weak_ptr<Communication> commu;

	virtual void ReadReady() {}
	virtual void WriteNotify()
	{
		shared_ptr<Communication> commutmp = commu.lock();
		if (commutmp) commutmp->setSendHeaderContentAndPostSend(header, content);
	}
};

ServerResponse::ServerResponse(const shared_ptr<Communication>& commu, CacheType type)
{
	internal = new ServerResponseInternal;

	internal->header = make_shared<Header>();
	internal->commu = commu;
	internal->content = make_shared<WriteContent>(commu->recvHeader, internal, type);

	internal->header->statuscode = 200;
	internal->header->statusmsg = "OK";
}
ServerResponse::~ServerResponse()
{
	internal->WriteNotify();
	SAFE_DELETE(internal);
}


shared_ptr<Header> ServerResponse::header()
{
	return internal->header;
}

shared_ptr<WriteContent> ServerResponse::content()
{
	return internal->content;
}

void ServerResponse::buildWWWAuthenticate(const std::string& username, const std::string& password)
{
	shared_ptr<Communication> commutmp = internal->commu.lock();
	if (commutmp == NULL) return;

	internal->header->statuscode = 401;
	{
		std::string digauthen = WWW_Authenticate::buildWWWAuthenticate(String::toupper(commutmp->recvHeader->method), username, password, WWW_Authenticate::Authenticate_Type_Digest);
		internal->header->add(HTTPHEADER_WWWAUTHENTICATE, digauthen);
	}
	{
		std::string digauthen = WWW_Authenticate::buildWWWAuthenticate(String::toupper(commutmp->recvHeader->method), username, password, WWW_Authenticate::Authenticate_Type_Basic);
		internal->header->add(HTTPHEADER_WWWAUTHENTICATE, digauthen);
	}
}
}
}
}