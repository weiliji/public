
#if 0
#include "Base/Func.h"
using namespace Public::Base;
#include "HTTP/HTTP.h"
using namespace Public::HTTP;

typedef void(*ptrtype)(int);

struct Test
{
	void testfunc(int) {}
};

void recvcallback1(const shared_ptr<HTTPServerSession>& session)
{
	session->response->statusCode() = 200;
	session->response->content()->write("this recv calblack1");
}

void recvcallback2(const shared_ptr<HTTPServerSession>& session)
{
	session->response->statusCode() = 200;
	session->response->content()->write("this recv calblack2");
}

int main()
{
	std::string url = "/api/entities/11";
	std::string math = "^/api/entities/.+";

	shared_ptr<IOWorker> worker = make_shared<IOWorker>(2);
	
	//shared_ptr<HTTPClientRequest> req = make_shared<HTTPClientRequest>("get", "http://47.106.74.104:8010/");
	//Public::HTTP::HTTPClient client(worker,"user");
	//shared_ptr<Public::HTTP::HTTPClientResponse> respse = client.request(req);

	//int a = 0;

	//std::string data = respse->content()->read();


	shared_ptr<HTTPServer> server = make_shared<HTTPServer>(worker,"userage");
	server->listen("/a$","get" ,recvcallback1);
	server->defaultListen("get", recvcallback2);

	server->run(8081);

	getchar();

	int b = 0;
	/*ifstream infile;
	infile.open("test.vcxproj.user", std::ofstream::in | std::ofstream::binary);
	if (!infile.is_open()) return false;

	while (!infile.eof())
	{
		char buffer[1024];
		streamsize readlen = infile.readsome(buffer, 1024);
		if (readlen == 0) break;
	}


	infile.close();*/


	//std::shared_ptr<Test> testptr(new Test);

	//Function1<void, int> testfunc = Function1<void, int>(&Test::testfunc, testptr.get());

	//test(1);
	
	//Function2<void,int, int> f = std::bind(&Test::testfunc, std::weak_ptr<Test>(t).lock(), std::placeholders::_1, std::placeholders::_2);

	//Function1<void, int> f1 = [&](int) {
	//	int a = 0;
	//};

	//std::function<void(int)> test1 = f1;

	/*ptrtype ptrtmp = test;

	if (test == test1)
	{
		int a = 0;
	}
	if (test != test1)
	{
		int b = 0;
	}*/
	//std::map < std::string, Host::NetworkInfo > infos;
	//std::string defaultMac;

	//Host::getNetworkInfos(infos, defaultMac);

	return 0;
}
#endif

#if 0
#include "Medis/Medis.h"
using namespace Public::Medis;

#define WORKPORT 6379

shared_ptr<IOWorker>		worker;
shared_ptr<Service>			service;


void systemExit(void*, BaseSystem::CloseEvent)
{
	service = NULL;
	worker = NULL;


	Public::Network::NetworkSystem::uninit();
	Public::Base::BaseSystem::uninit();

	Public::Base::BaseSystem::autoExitDelayer(30);
}
int main(int argc, const char* argv[])
{
	Public::Base::BaseSystem::init(systemExit);
	Public::Network::NetworkSystem::init();


	worker = make_shared<IOWorker>(16);
	service = make_shared<Service>();

	if (!service->start(worker, WORKPORT))
	{
		int a = 0;
		return -1;
	}

	ConsoleCommand cmd;
	cmd.run("medis");

	return 0;
}
#endif

#if 0
#include "OnvifClient/OnvifClient.h"
using namespace Public::Onvif;

