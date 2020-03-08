#pragma  once
#include "Message.h"
#include "Connection.h"
#include "MSDispatcher.h"
#include "MSSystem.h"
namespace Milesight {
namespace MQ {

//这里只是给客户端使用的
//消息发送
class MSMQ_API MSMQClient :public MSDispatcher
{
public:
	typedef Function<void(const MSProtoVMSSystemNotify&)> QuerySytemInfoCallback;
public:
	//默认消息队列线程数
	MSMQClient(const shared_ptr<IOWorker>& ioworker, const shared_ptr<MSSystem>&mssystem, ConnectionType type = ConnectionType_ConnectTCP);
	~MSMQClient();

	//根据系统信息启动
	ErrorInfo startBySystemId(const std::string& systemId);
	//根据服务地址启动
	ErrorInfo startByServerIp(const std::string& serverIp);

	void registerQuerySystemInfoCallback(const QuerySytemInfoCallback& callback);

	//--------停止工作
	ErrorInfo stop();

	shared_ptr<Connection> connection();

	std::string systemId();
	std::string masterId();

	//定时器处理
	void onPoolTimerProc();
private:
	struct MSMQClientInternal;
	MSMQClientInternal* internal;
};

}
}