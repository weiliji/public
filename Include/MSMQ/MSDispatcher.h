#pragma  once
#include "Message.h"
#include "Connection.h"

namespace Milesight {
namespace MQ {

class Connection_Interface;
//这里是MSMQ消息分发中心
class MSMQ_API MSDispatcher:public enable_shared_from_this<MSDispatcher>
{
public:
	MSDispatcher();
	~MSDispatcher();
	//---------------------以下为消息订阅发送,以下消息订阅的为全局消息，监听所有connection产生的消息
	//--------------------消息订阅主要通过topic来区分消息

	//订阅请求包
	template<typename O>
	void subscribe(const std::string& topic, shared_ptr<Message>(O::*callback)(const shared_ptr<Message>&), O* o)
	{
		subscribe(topic, o, RequestMessageCallback(callback, o), ResponseMessageCallback());
	}
	//订阅请求包
	template<typename O>
	void subscribe(const std::string& topic, shared_ptr<Message>(O::*callback)(const shared_ptr<Message>&), const shared_ptr<O>& o)
	{
		subscribe(topic, o.get(), RequestMessageCallback(callback, o), ResponseMessageCallback());
	}
	//订阅回复包
	template<typename O>
	void subscribe(const std::string& topic, void(O::*callback)(const shared_ptr<Message>&), O* o)
	{
		subscribe(topic, o, RequestMessageCallback(), ResponseMessageCallback(callback, o));
	}
	//订阅请求包
	template<typename O>
	void subscribe(const std::string& topic, void(O::*callback)(const shared_ptr<Message>&), const shared_ptr<O>& o)
	{
		subscribe(topic, o.get(), RequestMessageCallback(), ResponseMessageCallback(callback, o));
	}

	//订阅消息
	void subscribe(const std::string& topic, void* flag, const RequestMessageCallback& callback, const ResponseMessageCallback& ackcallback);
	void unsubcribe(void* flag);

	void connectionRecvCallback(const shared_ptr<Connection_Interface>& connection, const shared_ptr<Message>& msg);
private:
	struct MSDispatcherInternal;
	MSDispatcherInternal* internal;
};

}
}