#pragma  once
#include "Base/Base.h"
#include "Network/Network.h"
#include "MSProtocol/MSProtocol.h"
#include "Defs.h"

using namespace Public::Base;
using namespace Public::Network;
using namespace Milesight::Protocol;

namespace Milesight {
namespace MQ {

class Connection;



//在这里定义了MSMQ通讯的消息包
class MSMQ_API Message
{
public:
	Message(const MSProtoMessage& msg);
	Message(const Message& msg);
	Message(const shared_ptr<Message>& msg);
	Message();
	~Message();

	uint32_t sn() const;

	std::string& topic();
	const std::string& topic() const;

	std::string& toserver();
	const std::string& toserver() const;

	std::string& fromserver();
	const std::string& fromserver() const;

	uint32_t& ttl();
	uint32_t ttl() const;
	
	ErrorInfo& errorInfo();
	ErrorInfo errorInfo()const;

	MSProtoMessageDirection& direction();
	MSProtoMessageDirection direction() const;

	//用于消息认证的
	std::string& session();
	std::string session() const;

	Message& operator = (const Message& msg);

	bool parseBody(MSPacket* pkt) const;
	bool setBody(const MSPacket* pkt);
	std::string& body();
	const std::string& body() const;

	bool parseExtmsg(MSPacket* pkt) const;
	bool setExtmsg(const MSPacket* pkt);
	std::string& extmsg();
	const std::string& extmsg() const;
	
	shared_ptr<Message> buildResponse(const ErrorInfo& oresult, const MSPacket* pkt = NULL) const;
	shared_ptr<Message> buildResponse(const shared_ptr<Message>& ackdata) const;
	shared_ptr<Message> buildRequest() const;

	void setConnection(const shared_ptr<Connection>& connection);
	shared_ptr<Connection> connection();
public:
	struct MessageInternal;
	MessageInternal* internal;
};


#define NoneResponse() shared_ptr<Message>()
#define ResultResponse(rsp,req)  req->buildResponse(ErrorInfo(),&rsp)
#define SuccessResponse(req)	 req->buildResponse(ErrorInfo(),NULL)
#define ErrorResponse(ret,req)  req->buildResponse(ret,NULL)


}
}
