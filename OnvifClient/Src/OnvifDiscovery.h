#pragma  once
#include "protocol/CmdDiscovery.h"
#include "Network/Network.h"
#include "OnvifClient/OnvifClient.h"
using namespace Public::Network;

#define DISCONVERYPORT		3702
#define DISCONVERADDR		"239.255.255.250"

#define IP_ADD_MEMBERSHIP_V2	12

class OnvifDisconvery
{
public:
	OnvifDisconvery(){}
	~OnvifDisconvery(){}
	bool start(const shared_ptr<IOWorker>& _worker, const OnvifClientManager::DisconveryCallback& _callback)
	{
		ioworker = _worker;
		callback = _callback;

		muludp = UDP::create(ioworker);
		muludp->bind(DISCONVERYPORT);
		
		//{
		//	/* 设置要加入组播的地址 */
		//	memset(&mreq,0, sizeof(struct ip_mreq));

		//	/* 设置组地址 */
		//	mreq.imr_multiaddr.s_addr = inet_addr(DISCONVERADDR);
		//	/* 设置发送组播消息的源主机的地址信息 */
		//	mreq.imr_interface.s_addr = htonl(INADDR_ANY);

		//	/* 把本机加入组播地址，即本机网卡作为组播成员，只有加入组才能收到组播消息 */
		//	if (!muludp->setSocketOpt(IPPROTO_IP, IP_ADD_MEMBERSHIP_V2, (const char*)&mreq, sizeof(struct ip_mreq)))
		//	{
		//		return false;
		//	}
		//	int loop = 0;
		//	if (!muludp->setSocketOpt(IPPROTO_IP, IP_MULTICAST_LOOP, (const char*)&loop, sizeof(loop)))
		//	{
		//		return false;
		//	}
		//}

		muludp->async_recvfrom(Socket::RecvFromCallback(&OnvifDisconvery::onSocketMULRecvCallback, this), 1024 * 10);


		return true;
	}
	bool stop()
	{
		if (muludp)
		{
			muludp->setSocketOpt(IPPROTO_IP, IP_DROP_MEMBERSHIP, (const char*)&mreq, sizeof(struct ip_mreq));

			muludp->disconnect();
		}
		muludp = NULL;

		callback(shared_ptr<OnvifClientDefs::DiscoveryInfo>());

		return true;
	}
	bool sendSearch()
	{
		NetAddr toaddr(DISCONVERADDR, DISCONVERYPORT);

		CmdDiscovery disconvery;

		std::string protocol = disconvery.build(URL());

		shared_ptr<Socket> sock = muludp;
		if (!sock) return false;

		sock->sendto(protocol.c_str(), protocol.length(), toaddr);

		return true;
	}
private:
	void onSocketMULRecvCallback(const weak_ptr<Socket>& , const char* buffer, int len,const NetAddr& addr)
	{
		if (len > 0)
		{
			shared_ptr<OnvifClientDefs::DiscoveryInfo> info = parseDiscoverMessage(buffer, len);

			if (info) callback(info);
		}

		muludp->async_recvfrom(Socket::RecvFromCallback(&OnvifDisconvery::onSocketMULRecvCallback, this), 1024 * 10);
	}
	shared_ptr<OnvifClientDefs::DiscoveryInfo> parseDiscoverMessage(const char* buffer, int len)
	{
		size_t pos = String::indexOf(std::string(buffer, len), "ProbeMatches");
		if (pos != -1)
		{
			int a = 0;
		}

		XMLObject xml;
		if (!xml.parseBuffer(std::string(buffer,len))) return shared_ptr<OnvifClientDefs::DiscoveryInfo>();

		if (strcasecmp(xml.getRoot().name().c_str(),"Envelope") != 0) return shared_ptr<OnvifClientDefs::DiscoveryInfo>();

		const XMLObject::Child& body = xml.getRoot().getChild("Body");
		if (body.isEmpty()) return shared_ptr<OnvifClientDefs::DiscoveryInfo>();


		CmdDiscovery disconvery;
		if (!disconvery.parse(body)) return shared_ptr<OnvifClientDefs::DiscoveryInfo>();

		return disconvery.info;
	}
private:
	shared_ptr<IOWorker>	ioworker;
	OnvifClientManager::DisconveryCallback callback;
	shared_ptr<Socket>		muludp;
	
	struct ip_mreq mreq;
};