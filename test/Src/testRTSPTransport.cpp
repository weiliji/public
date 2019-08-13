#include "RTSP/RTSP.h"
using namespace Public::RTSP;

#if 1
class RTSPServerSessiontmp;
class RTSPClintessiontmp;

Mutex													g_mutex;
std::map<std::string, shared_ptr<RTSPClintessiontmp> > g_clientlist;
std::map< RTSPServerSession*, shared_ptr< RTSPServerSessiontmp> >	g_totalserverlist;



class RTSPServerSessiontmp :public RTSPServerHandler
{
	
public:
	RTSPServerSessiontmp():isplaying(false)
	{
	}
	~RTSPServerSessiontmp()
	{
		int a = 0;
	}

	shared_ptr<RTSPServerSession> session;
	bool						  isplaying;
	shared_ptr<MEDIA_INFO>		 mediainfo;
	shared_ptr<RTSPClintessiontmp>	client;

	shared_ptr<STREAM_TRANS_INFO>	videotrans;

	virtual void onOptionRequest(const shared_ptr<RTSPServerSession>& session, const shared_ptr<RTSPCommandInfo>& cmdinfo)
	{
		session->sendOptionResponse(cmdinfo);
	}
	virtual void onDescribeRequest(const shared_ptr<RTSPServerSession>& session, const shared_ptr<RTSPCommandInfo>& cmdinfo);

	virtual void onPlayRequest(const shared_ptr<RTSPServerSession>& session, const shared_ptr<RTSPCommandInfo>& cmdinfo, const shared_ptr<MEDIA_INFO>& _mediainfo, const RANGE_INFO& range)
	{
		session->sendPlayResponse(cmdinfo);
		isplaying = true;
		mediainfo = _mediainfo;

		videotrans = mediainfo->videoStreamInfo();

		/*Guard locker(mutex);
		for (std::list< RTSPBufferInfo>::iterator iter = cache.begin(); iter != cache.end(); iter++)
		{
			session->sendMediaPackage(mediainfo->videoStreamInfo(), iter->timestmap, iter->buffer, iter->mark);
		}*/
	}
	virtual void onPauseRequest(const shared_ptr<RTSPServerSession>& session, const shared_ptr<RTSPCommandInfo>& cmdinfo)
	{
		session->sendErrorResponse(cmdinfo, 500, "NOT SUPPORT");
	}
	virtual void onTeardownRequest(const shared_ptr<RTSPServerSession>& session, const shared_ptr<RTSPCommandInfo>& cmdinfo)
	{
		session->sendTeardownResponse(cmdinfo);
	}
	virtual void onGetparameterRequest(const shared_ptr<RTSPServerSession>& session, const shared_ptr<RTSPCommandInfo>& cmdinfo, const std::string& content) { session->sendErrorResponse(cmdinfo, 500, "NOT SUPPORT"); }

	virtual void onClose(const shared_ptr<RTSPServerSession>& session, const std::string& errmsg);
};
class RTSPClintessiontmp :public RTSPClientHandler
{
public:
	Mutex		clientmutex;
	std::map< RTSPServerSession*, shared_ptr< RTSPServerSessiontmp> > serverlist;
	shared_ptr<MEDIA_INFO>		sourcemediainfo;


	shared_ptr<RTSPClient>		clienttmp;

	virtual void onConnectResponse(bool success, const std::string& errmsg) 
	{
		int a = 0;
	}

