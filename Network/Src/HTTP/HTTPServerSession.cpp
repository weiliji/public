
#include "HTTPCommunication.h"
#include "Network/HTTP/Public.h"
#include "Network/HTTP/Server.h"

namespace Public {
namespace Network{
namespace HTTP {

ServerSession::ServerSession(const shared_ptr<Communication>& commuSession, CacheType type)
{
	shared_ptr<ReadContent> content = make_shared<ReadContent>(commuSession->recvHeader, (WriteContenNotify*)NULL, type);

	commuSession->recvContent = content;

	//ServerRequest(const shared_ptr<Header>& header,const shared_ptr<ReadContent>& content,const shared_ptr<Socket>& sock);
	request = make_shared<ServerRequest>(commuSession->recvHeader, content, commuSession->socket);

	response = make_shared<ServerResponse>(commuSession, type);
}
ServerSession::~ServerSession()
{
}

void ServerSession::disconnected()
{
	request->discallback()(request, "socket disconnect");
}
}
}
}