void disconveryCallback(const shared_ptr<OnvifClientDefs::DiscoveryInfo>& info)
{
	if (info == NULL) return;
	printf("%s %s\r\n", info->name.c_str(), info->addrs[0].c_str());
}
int main()
{
	shared_ptr<IOWorker> worker = make_shared<IOWorker>(4);

	shared_ptr<OnvifClientManager> manager = make_shared<OnvifClientManager>(worker,"test");

//	manager->disconvery(disconveryCallback);

	std::string addr = "admin:ms123456@192.168.7.104";
//	std::string addr = "admin:support2019@192.168.9.205";
//	std::string addr = "admin:ms123456@192.168.10.230";
//	std::string addr = "admin:ms123456@192.168.4.105";
//	std::string addr = "admin:ms123456@192.168.4.150";

	shared_ptr<OnvifClient> client = manager->create(URL(addr));

	shared_ptr<OnvifClientDefs::Info> info = client->getInfo();
	assert(info);

	shared_ptr<OnvifClientDefs::Capabilities> cap = client->getCapabities();	//获取设备能力集合，错误信息使用XM_GetLastError捕获
	assert(cap);

	shared_ptr<OnvifClientDefs::Profiles> profile = client->getProfiles(); //获取配置信息，错误信息使用XM_GetLastError捕获
	assert(profile);

	shared_ptr<OnvifClientDefs::StreamUrl> streamurl = client->getStreamUrl(profile->infos[0]); //获取六信息,错误信息使用XM_GetLastError捕获
	assert(streamurl);

	shared_ptr<OnvifClientDefs::SnapUrl> snapurl = client->getSnapUrl(profile->infos[0]);
	assert(snapurl);
	
	shared_ptr<OnvifClientDefs::NetworkInterfaces> network = client->getNetworkInterfaces();//网络信息，错误信息使用XM_GetLastError捕获
	assert(network );

	OnvifClientDefs::PTZCtrl ptz;
	ptz.ctrlType = OnvifClientDefs::PTZCtrl::PTZ_CTRL_PAN;
	ptz.panTiltX = 0.5;

	bool ret1 = client->continuousMove(profile->infos[0], ptz);
	assert(ret1);

	bool ret2 = client->stopPTZ(profile->infos[0], ptz);
	assert(ret2 );


	bool ret3 = client->setPreset(profile->infos[0], Value(Time::getCurrentMilliSecond()).readString(), 10000);
	assert(ret3 );

	//bool ret8 = client->setPreset(profile->infos[0], "5", 10000);

	shared_ptr<OnvifClientDefs::PresetInfos> infos = client->getPreset(profile->infos[0], 10000);
	assert(infos );

	bool ret4 = client->gotoPreset(profile->infos[0],infos->infos[0]);
	assert(ret4 );

	bool ret5 = client->removePreset(profile->infos[0], infos->infos[0]);
	assert(ret5 );

	//infos = client->getPreset(profile->infos[0], 10000);


	shared_ptr<OnvifClientDefs::StartRecvAlarm> startrecv = client->startRecvAlarm(cap, 10000);
	assert(startrecv);

	/*for (uint32_t i = 0; i < 2; i++)
	{
		shared_ptr < OnvifClientDefs::RecvAlarmInfo> alarm = client->recvAlarm(startrecv);
		if (alarm == NULL)
		{
			int a = 0;
		}
	}*/
	client->stopRecvAlarm();

	//shared_ptr<OnvifClientDefs::PTZConfig> config = client->getConfigurations(); //错误信息使用XM_GetLastError捕获
	//assert(config );
	//
	//
	//shared_ptr<OnvifClientDefs::ConfigurationOptions> opt = client->getConfigurationOptions(config); //错误信息使用XM_GetLastError捕获
	//assert(opt );
	
	shared_ptr<Time> time = client->getSystemDatetime(); //错误信息使用XM_GetLastError捕获
	assert(time );

//	shared_ptr<OnvifClientDefs::StartRecvAlarm> alarminfo = client->startRecvAlarm(cap);

//	client->recvAlarm(alarminfo);

	//getchar();

	return 0;
}
#endif

#if 0
#include "Base/Base.h"

using namespace Public::Base;

#define MAXBUFFERLEN		1024

