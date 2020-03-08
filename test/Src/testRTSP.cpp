#include "RTSP/RTSP.h"
using namespace Public::RTSP;

#if 0

class RTSPClintSessiontmp1 :public RTSPClientHandler
{
public:
	shared_ptr<RTSP_Media_Infos> mediainfo;

	virtual void onConnectResponse(shared_ptr<RTSPCommandSender>& sender, const ErrorInfo& err) 
	{
		int a = 0;
	}
	virtual void onErrorResponse(shared_ptr<RTSPCommandSender>& sender, const shared_ptr<RTSPCommandInfo>& cmdinfo, int errcode, const std::string& errmsg)
	{
		int a = 0;
	}

	virtual void onOptionResponse(shared_ptr<RTSPCommandSender>& sender, const shared_ptr<RTSPCommandInfo>& cmdinfo, const ErrorInfo& err, const std::string& optstr)
	{
		int a = 0;
	}
	virtual void onDescribeResponse(shared_ptr<RTSPCommandSender>& sender, const shared_ptr<RTSPCommandInfo>& cmdinfo, const shared_ptr<RTSP_Media_Infos>& info) 
	{
		info->cleanExStreamInfo(); 

		mediainfo = info;
	}
	virtual void onSetupResponse(shared_ptr<RTSPCommandSender>& sender, const shared_ptr<RTSPCommandInfo>& cmdinfo, const shared_ptr<STREAM_TRANS_INFO>& transport)
	{
		int a = 0;
	}
	virtual void onPlayResponse(shared_ptr<RTSPCommandSender>& sender, const shared_ptr<RTSPCommandInfo>& cmdinfo)
	{
		int a = 0;
	}
	virtual void onPauseResponse(shared_ptr<RTSPCommandSender>& sender, const shared_ptr<RTSPCommandInfo>& cmdinfo, const ErrorInfo& err)
	{
		int a = 0;
	}
	virtual void onGetparameterResponse(shared_ptr<RTSPCommandSender>& sender, const shared_ptr<RTSPCommandInfo>& cmdinfo, const std::string& content)
	{
		int a = 0;
	}
	virtual void onTeradownResponse(shared_ptr<RTSPCommandSender>& sender, const shared_ptr<RTSPCommandInfo>& cmdinfo)
	{
		int a = 0;
	}

	virtual void onRTPPackageCallback(shared_ptr<RTSPCommandSender>& sender, const shared_ptr<STREAM_TRANS_INFO>& transport, const shared_ptr<RTPPackage>& rtppackge)
	{
		int a = 0;
	}
	virtual void onRTCPPackageCallback(shared_ptr<RTSPCommandSender>& sender, const shared_ptr<STREAM_TRANS_INFO>& transport, const shared_ptr<RTCPPackage>& rtcppackge)
	{
		int a = 0;
	}

	virtual void onClose(shared_ptr<RTSPCommandSender>& sender, const ErrorInfo& err) 
		{
			int a = 0;
		}
};

#define MAXTESTRTSPCLIENT		1

struct RTSPClientInfo
{
	shared_ptr<RTSPClintSessiontmp1> handler;
	shared_ptr<RTSPClient> client;
};

string rtspaddr[] = {
	"rtsp://admin:ms111111@192.168.15.100:554/main",
//	"rtsp://admin:ms123456@192.168.3.135:554/main",
//	"rtsp://192.168.9.230:554/cam/realmonitor?channel=1&subtype=0&unicast=true&proto=Onvif",
//	"rtsp://admin:support2019@192.168.9.205:554/Streaming/Channels/102",
	//"rtsp://admin:ms123456@192.168.10.230:554/main",
	//"rtsp://admin:ms123456@192.168.11.230:554/main",
	//"rtsp://admin:ms123456@192.168.4.150:554/main",
	//"rtsp://admin:ms123456@192.168.4.111:554/main",
	//"rtsp://admin:ms123456@192.168.2.172:554/main",
	//"rtsp://admin:ms123456@192.168.4.105:554/main",
	//"rtsp://admin:ms123456@192.168.10.236:554/main",
	//"rtsp://192.168.2.46:5554/111",
};

int runClient22(const std::string& ipaddr,const std::list<std::string>& rtsplist)
{
	shared_ptr<IOWorker>	worker = make_shared<IOWorker>(4);
	shared_ptr<RTSPClientManager> manager = make_shared<RTSPClientManager>(worker,"test");


	std::vector<std::string> rtsplisttmp;
	for(std::list<std::string>::const_iterator iter= rtsplist.begin();iter != rtsplist.end();iter++)
	{ 
		std::string rtspaddr = "rtsp://" + ipaddr + ":5554/" + Base64::encode(*iter);

		rtsplisttmp.push_back(rtspaddr);
	}

	if(1)
//	if (0)
	{
		rtsplisttmp.clear();

		/*uint32_t rtspaddrsize = sizeof(rtspaddr) / sizeof(string);
		for (int i = 0; i < rtspaddrsize* MAXTESTRTSPCLIENT; i++)
		{
			rtsplisttmp.push_back(rtspaddr[i% rtspaddrsize]);
		}*/

		for (int i = 150; i < 250; i++)
		{
			char buffer[255];
			snprintf_x(buffer, 255, "rtsp://admin:ms111111@192.168.15.%d:554/main", i);

			rtsplisttmp.push_back(buffer);
		}
	}

	std::list< RTSPClientInfo> clientlist;
	uint32_t index = 0;
	while (1)
	{
		while (clientlist.size() >= 25)
		{
			clientlist.pop_front();
		}
		while (clientlist.size() < 30)
		{
			RTSPClientInfo info;
			info.handler = make_shared<RTSPClintSessiontmp1>();
			info.client = manager->create(info.handler, RTSPUrl(rtsplisttmp[index ++ % rtsplisttmp.size()]));
			info.client->initRTPOverTcpType();
			ErrorInfo err = info.client->start(10000);


			clientlist.push_back(info);
		}

		RTSPStatistics::RecvStatistics statistics;
		for(std::list< RTSPClientInfo>::iterator iter = clientlist.begin();iter != clientlist.end();iter++)
		{
			shared_ptr<RTSP_Media_Infos> mediainfo = iter->handler->mediainfo;
			if(mediainfo == NULL) continue;

			for (std::list< shared_ptr<STREAM_TRANS_INFO> >::iterator siter = mediainfo->infos.begin(); siter != mediainfo->infos.end(); siter++)
			{
				shared_ptr<RTSPStatistics> rtspstatis = (*siter)->rtspstatistics.lock();
				if (rtspstatis == NULL) continue;

				RTSPStatistics::RecvStatistics tmp;
				rtspstatis->getRecvStatistics(tmp);

				statistics.needCountPackage += tmp.needCountPackage;
				statistics.realRecvPackage += tmp.realRecvPackage;
			}		
		}
		uint32_t pktloss = statistics.needCountPackage == 0 ? 0 : (statistics.needCountPackage - statistics.realRecvPackage) * 100 / statistics.needCountPackage;
		logdebug("RTSPDCFactory devicesize %d needrecv %llu realrecv %llu pktloss %llu pktlossrate %d", clientlist.size(), statistics.needCountPackage, statistics.realRecvPackage, statistics.needCountPackage - statistics.realRecvPackage, pktloss);

		Thread::sleep(1000);
	}

	return 0;
}

#endif