	virtual void onDescribeResponse(const shared_ptr<RTSPCommandInfo>& cmdinfo, const shared_ptr<MEDIA_INFO>& info)
	{
		sourcemediainfo = info;
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
	virtual void onMediaPackageCallback(const shared_ptr<STREAM_TRANS_INFO> mediainfo, const RTPPackage& rtppackage)
	{
		std::map< RTSPServerSession*, shared_ptr< RTSPServerSessiontmp> > sendlist;
		
		if (strcasecmp(mediainfo->streaminfo.szMediaName.c_str(), "video") != 0) return;

		{
			Guard locker(clientmutex);
			sendlist = serverlist;
		}

		for (std::map< RTSPServerSession*, shared_ptr< RTSPServerSessiontmp> >::iterator iter = sendlist.begin(); iter != sendlist.end(); iter++)
		{
			shared_ptr< RTSPServerSessiontmp> tmp = iter->second;
			if(tmp == NULL || !tmp->isplaying) continue;

			shared_ptr<RTSPServerSession> session = tmp->session;
			if(session == NULL) continue;

			shared_ptr<STREAM_TRANS_INFO> trans = tmp->videotrans;
			if(trans == NULL) continue;

			session->sendMediaPackage(trans, rtppackage);
		}

		int a = 0;
	}
};

void RTSPServerSessiontmp::onDescribeRequest(const shared_ptr<RTSPServerSession>& session, const shared_ptr<RTSPCommandInfo>& cmdinfo)
{
	shared_ptr<RTSPClintessiontmp> client;

	URL url(session->url().rtspurl);

	std::string urlstr = url.pathname;

	{
		Guard locker(g_mutex);
		std::map<std::string, shared_ptr<RTSPClintessiontmp> >::iterator iter = g_clientlist.find(urlstr);
		if (iter != g_clientlist.end())
		{
			client = iter->second;
		}
	}

	if (client == NULL || client->sourcemediainfo == NULL)
	{
		session->sendErrorResponse(cmdinfo, 500, "Source Not Find");
	}
	else
	{
		MEDIA_INFO info = client->sourcemediainfo->cloneStreamInfo();
		//info.bHasVideo = true;
		//info.bHasAudio = true;

		session->sendDescribeResponse(cmdinfo, info);


		shared_ptr< RTSPServerSessiontmp> servertmp;
		{
			std::map< RTSPServerSession*, shared_ptr< RTSPServerSessiontmp> >::iterator iter = g_totalserverlist.find(session.get());
			if (iter != g_totalserverlist.end())
			{
				servertmp = iter->second;
			}
		}

		if(servertmp)
		{
			servertmp->client = client;

			Guard locker(client->clientmutex);
			client->serverlist[session.get()] = servertmp;
		}
	}
}

void RTSPServerSessiontmp::onClose(const shared_ptr<RTSPServerSession>& session, const std::string& errmsg)
{
	shared_ptr< RTSPServerSessiontmp> tmp;
	{
		Guard locker(g_mutex);
		std::map< RTSPServerSession*, shared_ptr< RTSPServerSessiontmp> >::iterator iter = g_totalserverlist.find(session.get());
		if (iter != g_totalserverlist.end())
		{
			tmp = iter->second;
			g_totalserverlist.erase(iter);
		}
	}
	if (client)
	{
		Guard locker(client->clientmutex);
		client->serverlist.erase(session.get());
	}
}


struct RTSPClientInfo
{
	shared_ptr<RTSPClientHandler> handler;
	shared_ptr<RTSPClient> client;
};

static shared_ptr<RTSPServerHandler> rtspAceeptCallback(const shared_ptr<RTSPServerSession>& server)
{
	Guard locker(g_mutex);

	shared_ptr< RTSPServerSessiontmp> servertmp = make_shared<RTSPServerSessiontmp>();
	servertmp->session = server;

	g_totalserverlist[server.get()] = servertmp;


	return servertmp;
}


int runserver(const std::list<std::string>& rtsplist)
{
	shared_ptr<IOWorker>	worker = make_shared<IOWorker>(Host::getProcessorNum()*2);
	shared_ptr<RTSPClientManager> manager = make_shared<RTSPClientManager>(worker,"test");

	
	for(std::list<std::string>::const_iterator iter = rtsplist.begin();iter != rtsplist.end();iter ++)
	{
		shared_ptr<RTSPClintessiontmp> handler = make_shared<RTSPClintessiontmp>();
		handler->clienttmp = manager->create(handler,RTSPUrl(*iter));

		handler->clienttmp->initRTPOverUdpType();
		handler->clienttmp->start(10000);

		Guard locker(g_mutex);

		std::string urlflag = Base64::encode(*iter);

		g_clientlist["/"+urlflag] = handler;
	}

	shared_ptr<RTSPServer> servermanager = make_shared<RTSPServer>(worker, "test");
	servermanager->run(5554, rtspAceeptCallback);

	getchar();

	return 0;
}

#endif