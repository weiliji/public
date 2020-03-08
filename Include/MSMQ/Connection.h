#pragma  once
#include "Base/Base.h"
#include "Network/Network.h"
#include "MSProtocol/MSProtocol.h"
#include "Defs.h"
#include "Message.h"

using namespace Public::Base;
using namespace Public::Network;
using namespace Milesight::Protocol;

#define MSMQWEBSOCKERTREQUEST		"/msmq"

namespace Milesight {
namespace MQ {
	
//这里定义的为消息回调，返回值为回复内容
typedef Function<shared_ptr<Message>(const shared_ptr<Message>&)> RequestMessageCallback;
typedef Function<void(const shared_ptr<Message>&)> ResponseMessageCallback;


//对该connection操作只支持对链接的用户
//如果是connect connection模式，断开后会自动链接
//如果是listen connection模式，断开后不会自动链接
//如果是direct connection模式，不会断开

typedef enum
{
	ConnectionType_ConnectHTTP = 0,
	ConnectionType_ConnectWebSocket,
	ConnectionType_ConnectTCP,
	ConnectionType_ListenHTTP,
	ConnectionType_ListenWebsocket,
	ConnectionType_ListenTCP,
	ConnectionType_Direct,
}ConnectionType;


//这里定义的为MQ通讯的链接,与服务链接都会产生一个链接
struct SendMessageInfo;

class MSMQ_API Connection
{
public:
	Connection();
	virtual ~Connection();

	virtual void disconnect();

	virtual ConnectionType type() const = 0;

	//获取connection对方的地址
	virtual NetAddr srcAddr() const = 0;

	virtual NetStatus status() = 0;

	void setSession(const std::string& session);

	virtual void onPoolTimerProc();

	//*****************************以下为异步发送消息接口
	//直接跟toserver发送消息
	//-----------------------------这些是直接给toserver发送消息
	uint32_t pushRequest(const std::string& topic, const MSPacket* pkt, const ResponseMessageCallback& ackcallback = ResponseMessageCallback(), uint32_t timeout = 10000);
	uint32_t pushRequest(const std::string& toserver, const std::string& topic, const MSPacket* pkt, const ResponseMessageCallback& ackcallback = ResponseMessageCallback(), uint32_t timeout = 10000);
	uint32_t pushRequest(const std::string& topic, const std::string& body, const ResponseMessageCallback& ackcallback = ResponseMessageCallback(), uint32_t timeout = 10000);
	uint32_t pushRequest(const std::string& toserver, const std::string& topic, const std::string& body, const ResponseMessageCallback& ackcallback = ResponseMessageCallback(), uint32_t timeout = 10000);
	ErrorInfo pushRequest(const shared_ptr<Message>& msg, const ResponseMessageCallback& ackcallback = ResponseMessageCallback(), uint32_t timeout = 10000);
	ErrorInfo pushResponse(const shared_ptr<Message>& msg);
	ErrorInfo forwardMessage(const std::string& myserver, const std::string& toserver, const shared_ptr<Message>& msg, const MSPacket* extpkt = NULL);
	//*****************************以下为同步发送消息接口
	//直接给toserver发送消息
	virtual shared_ptr<Message> command(const std::string& toserver, const std::string& topic, const MSPacket* pkt, uint32_t timeout = 10000);
	virtual shared_ptr<Message> command(const shared_ptr<Message>& msg, uint32_t timeout = 10000);
private:
	virtual shared_ptr<SendMessageInfo> sendMessage(const shared_ptr<Message>& msg, const ResponseMessageCallback& ackcallback, uint32_t timeout) = 0;
private:
	struct ConnectionInternal;
	ConnectionInternal* internal;
};

}
}

