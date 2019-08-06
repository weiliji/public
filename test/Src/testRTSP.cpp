#include "RTSP/RTSP.h"
using namespace Public::RTSP;

#if 1

class RTSPClintSessiontmp1 :public RTSPClientHandler
{
	virtual void onConnectResponse(bool success, const std::string& errmsg) 
	{
		int a = 0;
	}

	virtual void onDescribeResponse(const shared_ptr<RTSPCommandInfo>& cmdinfo, const shared_ptr<MEDIA_INFO>& info) 
	{
		info->cleanExStreamInfo();
		int a = 0;
	}
	virtual void onSetupResponse(const shared_ptr<RTSPCommandInfo>& cmdinfo, const shared_ptr<STREAM_TRANS_INFO>& transport)
	{
		int a = 0;
	}
	virtual void onPlayResponse(const shared_ptr<RTSPCommandInfo>& cmdinfo)
	{
		int a = 0;
	}
	virtual void onPauseResponse(const shared_ptr<RTSPCommandInfo>& cmdinfo)
	{
		int a = 0;
	}
	virtual void onGetparameterResponse(const shared_ptr<RTSPCommandInfo>& cmdinfo, const std::string& content)
	{
		int a = 0;
	}
	virtual void onTeradownResponse(const shared_ptr<RTSPCommandInfo>& cmdinfo)
	{
		int a = 0;
	}

	virtual void onErrorResponse(const shared_ptr<RTSPCommandInfo>& cmdinfo, int statuscode, const std::string& errmsg)
	{
		int a = 0;
	}

	virtual void onClose(const std::string& errmsg)
	{
		int a = 0;
	}
	virtual void onMediaPackageCallback(const shared_ptr<STREAM_TRANS_INFO> mediainfo, const RTPHEADER& rtpheader, const StringBuffer& buffer)
	{
		int a = 0;
	}
	virtual void onContorlPackageCallback(const shared_ptr<STREAM_TRANS_INFO> mediainfo, const char*  buffer, uint32_t bufferlen) 
	{
		int a = 0;
	}
};

#define MAXTESTRTSPCLIENT		50

struct RTSPClientInfo
{
	shared_ptr<RTSPClientHandler> handler;
	shared_ptr<RTSPClient> client;
};

string rtspaddr[] = {
	"rtsp://admin:ms123456@192.168.7.104:554/main",
	"rtsp://admin:ms123456@192.168.3.135:554/main",
	"rtsp://192.168.9.230:554/cam/realmonitor?channel=1&subtype=0&unicast=true&proto=Onvif",
	"rtsp://admin:support2019@192.168.9.205:554/Streaming/Channels/102",
	"rtsp://admin:ms123456@192.168.10.230:554/main",
	"rtsp://admin:ms123456@192.168.11.230:554/main",
	"rtsp://admin:ms123456@192.168.4.150:554/main",
	"rtsp://admin:ms123456@192.168.4.111:554/main",
	"rtsp://admin:ms123456@192.168.2.172:554/main",
	"rtsp://admin:ms123456@192.168.4.105:554/main",
	"rtsp://admin:ms123456@192.168.10.236:554/main",
	"rtsp://192.168.2.46:5554/111",
};

int runClient(const std::string& ipaddr,const std::list<std::string>& rtsplist)
{
	shared_ptr<IOWorker>	worker = make_shared<IOWorker>(8);
	shared_ptr<RTSPClientManager> manager = make_shared<RTSPClientManager>(worker,"test");


	std::list<std::string> rtsplisttmp;
	for(std::list<std::string>::const_iterator iter= rtsplist.begin();iter != rtsplist.end();iter++)
	{ 
		std::string rtspaddr = "rtsp://" + ipaddr + ":5554/" + Base64::encode(*iter);

		rtsplisttmp.push_back(rtspaddr);
	}

	if (0)
	{
		rtsplisttmp.clear();

		uint32_t rtspaddrsize = sizeof(rtspaddr) / sizeof(string);
		for (int i = 0; i < rtspaddrsize* MAXTESTRTSPCLIENT; i++)
		{
			rtsplisttmp.push_back(rtspaddr[i% rtspaddrsize]);
		}
	}

	std::list< RTSPClientInfo> clientlist;

	for (std::list<std::string>::const_iterator iter = rtsplisttmp.begin(); iter != rtsplisttmp.end(); iter++)
	{
		Thread::sleep(1000);

		RTSPClientInfo info;
		info.handler = make_shared<RTSPClintSessiontmp1>();
		info.client = manager->create(info.handler, RTSPUrl(*iter));
		//	info.client->initRTPOverUdpType();
		info.client->start(10000);

		clientlist.push_back(info);
	}


	getchar();

	return 0;
}

#endif