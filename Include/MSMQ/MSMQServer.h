#pragma  once
#include "Message.h"
#include "Connection.h"
#include "MSDefine.h"
#include "MSDispatcher.h"
#include "MSSystem.h"

namespace Milesight {
namespace MQ {

/*

	Milesight Message Queue 介绍

	//消息队列发布模式/以topic为传输媒介/topic不区分大小写

	[connection 1]	\                                                     / [connection 3]
					  --------publish------- [topic] -------subscribe-----
	[connection 2]  /                                                     \ [connection 4]


*/


//这里定义的是milesight Message Queue
//该MQ支持HTTP/Websocket/TCP/direct等通讯
//这里是给服务端用的，消息中转等
class MSMQ_API MSMQServer :public MSDispatcher
{
public:
	typedef Function<void(const shared_ptr<Socket>&, const char* buffer, uint32_t bufferlen)> RTSPSocketCallblack;
public:
	//默认消息队列线程数
	MSMQServer(uint32_t threadnum);
	~MSMQServer();

	//---------启动listen connection模式，作为服务端，可以同时监听多个端口
	//server端启动服务及server工作端口,listen
	ErrorInfo start();
	//--------停止工作
	ErrorInfo stop();

	shared_ptr<IOWorker> ioWorker();
	void registeRTSPSocketCallback(const RTSPSocketCallblack& callback);

	shared_ptr<MSSystem> system();

	//----------------通知大家自己系统的信息
	void notifySystemNotify(const MSProtoVMSSystemNotify& msg);


	//----------启动connect connection模式
	shared_ptr<Connection> getServerConnection(const std::string& serverid);	


	//---------------------以下针对direct connection模式
	//以下为同步命令,返回请求命令的回复包
	ErrorInfo invok(const std::string& topic, const MSPacket* pkt, MSPacket* response = NULL);
	ErrorInfo invok(const std::string& topic, MSPacket* response = NULL);
	virtual shared_ptr<Message> invok(const std::string& topic, const shared_ptr<Message>& msg);

	virtual ErrorInfo directNotify(const std::string& topic, const MSPacket* msg);
	shared_ptr<Connection> directNotifyConnection();
private:
	struct MSMQServerInternal;
	MSMQServerInternal* internal;
};

}
}