#pragma  once
#include "Base/Base.h"
#include "MSProtocol/MSProtocol.h"
#include "Connection.h"
using namespace Public::Base;
using namespace Milesight::Protocol;

namespace Milesight {
namespace MQ {


//system 组网及协商master等
//虚拟的system，组网，协商，等等
class MSMQ_API MSSystem
{
public:
	struct ServerInfo
	{
		MSProtoVMSSystemNotify		notify;
		std::string					recvaddr;	//直接可以接收到的地址
	};
public:
	MSSystem(const shared_ptr<IOWorker>& ioworker, bool isserver);
	~MSSystem();


	//----------------通知大家自己系统的信息
	void notifySystemNotify(const MSProtoVMSSystemNotify& msg);
	
	//----------------网络中接受到的通知信息
	void inputRecvNotify(const NetAddr& addr,const MSProtoVMSSystemNotify& msg);

	//---------------获取所有系统信息
	ErrorInfo getAllSystemList(std::map<std::string, std::map<std::string, ServerInfo> >& systemlist);

	//---------------根据系统获取服务信息
	ErrorInfo getSystemServerList(const std::string& systemid, std::map<std::string, ServerInfo>& serverlist);

	//---------------获取现在系统中master的信息
	std::string getSystemMasterServerId(const std::string& systemid);

	static bool systemNotifyCompare(const MSProtoVMSSystemNotify& notify1, const MSProtoVMSSystemNotify& notify2);

	//定时器处理
	void onPoolTimerProc();
private:
	struct MSSystemInternal;
	MSSystemInternal* internal;
};


}
}