

#if 0
#include "Base/Function.h"
using namespace Public::Base;
#include "Network/Network.h"
using namespace Public::Network;

typedef void (*ptrtype)(int);

struct Test
{
	void testfunc(int) {}
};
//
//void recvcallback1(const shared_ptr<HTTP::ServerSession> &session)
//{
//	session->response->header()->statuscode = 200;
//	session->response->content()->write("this recv calblack1");
//}
//
//void recvcallback2(const shared_ptr<HTTP::ServerSession> &session)
//{
//	session->response->header()->statuscode = 200;
//	session->response->content()->write("this recv calblack2");
//}
#include <windows.h>

#pragma pack(push, 1)
typedef struct BOUNDSHEET
{
	DWORD	filepos;
	BYTE	type;
	BYTE	visible;
	BYTE	name[];
};

typedef struct BOUNDSHEET1
{
	DWORD	filepos;
	BYTE	type;
	BYTE	visible;
};
#pragma (pop)

int main()
{
	int size1 = sizeof(BOUNDSHEET);
	int size2 = sizeof(BOUNDSHEET1);

	BaseSystem::startSaveLog("test");

	getchar();
#if 0
	std::string url = "/api/entities/11";
	std::string math = "^/api/entities/.+";

	shared_ptr<IOWorker> worker = make_shared<IOWorker>(2);

	shared_ptr<HTTP::ClientRequest> req = make_shared<HTTP::ClientRequest>("get", "http://114.215.177.38");
	HTTP::Client client(worker, "user");
	shared_ptr<HTTP::ClientResponse> respse = client.request(req);

	int a = 0;

	shared_ptr<HTTP::Header> header = respse->header();
	std::string data = respse->content()->read();
	logdebug("%d %s", header->statuscode, header->statusmsg);
#endif

	/*shared_ptr<HTTPServer> server = make_shared<HTTPServer>(worker, "userage");
	server->listen("/a$", "get", recvcallback1);
	server->defaultListen("get", recvcallback2);

	server->run(8081);*/

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

	//Function<void, int> testfunc = Function<void, int>(&Test::testfunc, testptr.get());

	//test(1);

	//Function<void,int, int> f = std::bind(&Test::testfunc, std::weak_ptr<Test>(t).lock(), std::placeholders::_1, std::placeholders::_2);

	//Function<void, int> f1 = [&](int) {
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

int index = 0;
void disconveryCallback(const OnvifClientDefs::DiscoveryInfo& info)
{
	printf("%d:%s %s\r\n", index++, info.name.c_str(), info.addr.c_str());
	if (info.addr == "192.168.15.40")
	{
		int a = 0;
	}
}
int main()
{
	shared_ptr<IOWorker> worker = make_shared<IOWorker>(4);

	shared_ptr<OnvifClientManager> manager = make_shared<OnvifClientManager>(worker, "test");

	/*{
		shared_ptr<OnvifClientManager::Disconvery> disconvery = manager->disconvery();

		for (int i = 0; i < 10; i++)
		{
			Thread::sleep(1000);

			std::list<OnvifClientDefs::DiscoveryInfo> devlist;
			disconvery->getDeviceList(devlist);

			for (std::list<OnvifClientDefs::DiscoveryInfo>::iterator iter = devlist.begin(); iter != devlist.end(); iter++)
			{
				disconveryCallback(*iter);
			}
		}
	}*/


	//getchar();

	std::string addr = "admin:ms111111@192.168.15.140";
	//	std::string addr = "admin:support2019@192.168.9.205";
	//	std::string addr = "admin:ms123456@192.168.10.230";
	//	std::string addr = "admin:ms123456@192.168.4.105";
	//	std::string addr = "admin:ms123456@192.168.4.150";



	shared_ptr<OnvifClient> client = manager->create(URL(addr));


	OnvifClientDefs::Info info;

	ErrorInfo ret;

	Time time;
	ret = client->getSystemDatetime(time); //错误信息使用XM_GetLastError捕获
	assert(!ret);

	ret = client->getInfo(info);
	assert(!ret);

	OnvifClientDefs::Capabilities cap;
	ret = client->getCapabities(cap);
	assert(!ret);

	OnvifClientDefs::Profiles profile;
	ret = client->getProfiles(profile);
	assert(!ret);

	OnvifClientDefs::StreamUrl streamurl;
	ret = client->getStreamUrl(profile.infos[0], streamurl);
	assert(!ret);

	OnvifClientDefs::SnapUrl snapurl;
	snapurl.url = "http://192.168.15.155:80/snapshot.cgi?mainstream";

	ret = client->getSnapUrl(profile.infos[0], snapurl);
	assert(!ret);

	{
		/*shared_ptr<HTTPClientResponse> response = client->httpRequest("GET", "/cgi-bin/admin/admin.cgi?action=get.network.general");
		auto header = response->header();
		std::string data = response->content()->read();
		getchar();*/
	}

	{
		std::string imagedata;
		int imagetype = -1;
		ret = client->getSnapImage(snapurl, imagedata, imagetype);

		assert(!ret);
	}

	OnvifClientDefs::NetworkInterfaces network;
	ret = client->getNetworkInterfaces(network);
	assert(!ret);

	OnvifClientDefs::PTZCtrl ptz;
	ptz.ctrlType = OnvifClientDefs::PTZCtrl::PTZ_CTRL_PAN;
	ptz.panTiltX = 0.5;

	ret = client->continuousMove(profile.infos[0], ptz);
	assert(!ret);

	ret = client->stopPTZ(profile.infos[0], ptz);
	assert(!ret);


	ret = client->setPreset(profile.infos[0], Value(Time::getCurrentMilliSecond()).readString());
	assert(!ret);

	OnvifClientDefs::PresetInfos infos;

	ret = client->getPreset(profile.infos[0], infos);
	assert(!ret);

	ret = client->gotoPreset(profile.infos[0], infos.infos[0]);
	assert(!ret);

	ret = client->removePreset(profile.infos[0], infos.infos[0]);
	assert(!ret);

	//infos = client->getPreset(profile->infos[0], 10000);

	OnvifClientDefs::SubEventResponse startrecv;
	ret = client->subscribeEvent(cap, startrecv, 10000);
	assert(!ret);;

	for (uint32_t i = 0; i < 2; i++)
	{
		OnvifClientDefs::EventInfos alarm;

		ret = client->getEvent(startrecv, alarm);
		assert(!ret);
	}
	client->stopSubEvent();

	//	shared_ptr<OnvifClientDefs::StartRecvAlarm> alarminfo = client->startRecvAlarm(cap);

	//	client->recvAlarm(alarminfo);

		//getchar();

	return 0;
}
#endif

#if 0
#include "Base/Base.h"

using namespace Public::Base;

#define MAXBUFFERLEN 1024

int main()
{

	FILE* fd = fopen("test.md", "wb+");

	char* buffer = new char[MAXBUFFERLEN];

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
			logdebug("write speed %llu", writetotlsize * 1000 / (nowtime - startime));

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

#if 0
#include "Base/Base.h"
#include "RTSP/RTSP.h"
using namespace Public::Base;
using namespace Public::RTSP;

#ifdef WIN32

#ifdef _DEBUG
#pragma comment(lib, "Base_debug.lib")
#pragma comment(lib, "MSProtocol_debug.lib")
#pragma comment(lib, "RTSP_debug.lib")
#pragma comment(lib, "Network_debug.lib")
#else
#pragma comment(lib, "Base.lib")
#pragma comment(lib, "MSProtocol.lib")
#pragma comment(lib, "RTSP.lib")
#pragma comment(lib, "Network.lib")
#endif
#endif

extern int runClient22(const std::string& ipaddr, const std::list<std::string>& rtsplist);
extern int runserver(const std::list<std::string>& rtsplist);
extern int runStorage();
extern int runStorage_save();
extern int runStorage_read();

extern int runtestfile();

struct RecordInfo
{
	uint64_t	deviceid = 0;
	uint64_t	starttime = 0;
	uint64_t	stoptime = 0;
	uint32_t	datafileid = 0;
	uint32_t	indexpos = 0;
	uint32_t	filedsize = 0;
	uint32_t	streamtype;

	uint32_t	filepos = 0;
};

int main(int argc, char** argv)
{
	std::list<std::string> rtspaddrlist;
	if (0)
	{
		std::string filename = File::getExcutableFileFullPath() + "/rtsplist.ini";

		FILE* fd = fopen(filename.c_str(), "rb");
		if (fd == NULL)
		{
			logerror("load config %s err", filename.c_str());
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

		if (rtspaddrlist.size() == 0)
		{
			logerror("not rtsp addr");
			getchar();

			return -2;
		}
	}

	{
		std::string tranrtspaddrlist = File::getExcutableFileFullPath() + "/rtsplist_trans.ini";
		FILE* fd = fopen(tranrtspaddrlist.c_str(), "wb+");

		if (fd != NULL)
		{
			std::list<std::string> rtsplisttmp;
			for (std::list<std::string>::const_iterator iter = rtspaddrlist.begin(); iter != rtspaddrlist.end(); iter++)
			{
				std::string rtspaddr = "rtsp://127.0.0.1:5554/" + Base64::encode(*iter) + "\r\n";

				fwrite(rtspaddr.c_str(), 1, rtspaddr.length(), fd);
			}

			fclose(fd);
		}
	}
	
//	runmsfs();
//	runStorage();

//	runStorage_save();
//	runStorage_read();

	if (0)
		//	if (argc == 1)
	{
		//runserver(rtspaddrlist);
	}
	else
	{
		runClient22(argc == 1 ? "" : argv[1], rtspaddrlist);
	}

//	runtestfile();


	return 0;
}
#endif

#if 0
#include "Base/Base.h"
using namespace Public::Base;

void readcalblack1(int a)
{
	a = 0;
}

void readcallback2(int a, const char* b)
{
	a = 0;
}

void readcalblack3(int a, const char* b, const std::string& c)
{
	a = 0;
}

void readcalblack4(int a, const char* b, const std::string& c, double d)
{
	a = 0;
}

int main()
{
	MsgCenter msgcenter;


	MsgCenter::Worker1<int> worker1 = msgcenter.worker<int>(1);

	worker1->subscribe((void*)NULL, Function<void, int>(readcalblack1));

	worker1->publish(1);


	MsgCenter::Worker2<int, char*> worker2 = msgcenter.worker<int, char*>(2);

	worker2->subscribe((void*)NULL, Function<void, int, const char*>(readcallback2));

	worker2->publish(1, (char*)"aaa");


	MsgCenter::Worker3<int, const char*, const char*> worker3 = msgcenter.worker<int, const char*, const char*>(3);

	worker3->subscribe((void*)NULL, Function<void, int, const char*, const char*>(readcalblack3));

	worker3->publish(1, "aaa", "bbbbb");

	MsgCenter::Worker4<int, const char*, const char*, float> worker4 = msgcenter.worker<int, const char*, const char*, float>(4);

	worker4->subscribe((void*)NULL, Function<void, int, const char*, const char*, double>(readcalblack4));

	worker4->publish(1, "aaa", "bbbbb", 0);

	getchar();

	return 0;
}

#endif

#if 0
#include "Base/Function.h"
using namespace Public::Base;

void test1()
{
	int a = 0;
}

void test2(int a, int b)
{
	int i = 0;
}

int main()
{
	Function<void> f(test1);

	f();

	Function<void, int, int> f1(test2);
	f1(2, 3);

}

#endif

#if 0
#include "MSPlayer/MSPlayer.h"
#include "UnLockWindow.h"
#include <thread>
#include <future>

using namespace Milesight::Player;

class RectangleAlloc
{
#define DEFUALT_BORDER_PIX 0
	Mutex m_mutex;
	typedef std::vector<RECT>	WindowVector;
	WindowVector m_vecWindows;
public:
	RectangleAlloc(int _size, HWND _hWnd)
		: size(_size)
		, hWnd(_hWnd)
	{
		PaintWindow(size);
//		timer.start(Timer::Proc(&RectangleAlloc::TimerProc, this), 0, 1000);
	}

	virtual ~RectangleAlloc()
	{

	}

	//void TimerProc(unsigned long param)
	//{
	//	PaintWindow(GetDC(hWnd), size, m_border);
	//}

	RECT GetRect(int index)
	{
		Guard locker(m_mutex);
		return m_vecWindows[index];
	}

	void PaintWindow(int layout)
	{
		RECT rtWindow;
		GetClientRect(hWnd, &rtWindow);
		if (rtWindow.right <= rtWindow.left || rtWindow.bottom <= rtWindow.top)
		{
			return;
		}
		WindowVector vector;
		switch (layout)
		{
		case 1:
			vector = PaintRuleWindow(1);
			break;
		case 4:
			vector = PaintRuleWindow(2);
			break;
		case 9:
			vector = PaintRuleWindow(3);
			break;
		case 16:
			vector = PaintRuleWindow(4);
			break;
		case 25:
			vector = PaintRuleWindow(5);
			break;
		case 36:
			vector = PaintRuleWindow(6);
			break;
		case 49:
			vector = PaintRuleWindow(7);
			break;
		case 64:
			vector = PaintRuleWindow(8);
			break;
		case 6:
			vector = PaintOldWindow(6);
			break;
		case 8:
			vector = PaintOldWindow(8);
			break;
		case 10:
			vector = PaintEvenWindow(10);
			break;
		case 13:
			vector = PaintOldWindow(14);
			break;
		default:
		{
			if (layout % 2 == 0)
			{
				vector = PaintEvenWindow(layout);
			}
			else
			{
				layout += 1;
				vector = PaintOldWindow(layout);
			}
		}
		break;
		}

		if (vector.size() > 0)
		{
			Guard locker(m_mutex);
			//			memset(&m_rect_select, 0, sizeof(RECT));
			m_vecWindows = vector;
		}

		//		SelectWindow(m_point_select);
	}

	int Height(const RECT &rt)
	{
		return rt.bottom - rt.top;
	}

	int Width(const RECT &rt)
	{
		return rt.right - rt.left;
	}

	WindowVector PaintRuleWindow(int count)
	{
		WindowVector vec;
//		Graphics g(pDC);
		RECT rtWindow;
		GetClientRect(hWnd, &rtWindow);
		int height = Height(rtWindow) - (1 * DEFUALT_BORDER_PIX);
		int width = Width(rtWindow) - (1 * DEFUALT_BORDER_PIX);

		int singleWidth = width / count;
		int singleHeight = height / count;

//		Pen pen(Color(GetRValue(border), GetGValue(border), GetBValue(border)), DEFUALT_BORDER_PIX);
		for (int y = 0; y < count; y++)
		{
			for (int x = 0; x < count; x++)
			{
				RECT rt;
				rt.left = x * singleWidth + DEFUALT_BORDER_PIX;
				if (x == count - 1)
				{
					rt.right = width;
				}
				else
				{
					rt.right = rt.left + singleWidth - DEFUALT_BORDER_PIX;
				}

				rt.top = y * singleHeight + DEFUALT_BORDER_PIX;
				if (y == count - 1)
				{
					rt.bottom = height;
				}
				else
				{
					rt.bottom = rt.top + singleHeight - DEFUALT_BORDER_PIX;
				}

				Guard locker(m_mutex);
				vec.push_back(rt);
			}
			int nLineY = y * singleHeight;
//			g.DrawLine(&pen, 0, nLineY, width, nLineY);

			int nLineX = y * singleWidth;
//			g.DrawLine(&pen, nLineX, 0, nLineX, height);
		}
//		g.DrawLine(&pen, 0, height, width, height);
//		g.DrawLine(&pen, width, 0, width, height);
		return vec;
	}

	RECT GetDisplayRect(RECT rt)
	{
		RECT rtDisplay = rt;
		rtDisplay.left += DEFUALT_BORDER_PIX;
		rtDisplay.right -= DEFUALT_BORDER_PIX;
		rtDisplay.top += DEFUALT_BORDER_PIX;
		rtDisplay.bottom -= DEFUALT_BORDER_PIX;
		return rtDisplay;
	}

	WindowVector PaintOldWindow(int count)
	{
		count = count - 1;
		int nSingleCount = (int)((double)count / 2 + 0.5);

		int val = nSingleCount;
		WindowVector vec;
		WindowVector y_vec;
		WindowVector x_vec;
		RECT rtWindow;
		GetClientRect(hWnd, &rtWindow);
		int height = Height(rtWindow);
		int width = Width(rtWindow);

		int singleWidth = width / val;
		int singleHeight = height / val;


		RECT rt = { 0 };
		RECT rtDisplay = { 0 };


		/// 最大区域
		rt.top = 0;
		rt.bottom = (val - 1) * singleHeight + DEFUALT_BORDER_PIX;
		rt.left = 0;
		rt.right = (val - 1) *singleWidth + DEFUALT_BORDER_PIX;
//		FrameRect(pDC, &rt, brushFrame);
		rtDisplay = GetDisplayRect(rt);
		vec.push_back(rtDisplay);

		for (int x = 0; x < val; x++)
		{
			///// 最右一列
			rt.top = x * singleHeight;
			rt.bottom = rt.top + singleHeight + DEFUALT_BORDER_PIX;
			rt.left = (val - 1) * singleWidth;
			rt.right = width;

//			FrameRect(pDC, &rt, brushFrame);

			rtDisplay = GetDisplayRect(rt);
			if (x != val - 1)
			{
				y_vec.push_back(rtDisplay);
			}


			/// 最后一行
			rt.left = x * singleWidth;
			rt.right = rt.left + singleWidth + DEFUALT_BORDER_PIX;


			rt.top = (val - 1) * singleHeight;
			rt.bottom = height;
//			FrameRect(pDC, &rt, brushFrame);
			rtDisplay = GetDisplayRect(rt);
			x_vec.push_back(rtDisplay);
		}

		/// 为了保证以top再以left顺序排列
		vec.insert(vec.end(), y_vec.begin(), y_vec.end());
		vec.insert(vec.end(), x_vec.begin(), x_vec.end());
		return vec;
	}

	WindowVector PaintEvenWindow(int count)
	{
		count = count - 2;
		int nSingleCount = (int)((float)count / 2);

		int val = nSingleCount;

		WindowVector vec;
		WindowVector vec1;
		WindowVector vec2;
//		Graphics g(pDC);
		RECT rtWindow;
		GetClientRect(hWnd, &rtWindow);
		int height = Height(rtWindow);
		int width = Width(rtWindow);

		int singleWidth = width / val;
		int singleHeight = height / val;

//		Pen pen(Color(GetRValue(border), GetGValue(border), GetBValue(border)), DEFUALT_BORDER_PIX);

		RECT rt = { 0 };
		RECT rtDisplay = { 0 };


//		HBRUSH brushFrame = CreateSolidBrush(m_border);
		for (int x = 0; x < val; x++)
		{
			/// 第一行
			rt.top = 0;
			rt.bottom = rt.top + singleHeight + DEFUALT_BORDER_PIX;
			rt.left = x * singleWidth;
			if (x == val - 1)
			{
				rt.right = width;
			}
			else
			{
				rt.right = rt.left + singleWidth + DEFUALT_BORDER_PIX;
			}

//			FrameRect(pDC, &rt, brushFrame);

			rtDisplay = GetDisplayRect(rt);
			vec1.push_back(rtDisplay);

			/// 最后一行
			rt.top = (val - 1) * singleHeight;
			rt.bottom = height;
//			FrameRect(pDC, &rt, brushFrame);

			rtDisplay = GetDisplayRect(rt);
			vec2.push_back(rtDisplay);
		}

		/// 为了保证先从上往下排列，再从左到右排列的顺序
		vec.insert(vec.end(), vec1.begin(), vec1.end());
		/// 中间两个窗格
		rt.top = 1 * singleHeight;
		rt.bottom = (val - 1) * singleHeight + DEFUALT_BORDER_PIX;
		rt.left = 0;
		rt.right = width / 2 + DEFUALT_BORDER_PIX;
//		FrameRect(pDC, &rt, brushFrame);
		rtDisplay = GetDisplayRect(rt);
		vec.push_back(rtDisplay);

		rt.left = width / 2;
		rt.right = width;
//		FrameRect(pDC, &rt, brushFrame);
		rtDisplay = GetDisplayRect(rt);
		vec.push_back(rtDisplay);

		vec.insert(vec.end(), vec2.begin(), vec2.end());
		return vec;
	}
private:
	int size;
	HWND hWnd;
};

void onWindowLoopThread(std::vector<std::shared_ptr<WindowsBase>> &windows, const std::shared_ptr<RectangleAlloc> &rectalloc, std::promise<void> &promise, int totalplays)
{
	RECT rt = rectalloc->GetRect(0);
	std::shared_ptr<WindowsBase> windowbase = std::make_shared<WindowsBase>();
	windowbase->create(WS_EX_WINDOWEDGE | WS_EX_LEFT | WS_EX_LTRREADING, "ms_test_window", "ms_test_window",
		WS_POPUPWINDOW | WS_CLIPSIBLINGS | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_CAPTION, 0, 0, 100, 100, NULL, NULL, NULL, NULL);
	MoveWindow(windowbase->getHWnd(), rt.left, rt.top, rt.right - rt.left, rt.bottom - rt.top, FALSE);
//	windowbase->center();
	windowbase->show(SW_SHOWNORMAL);
	windows.push_back(windowbase);

	for (size_t i = 1; i < totalplays; i++)
	{
		rt = rectalloc->GetRect(i);
		std::shared_ptr<WindowsBase> winbase = std::make_shared<WindowsBase>();
		winbase->create(WS_EX_TOOLWINDOW | WS_EX_WINDOWEDGE | WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_TOPMOST, "ms_test_window", "ms_test_window",
			WS_POPUPWINDOW | WS_CLIPSIBLINGS | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_CAPTION, 0, 0, 100, 100, windowbase->getHWnd(), NULL, NULL, NULL);
		MoveWindow(winbase->getHWnd(), rt.left, rt.top, rt.right - rt.left, rt.bottom - rt.top, FALSE);
//		winbase->center();
		winbase->show(SW_SHOWNORMAL);

		windows.push_back(winbase);
	}
	//	std::ref(promise);
	promise.set_value();
	windowbase->msgloop();
}

int main()
{
	unsigned int totalplays = 1;
	WindowsBase::init();
	MSPlayer::init();
	std::list<shared_ptr<MSPlayer>> players;
	std::vector<shared_ptr<WindowsBase> > windows;
	std::promise<void> promise;
	std::shared_ptr<RectangleAlloc> windowpos = std::make_shared<RectangleAlloc>(totalplays, GetConsoleWindow());
	std::thread th1(onWindowLoopThread, std::ref(windows), std::ref(windowpos), std::ref(promise), totalplays);

	std::future<void> fut = promise.get_future();
	fut.get();

	unsigned int i = 101;
	unsigned int index = 0;
	while (1)
	{
		if (i >= 255)
		{
			i = 100;
		}

		if (players.size() >= totalplays && players.size() > 0)
		{
			int val;
			std::cin >> val;
			//Thread::sleep(3000);
			players.pop_front();
		}
		ostringstream stream;
//		stream << "rtsp://admin:ms111111@192.168.15."<< i++ <<":554/sub";
//		stream << "D:\\test_1.mp4";
		stream << "rtsp://admin:password@192.168.10.20:554/vod/55/0/20191129093430/20191129094637";
		std::string url = stream.str();
		int idx = index++ % windows.size();
		HWND hwnd = windows[idx]->getHWnd();
		shared_ptr<MSPlayer> player = make_shared<MSPlayer>(url, (uint32_t)hwnd);
		player->play();
//		player->enableAudio(true);
		SetConsoleTitle(url.c_str());

		players.push_back(player);

		player->setPlaySpeed(Play_Speed_Fast_2x);
		int val = 0;
		while (true)
		{
			std::cin >> val;
			player->setPlayDirect(PlayDirect::direction_backward);

			std::cin >> val;
			player->setPlayDirect(PlayDirect::direction_forward);

//			std::cout << "over\r\n";
//			std::cin >> val;
		}
		//	Thread::sleep(10000);
		//	player->setSourceFile("rtsp://admin:ms111111@192.168.15.120:554/sub");
		//	player->start();

//		std::string snapfile = File::getExcutableFileFullPath() + PATH_SEPARATOR + "1.jpg";


//		player->stop();
		//ErrorInfo err = player->snap(snapfile);
		//if (err)
		//{
		//	int a = 0;
		//}
	}
	return 0;

}
#endif

#if 0

#include <WinSock2.h>
#include <ws2ipdef.h>
#include "zip/zip.h"
#include "Base/Base.h"
#include "Network/Network.h"
#include "MSPlayer/MSPlayer.h"
using namespace Public::Zip;
using namespace Public::Base;
using namespace Public::Network;
using namespace Milesight::Player;

#define ETH_ALEN 6
#define ETH_HLEN 14
#define ETH_DATA_LEN 16
#define ETH_FRAME_LEN 1514

#ifdef DEBUG
#pragma comment(lib, "Base_debug.lib")
#pragma comment(lib, "MSProtocol_debug.lib")
#pragma comment(lib, "RTSP_debug.lib")
#pragma comment(lib, "Network_debug.lib")
#pragma comment(lib, "MSPlayer_debug.lib")
#pragma comment(lib, "MSStorage_debug.lib")
#else
#pragma comment(lib, "Base.lib")
#pragma comment(lib, "MSProtocol.lib")
#pragma comment(lib, "RTSP.lib")
#pragma comment(lib, "Network.lib")
#endif

int main()
{
	BaseSystem::startSaveLog("test");

	{
		shared_ptr<MSPlayer> player = make_shared<MSPlayer>("rtsp://admin:ms111111@192.168.15.150/main", (uint32_t)GetConsoleWindow());
		ErrorInfo err = player->init();
		assert(!err);

		err = player->play();
		assert(!err);

		int times = 0;
		while (1)
		{
			times++;

			//if (times == 10) player->setPlaySpeed(Play_Speed_Slow_4x);

			//if(times == 20) player->setPlaySpeed(Play_Speed_Slow_2x);

			//if (times == 30) player->setPlaySpeed(Play_Speed_Normal);


			//if (times == 35) player->pause();


			//if (times == 40) player->resume();

			//if (times == 60) player->setPlayDirect(direction_backward);

			Thread::sleep(1000);
		}
	}



	return 0;
}

#endif

#if 0
#include "Base/Base.h"
using namespace Public::Base;

int main()
{
	std::map<int, std::string> vallist;

	for (int i = 0; i < 100; i += 2)
	{
		vallist[i] = Value(i).readString();
	}
	for (int i = 1; i < 100; i += 2)
	{
		vallist[i] = Value(i).readString();
	}

	for (std::map<int, std::string>::iterator iter = vallist.begin(); iter != vallist.end(); )
	{
		if (iter->first % 5 == 0)
		{
			vallist.erase(iter++);
		}
		else
		{
			iter++;
		}
	}
	for (std::map<int, std::string>::iterator iter = vallist.begin(); iter != vallist.end(); iter++)
	{
		printf("%d\r\n", iter->first);
	}


	std::set<uint64_t> hashvallist;
	{
		const char* staffs[] = {
			"www.2013666.com ",
			"13927156666",
			"@_fdsa",
			"gfdgfdsgfdsg",
			"*___     ",
			"ewqe23e",
			"543254328800",
			"0!!!!!!!!!!",
			"13725036666",
			"8800676547654765",
			"613425386666",
			"6613425386666",
			" Hello",
			"abca",
			"aaca",
			"baca"
		};

		for (int i = 0; i < sizeof(staffs) / sizeof(const char*); i++)
		{
			uint64_t val = Hash::bkdr(staffs[i]);

			if (hashvallist.find(val) != hashvallist.end())
			{
				int a = 0;
			}

			hashvallist.insert(val);
		}
	}
	{
		uint64_t j = 1;
		while (j < 0xffffffffffff)
		{
			char buffer[256];
			snprintf_x(buffer, 255, "%02x.%02x.%02x.%02x.%02x.%02x", j / (255 * 255 * 255 * 255 * 255), j / (255 * 255 * 255 * 255), j / (255 * 255 * 255), j / (255 * 255), j / (255), j % (255));

			uint64_t val = Hash::bkdr(buffer);

			if (hashvallist.find(val) != hashvallist.end())
			{
				int a = 0;
			}

			hashvallist.insert(val);

			j++;
		}
		uint64_t i = 0;
		while (1)
		{
			char buffer[256];
			snprintf_x(buffer, 255, "rtsp://%d.%d.%d.%d:554/main", i / (255 * 255 * 255), i / (255 * 255), i / (255), i % (255));

			uint64_t val = Hash::bkdr(buffer);

			if (hashvallist.find(val) != hashvallist.end())
			{
				int a = 0;
			}

			hashvallist.insert(val);

			i++;
		}
	}


	return 0;
}
#endif

#if 0
#include "Base/Base.h"
using namespace Public::Base;

#include "MSProtocol/MSProtocol.h"
using namespace Milesight::Protocol;

#pragma comment(lib, "Base_debug.lib")
#pragma comment(lib, "MSProtocol_debug.lib")

int main()
{
	File::save("1111.dat","");

	char buffer[1024] = {0};

	FILE* fd = fopen("1111.dat", "rb+");
	for (int i = 0; i < 1024; i++)
	{
		fwrite(buffer, 1, 1024, fd);
	}
	fclose(fd);

	std::string s2 = File::load("devicelist.dat");

	MSProtoDeviceStatusInfos infos;

	uint64_t t1 = Time::getCurrentMilliSecond();

	MSPacket::parseFromString(s2, infos);

	uint64_t t2 = Time::getCurrentMilliSecond();

	std::string s3 = MSPacket::serializeAsString(infos);

	uint64_t t3 = Time::getCurrentMilliSecond();

	//Function<void()> f2(&AA::test1);

	return 0;
}

#endif

#if 0
#include "Base/Function.h"
using namespace Public::Base;

#include "Network/Network.h"
using namespace Public::Network;

#pragma comment(lib, "Base_debug.lib")
#pragma comment(lib, "Network_debug.lib")

int main()
{

	shared_ptr<IOWorker> worker = make_shared<IOWorker>(2);


	{
		shared_ptr<Socket> sock = TCPClient::create(worker);
		shared_ptr<Email::Client> connection = make_shared<Email::Client>(worker, "smtp.exmail.qq.com", 465, Email::Client::ProtocolType_SMTP_SSL);
		//shared_ptr<EmailConnection> connection = make_shared<EmailConnection>(worker, "smtp.exmail.qq.com", 25, EmailConnection::ConnectionType_SMTP);

		ErrorInfo ret = connection->login("lixq@milesight.com", "*******");

		shared_ptr<Email::Message> msg = make_shared<Email::Message>();
		msg->to().push_back(Email::Address("lxq00@foxmail.com", "email"));
		msg->to().push_back(Email::Address("wxs@milesight.com", "wxs"));
		msg->subject() = "11123123123";
		msg->content() = "aaa aa 啊啊啊111111111";

		ret = connection->submit(msg);
	}

	while (1) Thread::sleep(1000);

	return 0;
}
#endif

#if 0

#include "Base/Base.h"
#include <regex>
static std::string codeString(const std::string& str)
{
	const char* codekey = "Milesight VMS Database";
	int codelen = strlen(codekey);

	std::string tostr = str;
	char* tostrtmp = (char*)tostr.c_str();
	int tostrlen = tostr.length();

	for (int i = 0; i < tostrlen; i++)
	{
		tostrtmp[i] ^= codekey[i % codelen];
	}

	return tostr;
}

using namespace Public::Base; 
int main()
{
	{
		std::string src = "我是中华人民啊啊等待";
		std::string utf = String::ansi2utf8(src);
		std::string dst = String::utf82ansi(utf);
		int a = 0;
	}




    std::string str1 = "rtsp://\"aadmin:pa@*()2!_#!dd\"@192.168.1.3";
    std::string str2 = "rtsp://aadmin:pas@s#1!#$%&*()_+ward@192.168.1.3/123123";
    std::string str3 = "rtsp://aadmin:passward@192.168.2.3/main";
    std::string str4 = "rtsp://aadmin:passward@123.3.2.1/main?abc=123";
    std::string str5 = "rtsp://\"aadmin:passward\"@192.168.1.3/main";
    std::string str6 = "rtsp://\"aadmin:passward\"@192.168.1.3/main?subddas=3";
    std::string str7 = "http://\"d2\"min:passward@192.168.1.3";

	{
		std::string str1 = "ms1111";
		std::string db1 = codeString(str1);
		std::string base1 = Base64::encode(db1);
		std::string base2 = Base64::decode(base1);
		std::string db2 = codeString(base2);
		assert(db2 == str1);
		int a = 0;
	}
	



  /*  std::string str = "rtsp://\"wor@ld:world\"@192.168.1.50:80";
    std::regex re("rtsp://(\"([0-9a-zA-Z@]*:[0-9a-zA-Z]*)\"|(([0-9a-zA-Z@]*:[0-9a-zA-Z]*)))@((2(5[0-5]|[0-4]\\d))|[0-1]?\\d{1,2})(\\.((2(5[0-5]|[0-4]\\d))|[0-1]?\\d{1,2})){3}(:(\\d{1,5})|())(\\\\w+|())");
    bool ret = regex_match(str1, re);

    std::string strst = "@192.168.1.50:80";
    std::regex re1("@((2(5[0-5]|[0-4]\\d))|[0-1]?\\d{1,2})(\\.((2(5[0-5]|[0-4]\\d))|[0-1]?\\d{1,2})){3}(?:\\d|[1-9]\\d|2[0-4]\\d|25[0-5]):\\d{1,4}");
    bool ret1 = regex_match(strst, re1);

    std::regex re2("rtsp://\"([0-9a-zA-Z]*:[0-9a-zA-Z]*)\"@((2(5[0-5]|[0-4]\\d))|[0-1]?\\d{1,2})(\\.((2(5[0-5]|[0-4]\\d))|[0-1]?\\d{1,2})){3}:[0-9]*\\/[a-zA-Z0-9]*");
    bool ret2 = regex_match(str6, re2);*/

    printf("%s\r\n", str1.c_str());
    printf("%s\r\n", str2.c_str());
    printf("%s\r\n", str3.c_str());
    printf("%s\r\n", str4.c_str());
    printf("%s\r\n", str5.c_str());
    printf("%s\r\n", str6.c_str());
    printf("%s\r\n", str7.c_str());

    std::vector<std::string> testVec;
    testVec.push_back(str1);
    testVec.push_back(str2);
    testVec.push_back(str3);
    testVec.push_back(str4);
    testVec.push_back(str5);
    testVec.push_back(str6);
    testVec.push_back(str7);

    printf("\r\n");
    for (uint32_t i = 0; i < testVec.size(); i++)
    {
        URL testUrl(testVec[i]);

        std::string authen = testUrl.getAuhen();
        std::string host = testUrl.getHost();
        uint32_t port = testUrl.getPort();
        std::string path = testUrl.getPath();

        bool ret = testUrl.valid();
        printf("value = %s, authen = %s, host = %s, port = %d , path = %s %s \r\n", ret ? "true" : "false", URLEncoding::decode(authen).c_str(), host.c_str(), port, path.c_str(), URLEncoding::decode(testUrl.href()).c_str());
    }


    while (1) Thread::sleep(1000);

    return 0;
}

#endif


#if 0
#include "Base/Base.h"
#include "Network/Network.h"

using namespace Public::Base;
using namespace Public::Network;

#ifdef WIN32

#ifdef _DEBUG
#pragma comment(lib, "Base_debug.lib")
#pragma comment(lib, "Network_debug.lib")
#else
#pragma comment(lib, "Base.lib")
#pragma comment(lib, "MSProtocol.lib")
#pragma comment(lib, "RTSP.lib")
#pragma comment(lib, "Network.lib")
#endif
#endif


void connectCallback (const weak_ptr<Socket>& socket, bool status, const std::string& err)
{
    printf("%s %s %s\r\n", socket.lock()->getOtherAddr().getIP().c_str(), status ? "true" : "false", err.c_str());
}

void threadproc(Thread* thread, void*)
{
    while (thread->looping())
    {
        //String data;
        //data.alloc(2 * 1024 * 1024);
        std::vector<std::string> vec;
        for (int i = 0; i < 10000; i++)
        {
            
            vec.push_back("aaaaaaaadfasdfaaaaaaaaaa333333333333333333333333aaaaaaaaaaaaaaaaaaaaaafgggggggggggggdeeeeeeeeeeeeeeea\
                aaaaaaaadfasdfaaaaaaaaaa333333333333333333333333aaaaaaaaaaaaaaaaaaaaaafgggggggggggggdeeeeeeeeeeeeeeea\
                aaaaaaaadfasdfaaaaaaaaaa333333333333333333333333aaaaaaaaaaaaaaaaaaaaaafgggggggggggggdeeeeeeeeeeeeeeea\
                aaaaaaaadfasdfaaaaaaaaaa333333333333333333333333aaaaaaaaaaaaaaaaaaaaaafgggggggggggggdeeeeeeeeeeeeeeea\
                aaaaaaaadfasdfaaaaaaaaaa333333333333333333333333aaaaaaaaaaaaaaaaaaaaaafgggggggggggggdeeeeeeeeeeeeeeea\
                aaaaaaaadfasdfaaaaaaaaaa333333333333333333333333aaaaaaaaaaaaaaaaaaaaaafgggggggggggggdeeeeeeeeeeeeeeea\
                aaaaaaaadfasdfaaaaaaaaaa333333333333333333333333aaaaaaaaaaaaaaaaaaaaaafgggggggggggggdeeeeeeeeeeeeeeea\
                aaaaaaaadfasdfaaaaaaaaaa333333333333333333333333aaaaaaaaaaaaaaaaaaaaaafgggggggggggggdeeeeeeeeeeeeeeea\
                aaaaaaaadfasdfaaaaaaaaaa333333333333333333333333aaaaaaaaaaaaaaaaaaaaaafgggggggggggggdeeeeeeeeeeeeeeea\
                aaaaaaaadfasdfaaaaaaaaaa333333333333333333333333aaaaaaaaaaaaaaaaaaaaaafgggggggggggggdeeeeeeeeeeeeeeea\
                aaaaaaaadfasdfaaaaaaaaaa333333333333333333333333aaaaaaaaaaaaaaaaaaaaaafgggggggggggggdeeeeeeeeeeeeeeea\
                aaaaaaaadfasdfaaaaaaaaaa333333333333333333333333aaaaaaaaaaaaaaaaaaaaaafgggggggggggggdeeeeeeeeeeeeeeea\
                aaaaaaaadfasdfaaaaaaaaaa333333333333333333333333aaaaaaaaaaaaaaaaaaaaaafgggggggggggggdeeeeeeeeeeeeeeea\
                aaaaaaaadfasdfaaaaaaaaaa333333333333333333333333aaaaaaaaaaaaaaaaaaaaaafgggggggggggggdeeeeeeeeeeeeeeea\
                aaaaaaaadfasdfaaaaaaaaaa333333333333333333333333aaaaaaaaaaaaaaaaaaaaaafgggggggggggggdeeeeeeeeeeeeeeea\
                aaaaaaaadfasdfaaaaaaaaaa333333333333333333333333aaaaaaaaaaaaaaaaaaaaaafgggggggggggggdeeeeeeeeeeeeeeea");
        }

        Thread::sleep(500);
    }
}
std::list<shared_ptr<Thread> > threadlist;
std::list<shared_ptr<Socket> > socketlist;
int main(int argc, char** argv)
{
    for (int i = 0; i < 12; i++)
    {
        shared_ptr<Thread> thr = ThreadEx::creatThreadEx("11", threadproc, NULL);
        thr->createThread();
        threadlist.push_back(thr);
    }

    shared_ptr<IOWorker> ioworker = make_shared<IOWorker>(4);
    
    std::vector<std::string> iplist;
    for (uint32_t i = 0; i < 255; i++)
    {
        iplist.push_back("192.168.15." + Value(i).readString());
    }

	int index = 0;
    while (1)
    {
        socketlist.clear();
        while (socketlist.size() < 200)
        {
            shared_ptr<Socket> socket = TCPClient::create(ioworker);
            NetAddr addr(iplist[index++%iplist.size()], 80);
            socket->async_connect(addr, connectCallback, 5000);

            socketlist.push_back(socket);
        }

        Thread::sleep(7000);
    }

    return 0;
}
#endif



#if 0
#include "Base/Base.h"
#include "Network/Network.h"

using namespace Public::Base;
using namespace Public::Network;

#ifdef WIN32

#ifdef _DEBUG
#pragma comment(lib, "Base_debug.lib")
#pragma comment(lib, "Network_debug.lib")
#else
#pragma comment(lib, "Base.lib")
#pragma comment(lib, "MSProtocol.lib")
#pragma comment(lib, "RTSP.lib")
#pragma comment(lib, "Network.lib")
#endif
#endif


void httpAscynCallback(const shared_ptr<HTTP::ClientRequest>& req, const shared_ptr<HTTP::ClientResponse>&response)
{
    if (response == NULL || response->header()->statuscode == 408)
    {
        auto a = ErrorInfo(Error_Code_ConnectTimeout);
    }
    else if (response && response->header()->statuscode == 401)
    {
        auto a = ErrorInfo(Error_Code_Authen);
    }
    else if (response && response->header()->statuscode != 200)
    {
        auto a = ErrorInfo(Error_Code_Request);
    }
    else if (response && response->header()->statuscode == 200)
    {
		std::string data = response->content()->read();
        int a = 0;
    }
}

int main(int argc, char** argv)
{
    shared_ptr<IOWorker> ioworker = make_shared<IOWorker>(4);
    shared_ptr< HTTP::AsyncClient>  httpasync = make_shared<HTTP::AsyncClient>();


    shared_ptr<HTTP::Client> client = make_shared<HTTP::Client>(ioworker, "test");
    


    shared_ptr<HTTP::ClientRequest> request = make_shared<HTTP::ClientRequest>();
    request->header()->url = "http://218.88.22.83:800/";
   // request->header()->url = "http://admin:ms111111@192.168.15.119:80/cgi-bin/admin/admin.cgi?action=get.system.information&format=inf";
    request->timeout() = 10000;

    client->request(httpasync, request, httpAscynCallback);

    while (1)
    {
        Thread::sleep(1000);
    }
    return 0;
}
#endif

#if 1

#include <windows.h>

#define IDR_PAUSE 12
#define IDR_START 13
LPCTSTR szAppClassName = TEXT("服务程序");
LPCTSTR szAppWindowName = TEXT("服务程序");
HMENU hmenu;//菜单句柄

NOTIFYICONDATA nid;
BOOL ShowBalloonTip(LPCTSTR szMsg, LPCTSTR szTitle, DWORD dwInfoFlags = NIIF_INFO, UINT uTimeout = 1000)
{
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.uFlags = NIF_INFO;
	nid.uVersion = NOTIFYICON_VERSION;
	nid.uTimeout = uTimeout;
	nid.dwInfoFlags = dwInfoFlags;
	strcpy(nid.szInfo, szMsg ? szMsg : (""));
	strcpy(nid.szInfoTitle, szTitle ? szTitle : (""));

	return 0 != Shell_NotifyIcon(NIM_MODIFY, &nid);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	
	UINT WM_TASKBARCREATED;
	POINT pt;//用于接收鼠标坐标
	int xx;//用于接收菜单选项返回值

	// 不要修改TaskbarCreated，这是系统任务栏自定义的消息
	WM_TASKBARCREATED = RegisterWindowMessage(TEXT("TaskbarCreated"));
	switch (message)
	{
	case WM_CREATE://窗口创建时候的消息.
		nid.cbSize = sizeof(nid);
		nid.hWnd = hwnd;
		nid.uID = 0;
		nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
		nid.uCallbackMessage = WM_USER;
		nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		lstrcpy(nid.szTip, szAppClassName);
		Shell_NotifyIcon(NIM_ADD, &nid);
		hmenu = CreatePopupMenu();//生成菜单
		AppendMenu(hmenu, MF_STRING, IDR_PAUSE, "暂停服务");//为菜单添加两个选项
		AppendMenu(hmenu, MF_STRING, IDR_START, "关于");
		break;
	case WM_USER://连续使用该程序时候的消息.
		if (lParam == WM_LBUTTONDOWN)
		{
			ShowBalloonTip("111", "2222");
		//	MessageBox(hwnd, TEXT("Win32 API 实现系统托盘程序,双击托盘可以退出!"), szAppClassName, MB_OK);
		}
		if (lParam == WM_LBUTTONDBLCLK)//双击托盘的消息,退出.
			SendMessage(hwnd, WM_CLOSE, wParam, lParam);
		if (lParam == WM_RBUTTONDOWN)
		{
			GetCursorPos(&pt);//取鼠标坐标
			::SetForegroundWindow(hwnd);//解决在菜单外单击左键菜单不消失的问题
			EnableMenuItem(hmenu, IDR_PAUSE, MF_GRAYED);//让菜单中的某一项变灰
			xx = TrackPopupMenu(hmenu, TPM_RETURNCMD, pt.x, pt.y, NULL, hwnd, NULL);//显示菜单并获取选项ID
			if (xx == IDR_PAUSE) MessageBox(hwnd, TEXT("111"), szAppClassName, MB_OK);
			if (xx == IDR_START) MessageBox(hwnd, TEXT("222"), szAppClassName, MB_OK);
			if (xx == 0) PostMessage(hwnd, WM_LBUTTONDOWN, NULL, NULL);
			//MessageBox(hwnd, TEXT("右键"), szAppName, MB_OK);
		}
		break;
	case WM_DESTROY://窗口销毁时候的消息.
		Shell_NotifyIcon(NIM_DELETE, &nid);
		PostQuitMessage(0);
		break;
	default:
		/*
		* 防止当Explorer.exe 崩溃以后，程序在系统系统托盘中的图标就消失
		*
		* 原理：Explorer.exe 重新载入后会重建系统任务栏。当系统任务栏建立的时候会向系统内所有
		* 注册接收TaskbarCreated 消息的顶级窗口发送一条消息，我们只需要捕捉这个消息，并重建系
		* 统托盘的图标即可。
		*/
		if (message == WM_TASKBARCREATED)
			SendMessage(hwnd, WM_CREATE, wParam, lParam);
		break;
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}

int main()
{
	HWND hwnd;
	MSG msg;
	WNDCLASS wndclass;

	HWND handle = FindWindow(NULL, szAppWindowName);
	if (handle != NULL)
	{
		MessageBox(NULL, TEXT("Application is already running"), szAppClassName, MB_ICONERROR);
		return 0;
	}

	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = NULL;
	wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = szAppClassName;

	if (!RegisterClass(&wndclass))
	{
		MessageBox(NULL, TEXT("This program requires Windows NT!"), szAppClassName, MB_ICONERROR);
		return 0;
	}

	// 此处使用WS_EX_TOOLWINDOW 属性来隐藏显示在任务栏上的窗口程序按钮
	hwnd = CreateWindowEx(WS_EX_TOOLWINDOW,
		szAppClassName, szAppWindowName,
		WS_POPUP,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL, NULL, NULL, NULL);

	ShowWindow(hwnd, 0);
	UpdateWindow(hwnd);

	//消息循环
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}

#endif

#if 1
#include <windows.h>
#include <iostream>
#pragma comment(lib, "Advapi32")
SERVICE_STATUS ServiceStatus;
SERVICE_STATUS_HANDLE hStatus;
void ServiceMain(int argc, char** argv);
void ControlHandler(DWORD request);
int InitService();
bool AddService(char*serviceName)
{
	SERVICE_TABLE_ENTRY ServiceTable[2];
	ServiceTable[0].lpServiceName = serviceName;
	ServiceTable[0].lpServiceProc = (LPSERVICE_MAIN_FUNCTION)ServiceMain;

	ServiceTable[1].lpServiceName = NULL;
	ServiceTable[1].lpServiceProc = NULL;

	StartServiceCtrlDispatcher(ServiceTable);
	return true;
}
void main1(int argc, char* argv[])
{

	char serviceName[MAX_PATH] = "2";
	AddService(serviceName);
	SC_HANDLE hSCManager = NULL; /*服务控制管理器句柄*/
	SC_HANDLE hService = NULL;     //服务句柄
	char szExePath[MAX_PATH] = "F:\\vs2013\\Example\\Project3\\Release\\Project3.exe";
	if ((hSCManager = OpenSCManager(NULL,  /*NULL表明是本地主机*/ NULL, /* 要打开的服务控制管理数据库，默认为空*/SC_MANAGER_CREATE_SERVICE/*创建权限*/)) == NULL)
	{
		std::cout << "openSCManager fail!" << std::endl;
		return;
	}
	//system("pause");
	//strcat_s(szExePath,sizeof("main.exe"),"main.exe");  //应用程序绝对路径
	hService = OpenService(hSCManager, serviceName, SERVICE_ALL_ACCESS);
	if ((hService = CreateService(hSCManager,  //指向服务控制管理数据库的句柄
		serviceName,    //服务名
		"2", //显示用的服务名
		SERVICE_ALL_ACCESS, //所有访问权限
		SERVICE_WIN32_OWN_PROCESS, //私有类型
		SERVICE_DEMAND_START, //自启动类型        
		SERVICE_ERROR_IGNORE, //忽略错误处理
		szExePath,  //应用程序路径
		NULL,
		NULL,
		NULL,
		NULL,
		NULL)) == NULL)
	{
		std::cout << " CreateService" << GetLastError() << std::endl;
		return;
	}
	if (StartService(hService, 0, NULL) == FALSE)
	{
		std::cout << "StartService failed:" << GetLastError() << std::endl;
		return;
	}
	std::cout << "Install service successfully" << std::endl;
	CloseServiceHandle(hService);  //关闭服务句柄
	CloseServiceHandle(hSCManager); //关闭服务管理数据库句柄
}

void ServiceMain(int argc, char** argv)
{
	int error;
	ServiceStatus.dwServiceType = SERVICE_WIN32;
	ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
	ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
	ServiceStatus.dwWin32ExitCode = 0;
	ServiceStatus.dwServiceSpecificExitCode = 0;
	ServiceStatus.dwCheckPoint = 0;
	ServiceStatus.dwWaitHint = 0;

	hStatus = RegisterServiceCtrlHandler("MemoryStatus", (LPHANDLER_FUNCTION)ControlHandler);
	if (hStatus == (SERVICE_STATUS_HANDLE)0)
	{
		// Registering Control Handler failed
		return;
	}
	// Initialize Service 
	error = InitService();
	if (!error)
	{
		// Initialization failed
		ServiceStatus.dwCurrentState = SERVICE_STOPPED;
		ServiceStatus.dwWin32ExitCode = -1;
		SetServiceStatus(hStatus, &ServiceStatus);
		return;
	}
	// We report the running status to SCM. 
	ServiceStatus.dwCurrentState = SERVICE_RUNNING;
	SetServiceStatus(hStatus, &ServiceStatus);


	// The worker loop of a service
	while (ServiceStatus.dwCurrentState == SERVICE_RUNNING)
	{


	}
	return;
}

void ControlHandler(DWORD request)
{
	switch (request)
	{
	case SERVICE_CONTROL_STOP:
		ServiceStatus.dwWin32ExitCode = 0;
		ServiceStatus.dwCurrentState = SERVICE_STOPPED;
		SetServiceStatus(hStatus, &ServiceStatus);
		return;


	case SERVICE_CONTROL_SHUTDOWN:
		ServiceStatus.dwWin32ExitCode = 0;
		ServiceStatus.dwCurrentState = SERVICE_STOPPED;
		SetServiceStatus(hStatus, &ServiceStatus);
		return;


	default:
		break;
	}

	// Report current status
	SetServiceStatus(hStatus, &ServiceStatus);
	return;
}

int InitService() {

	return true;
}
#endif
