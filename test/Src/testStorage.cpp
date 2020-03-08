#include "RTSP/RTSP.h"
#include "MSStorage/MSStorage.h"
using namespace Public::RTSP;
using namespace Milesight::Storage;

#if 0

shared_ptr<MSStorage> storage = NULL;

class RTSPClintSessiontmp11 :public RTSPClientHandler
{
public:
	uint64_t					 devid;

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

	virtual void onRTPFrameCallback(shared_ptr<RTSPCommandSender>& sender, const shared_ptr<STREAM_TRANS_INFO>& transport, const shared_ptr<RTPFrame>& frame)
	{
		shared_ptr<MSFrameInfo> msframe = make_shared<MSFrameInfo>();
		msframe->codeid = frame->codeId();
		msframe->frametype = frame->frameType();
		msframe->timestmap = frame->timestmap();
		msframe->data = frame->framedata();

		storage->inputStream(devid,MSProtoStreamType_Main ,msframe, 0);
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
	shared_ptr<RTSPClintSessiontmp11> handler;
	shared_ptr<RTSPClient> client;
};

string rtspaddr1[] = {
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

class MSMSStorageEvent1:public MSMSStorageEvent
{
public:
	MSMSStorageEvent1() {}
	virtual ~MSMSStorageEvent1() {}

	//磁盘文件事件
	virtual void onEvent(MSStorageEventType event, const std::string& name, const std::string& filename, const ErrorInfo& err)
	{
		int a = 0;
	}

	//存储事件
	virtual void onStorageEvent(const DeviceId& devid, MSStorageEventType event, const ErrorInfo& err)
	{
		int a = 0;
	}
};


void doThreadProc(Thread* thread, void* param)
{
	uint32_t devid = (uint32_t)param;

	while (thread->looping())
	{
		storage->startPrerecord(devid, MSProtoStreamType_Main, 10, 0);

#define FRAMESIZE		1024*50

		shared_ptr<MSFrameInfo> info = make_shared<MSFrameInfo>();
		info->data.alloc(FRAMESIZE);
		info->data.resize(FRAMESIZE);
		info->codeid = CodeID_Video_H264;


		uint32_t index = 0;
		bool isrecording = false;
		while (1)
		{
			if (index++ % 25 == 0)
			{
				info->frametype = FrameType_Video_IFrame;
			}
			else
			{
				info->frametype = FrameType_Video_BFrame;
			}

			info->timestmap += index * 40;

			if (index >= 100 && !isrecording)
			{
				//storage->stopPrerecord(1, MSProtoStreamType_Main);

				storage->startRecord(devid, MSProtoStreamType_Main,0);

				isrecording = true;
			}

			storage->inputStream(devid, MSProtoStreamType_Main, info, 0);

			if (index >= 3000)
			{
				storage->stopRecord(devid, MSProtoStreamType_Main);

				isrecording = false;
				index = 0;
			}

			Thread::sleep(40);
		}
	}
}

void closeEvent(void*, BaseSystem::CloseEvent)
{
	storage = NULL;
}

int runStorage()
{
	BaseSystem::init(closeEvent);
	
	shared_ptr< MSMSStorageEvent> event = make_shared<MSMSStorageEvent1>();

	storage = make_shared<MSStorage>(event);

	storage->start(8);

	{
		MSVolumeConfig config;
		config.enable = true;
		config.reserved = 10;

	//	storage->setVolumeConfig("d", config);
		storage->setVolumeConfig("e", config);
	}

	Thread::sleep(2000);

	std::list<shared_ptr<Thread> > threadlist;

	for (int i = 0; i < 50; i++)
	{
		shared_ptr<Thread> thread = ThreadEx::creatThreadEx("1", doThreadProc, (void*)(i + 1));
		thread->createThread();

		threadlist.push_back(thread);
	}

	ConsoleCommand::runCmd("VMS");

	return 0;
}


DeviceId devid = 1001;


int runStorage_save()
{

	BaseSystem::init(closeEvent);

	shared_ptr< MSMSStorageEvent> event = make_shared<MSMSStorageEvent1>();

	storage = make_shared<MSStorage>(event);

	storage->start(8);

	{
		MSVolumeConfig config;
		config.enable = true;
		config.reserved = 10;

		//	storage->setVolumeConfig("d", config);
		storage->setVolumeConfig("e", config);
	}

	const char* filename[] = {"test_0.dat","test_1.dat"};

	Thread::sleep(2000);

	for (int i = 0; i < sizeof(filename) / sizeof(const char*); i++)
	{
		logdebug("open %s",filename[i]);
		FILE* fd = fopen(filename[i], "rb");
		if (fd == NULL)
		{
			return -1;
		}

		storage->startRecord(devid, MSProtoStreamType_Main, 0);
		int index = 0;
		while (1)
		{
			shared_ptr<MSFrameInfo> info = make_shared<MSFrameInfo>();
			info->codeid = CodeID_Video_H264;
			char* buf = info->data.alloc(FRAMESIZE);

			int readlen = fread(buf, 1, FRAMESIZE, fd);
			if (readlen == 0) break;

			info->data.resize(readlen);

			if (index++ % 25 == 0)
			{
				info->frametype = FrameType_Video_IFrame;
			}
			else
			{
				info->frametype = FrameType_Video_BFrame;
			}

			info->timestmap += index * 10;


			storage->inputStream(devid, MSProtoStreamType_Main, info, 0);
			Thread::sleep(10);

		}
		storage->stopRecord(devid, MSProtoStreamType_Main);

		logdebug("close %s", filename[i]);

		Thread::sleep(20000);
	}
	

	int rindex = 0;
	while (rindex++ < 10) Thread::sleep(1000);

	return 0;
}

int runStorage_read()
{
	BaseSystem::init(closeEvent);

	shared_ptr< MSMSStorageEvent> event = make_shared<MSMSStorageEvent1>();

	storage = make_shared<MSStorage>(event);

	storage->start(8);
	
	Thread::sleep(2000);

	std::list<MSBookRecordInfo> recordlist;

	uint64_t starttime = Time::getCurrentTime().makeTime() - 60 * 60 * 5;
	uint64_t stoptime = starttime + 60 * 60 * 24;

	ErrorInfo err = storage->searchRecord(devid, starttime, stoptime,recordlist);
	assert(!err && recordlist.size() > 0);


	
	uint32_t index = 0;

	for(std::list<MSBookRecordInfo>::iterator iter = recordlist.begin();iter != recordlist.end();iter++,index++)
	{
		char buffer[128];
		snprintf(buffer,127, "output_%d.dat", index);

		FILE* fd = fopen(buffer, "wb+");
		if (fd == NULL)
		{
			return -1;
		}

		shared_ptr<MSRecordStream> stream = storage->createRecordStream(devid, iter->streamtype);
		assert(stream);
		err = stream->open(iter->startTime, iter->stopTime);
		assert(!err);

		while (1)
		{
			std::vector<shared_ptr<MSFrameInfo>> gop = stream->readGop(ReadType_Forward);
			if (gop.size() <= 0) break;

			for (size_t i = 0; i < gop.size(); i++)
			{
				fwrite(gop[i]->data.c_str(), 1, gop[i]->data.length(), fd);
			}
		}
		fclose(fd);
	}
	
	
	return 0;
}

int runClient(const std::string& ipaddr,const std::list<std::string>& rtsplist)
{
	BaseSystem::init(closeEvent);

	shared_ptr< MSMSStorageEvent> event = make_shared<MSMSStorageEvent1>();

	storage = make_shared<MSStorage>(event);

	storage->start(8);

	{
		MSVolumeConfig config;
		config.enable = true;
		config.reserved = 10;

		//	storage->setVolumeConfig("d", config);
		storage->setVolumeConfig("e", config);
	}

	Thread::sleep(2000);




	shared_ptr<IOWorker>	worker = make_shared<IOWorker>(4);
	shared_ptr<RTSPClientManager> manager = make_shared<RTSPClientManager>(worker,"test");



	std::list<std::string> rtsplisttmp;

	//for (int i = 150; i < 250; i++)
	//{
	//	char buffer[255];
	//	snprintf_x(buffer, 255, "rtsp://admin:ms111111@192.168.15.%d:554/main", i);

	//	rtsplisttmp.push_back(buffer);
	//	//break;
	//}


	std::list< RTSPClientInfo> clientlist;

	for (std::list<std::string>::const_iterator iter = rtsplisttmp.begin(); iter != rtsplisttmp.end(); iter++)
	{
		Thread::sleep(100);

		RTSPClientInfo info;
		info.handler = make_shared<RTSPClintSessiontmp11>();
		info.client = manager->create(info.handler, RTSPUrl(*iter));
		info.client->initRTPOverTcpType();
		ErrorInfo err = info.client->start(10000);

		info.handler->devid = Hash::bkdr(*iter);

		storage->startRecord(info.handler->devid, MSProtoStreamType_Main, 0);


		if (err)
		{
			logdebug("%s 111111",*iter);

			continue;
		}

		clientlist.push_back(info);
	}

	int times = 0;
	while (1)// times++ < 60 * 5)
	{
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
	//	uint32_t pktloss = statistics.needCountPackage == 0 ? 0 : (statistics.needCountPackage - statistics.realRecvPackage) * 100 / statistics.needCountPackage;
	//	logdebug("RTSPDCFactory devicesize %d needrecv %llu realrecv %llu pktloss %llu pktlossrate %d", clientlist.size(), statistics.needCountPackage, statistics.realRecvPackage, statistics.needCountPackage - statistics.realRecvPackage, pktloss);

		Thread::sleep(1000);
	}

	clientlist.clear();

	storage->stop();

	return 0;
}

#endif