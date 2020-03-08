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
	~OnvifDisconvery()
	{
		stop();
	}
	bool start(const shared_ptr<IOWorker>& _worker, const Host::NetworkInfo& info)
	{
		ioworker = _worker;

		std::map<std::string, Host::NetworkInfo> infos;
		std::string defaultMac;
		Host::getNetworkInfos(infos, defaultMac);

		for (std::map<std::string, Host::NetworkInfo>::iterator iter = infos.begin(); iter != infos.end(); iter++)
		{
			shared_ptr<Socket>	muludp = UDP::create(ioworker);

			{

				struct ip_mreq mreq;
				/* 设置要加入组播的地址 */
				memset(&mreq, 0, sizeof(struct ip_mreq));

				/* 设置组地址 */
				mreq.imr_multiaddr.s_addr = inet_addr(DISCONVERADDR);
				/* 设置发送组播消息的源主机的地址信息 */
				mreq.imr_interface.s_addr = inet_addr(iter->second.ip.c_str());

				/* 把本机加入组播地址，即本机网卡作为组播成员，只有加入组才能收到组播消息 */
				if (!muludp->setSocketOpt(IPPROTO_IP, IP_ADD_MEMBERSHIP_V2, (const char*)&mreq, sizeof(struct ip_mreq)))
				{
					continue;
				}
				int loop = 0;
				if (!muludp->setSocketOpt(IPPROTO_IP, IP_MULTICAST_LOOP, (const char*)&loop, sizeof(loop)))
				{
					continue;
				}
			}

			int udpport = 30000 + Time::getCurrentMilliSecond() % 10000;
			if (!muludp->bind(NetAddr(iter->second.ip, udpport)))
			{
				continue;
			}

			muludp->async_recvfrom(Socket::RecvFromCallback(&OnvifDisconvery::onSocketMULRecvCallback, this), 1024 * 10);


			muludplist[iter->second.ip] = muludp;
		}

		return true;
	}
	bool stop()
	{
		muludplist.clear();

		return true;
	}
	bool sendSearch()
	{
		CmdDiscovery disconvery;
		disconvery.initGSopProtocol();


		std::string sendtoBuffer = disconvery.build(URL());

		std::map<std::string, shared_ptr<Socket> > socktmp = muludplist;
		for (std::map<std::string, shared_ptr<Socket> >::iterator iter = socktmp.begin(); iter != socktmp.end(); iter++)
		{
			shared_ptr<Socket> socktmp = iter->second;

			if(socktmp) socktmp->sendto(sendtoBuffer.c_str(), (uint32_t)sendtoBuffer.length(), NetAddr(DISCONVERADDR, DISCONVERYPORT));
		}

		return true;
	}
	void getDeviceList(std::list<OnvifClientDefs::DiscoveryInfo>& list)
	{
		Guard locker(mutex);
		for (std::list<OnvifClientDefs::DiscoveryInfo>::iterator iter = devlist.begin(); iter != devlist.end(); iter++)
		{
			list.push_back(*iter);
		}

		devlist.clear();
	}
private:
	void onSocketMULRecvCallback(const weak_ptr<Socket>& sock, const char* buffer, int len,const NetAddr& addr)
	{
		if (len > 0)
		{
			shared_ptr<OnvifClientDefs::DiscoveryInfo> info = parseDiscoverMessage(buffer, len);

			if (info)
			{
				Guard lockre(mutex);
				devlist.push_back(*info.get());
			}
		}

		shared_ptr<Socket> socktmp = sock.lock();

		if(socktmp)
			socktmp->async_recvfrom(Socket::RecvFromCallback(&OnvifDisconvery::onSocketMULRecvCallback, this), 1024 * 10);
	}
	shared_ptr<OnvifClientDefs::DiscoveryInfo> parseDiscoverMessage(const char* buffer, int len)
	{
		CmdDiscovery disconvery;
		if(!disconvery.parseGSopProtocol(std::string(buffer, len))) return shared_ptr<OnvifClientDefs::DiscoveryInfo>();

		if(!disconvery.parseProtocol()) return shared_ptr<OnvifClientDefs::DiscoveryInfo>();

		return disconvery.info;
	}
private:
	Mutex											mutex;
	std::map<std::string, shared_ptr<Socket> >		muludplist;
	shared_ptr<IOWorker>							ioworker;
	std::list<OnvifClientDefs::DiscoveryInfo>		devlist;
};