int main()
{

	FILE* fd = fopen("test.md","wb+");

	char* buffer = new char [MAXBUFFERLEN];

	uint64_t startime = Time::getCurrentMilliSecond();
	uint64_t writetotlsize = 0;
	uint64_t	prevtime = startime;
	while (1)
	{
		size_t writelen = fwrite(buffer, 1, MAXBUFFERLEN, fd);
		if (writelen <= 0) break;

		fflush(fd);

		writetotlsize += MAXBUFFERLEN;

		uint64_t nowtime = Time::getCurrentMilliSecond();

		if (nowtime - prevtime >= 1000)
		{
			logdebug("write speed %llu",writetotlsize * 1000 / (nowtime - startime));

			prevtime = nowtime;
		}
	}

	fclose(fd);

	getchar();
}

#endif

#if 0
#include "Base/Base.h"
using namespace Public::Base;


int main()
{
	LockFreeList<int> list;
	list.push_back(1);
	list.push_back(2);

	int val1 = list.front();
	list.pop_front();
	
//	int val2 = list.pop_front();

//	int val3 = list.pop_front();

	int count = list.size();


	return 0;
}

#endif


#if 0
#include "Network/Network.h"
using namespace Public::Network;

void socketRecv(const weak_ptr<Socket>& sock, const char* buf, int len, const NetAddr& addr)
{
	int a = 0;
}

void  socketSend(const weak_ptr<Socket>& sock, const char* buf, int len)
{
}


int main()
{
	shared_ptr<IOWorker> worker = make_shared<IOWorker>(2);
	shared_ptr<Socket> udp1 = UDP::create(worker);
	udp1->bind(5000);
	udp1->async_recvfrom(socketRecv);

	shared_ptr<Socket> udp2 = UDP::create(worker);

	std::deque<Socket::SBuf> sbuf;
	for (uint32_t i = 0; i < 3; i++)
	{
		Socket::SBuf buf;
		buf.bufadd = new char[10];
		sprintf((char*)buf.bufadd, "%d", i);
		buf.buflen = strlen(buf.bufadd);

		sbuf.push_back(buf);
	}

	udp2->async_sendto(sbuf, NetAddr("127.0.0.1", 5000), socketSend);

	getchar();

	return 0;
}

#endif


#if 1
#include "Base/Base.h"
using namespace Public::Base;

extern int runClient(const std::string& ipaddr, const std::list<std::string>& rtsplist);
extern int runserver(const std::list<std::string>& rtsplist);


int main(int argc, char** argv)
{
//	CPU::limit(4);

	std::list<std::string> rtspaddrlist;
	{
		std::string filename = File::getExcutableFileFullPath() + "/rtsplist.ini";
		
		FILE* fd = fopen(filename.c_str(), "rb");
		if (fd == NULL)
		{
			logerror("load config %s err",filename.c_str());
			getchar();

			return -1;
		}

		char addr[256];
		while (fgets(addr, 255, fd))
		{
			int len = strlen(addr);
			while (len > 0)
			{
				if (addr[len - 1] == '\r' || addr[len - 1] == '\n')
				{
					addr[len - 1] = 0;
					len -= 1;
				}
				else
				{
					break;
				}
			}

			rtspaddrlist.push_back(addr);
		}
		fclose(fd);
	}

	if (rtspaddrlist.size() == 0)
	{
		logerror("not rtsp addr");
		getchar();

		return -2;
	}

	{
		std::string tranrtspaddrlist = File::getExcutableFileFullPath() + "/rtsplist_trans.ini";
		FILE* fd = fopen(tranrtspaddrlist.c_str(), "wb+");

		if (fd != NULL)
		{
			std::list<std::string> rtsplisttmp;
			for (std::list<std::string>::const_iterator iter = rtspaddrlist.begin(); iter != rtspaddrlist.end(); iter++)
			{
				std::string rtspaddr = "rtsp://127.0.0.1:5554/" + Base64::encode(*iter)+"\r\n";

				fwrite(rtspaddr.c_str(), 1, rtspaddr.length(), fd);
			}

			fclose(fd);
		}
	}
//	if(0)
	if (argc == 1)
	{
		runserver(rtspaddrlist);
	}
	else
	{
		runClient(argc == 1 ? "" : argv[1], rtspaddrlist);
	}

	
	return 0;
}
#endif