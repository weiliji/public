#include "Network/HTTP/Server.h"
#include "Network/Socket/TcpServer.h"
#include "HTTPCommunication.h"
namespace Public {
namespace Network{
namespace HTTP {


#define COMMUFINISHTIMEOUT		5*1000
#define NOSESSIONTIMEOUT		2*60*1000

struct ListenInfo
{
	Server::HTTPCallback	callback;
	CacheType				type;
};

struct HTTPSessionInfo
{
	shared_ptr<Communication>			commu;
	weak_ptr<WebSocketSession>			websocketsession;
	shared_ptr<ServerSession>			httpsession;
	weak_ptr<ServerSession>				httpsessionback;
	ListenInfo							listeninfo;
	uint64_t							finishtime;
	uint64_t							nosessiontime;

	HTTPSessionInfo():finishtime(0), nosessiontime(0){}
};

struct Server_IMPL:public HTTPCommunicationHandler,public enable_shared_from_this<Server_IMPL>
{
	shared_ptr<IOWorker>				worker;
	shared_ptr<Socket>					tcpServer;
	uint32_t							httpport;
	std::string							useragent;
	

	Mutex														  mutex;
	std::map<std::string, std::map<std::string, ListenInfo> >     httplistencallbackmap;
	std::map<std::string, Server::WebsocketCallback>		  websocketlistencallbackmap;
	Server::WebsocketCallback								  websocketdefaultcallback;
	std::map<std::string, ListenInfo>							  httpdefaultlistencallback;
private:
	std::map<Communication*, shared_ptr<HTTPSessionInfo> >   sessionlist;

	std::list< shared_ptr< HTTPSessionInfo> >					  freesessionlist;

	shared_ptr<Timer>											pooltimer;
public:
	Server_IMPL() :httpport(0)
	{
		pooltimer = make_shared<Timer>("Server_IMPL");
		pooltimer->start(Timer::Proc(&Server_IMPL::onPoolTimerProc, this), 0, 1000);
	}
	~Server_IMPL()
	{
		pooltimer = NULL;
		if (tcpServer) tcpServer->disconnect();
		tcpServer = NULL;
		worker = NULL;
		
		sessionlist.clear();
		freesessionlist.clear();
	}
	void addAcceptSocket(const shared_ptr<Socket>& sock, const char* appendBufAddr, uint32_t appendBufLen)
	{
		shared_ptr<HTTPSessionInfo> info = make_shared<HTTPSessionInfo>();
		info->commu = make_shared<Communication>(true, sock, shared_from_this(), useragent);

		{
			Guard locker(mutex);
			sessionlist[info->commu.get()] = info;
		}
		info->commu->start(appendBufAddr,appendBufLen);
	}
	void onAcceptCallback(const weak_ptr<Socket>& oldsock, const shared_ptr<Socket>& newsock)
	{
		shared_ptr<HTTPSessionInfo> info = make_shared<HTTPSessionInfo>();
		info->commu = make_shared<Communication>(true,newsock,shared_from_this(),useragent);

		{
			Guard locker(mutex);
			sessionlist[info->commu.get()]  = info;
		}
		info->commu->start();
		tcpServer->async_accept(Socket::AcceptedCallback(&Server_IMPL::onAcceptCallback, this));
	}
private:
	bool onRecvHeaderOk(Communication* commu)
	{
		shared_ptr<HTTPSessionInfo> info;
		{
			Guard locker(mutex);
			std::map<Communication*, shared_ptr<HTTPSessionInfo> >::iterator iter = sessionlist.find(commu);
			if (iter == sessionlist.end()) return false;

			info = iter->second;
		}

		Value connectionval = info->commu->recvHeader->header(CONNECTION);
		if (!connectionval.empty() && String::strcasecmp(connectionval.readString().c_str(), CONNECTION_Upgrade) == 0)
		{
			Server::WebsocketCallback callback = findWebsocketCallback(info->commu);

			if (!callback)
			{
				info->commu->buildErrorResponse(405, "Method Not Allowed");

				return false;
			}

			if (!WebSocketSession::checkWebsocketHeader(info->commu->recvHeader))
			{
				info->commu->buildErrorResponse(405, "Bad Request");

				return false;
			}

			shared_ptr<WebSocketSession> websocketsession = make_shared<WebSocketSession>(info->commu);
			info->websocketsession = websocketsession;
			
			callback(websocketsession);

			return false;
		}
		else
		{
			info->listeninfo = findHttpCallback(info->commu);
			if(!info->listeninfo.callback)
			{
				info->commu->buildErrorResponse(405, "Method Not Allowed");

				return false;
			}
			info->httpsessionback = info->httpsession = make_shared<ServerSession>(info->commu, info->listeninfo.type);

			return true;
		}
	}
	virtual void onRecvContentOk(Communication* commu)
	{
		shared_ptr<HTTPSessionInfo> info;
		{
			Guard locker(mutex);
			std::map<Communication*, shared_ptr<HTTPSessionInfo> >::iterator iter = sessionlist.find(commu);
			if (iter == sessionlist.end()) return;

			info = iter->second;
		}

		if (!info->httpsession) return;

		shared_ptr<ServerSession> httpsession = info->httpsession;
		info->httpsession = NULL;

		info->listeninfo.callback(httpsession);
	}
	virtual void onDisconnect(Communication* commu)
	{
		shared_ptr<HTTPSessionInfo> info;
		{
			Guard locker(mutex);
			std::map<Communication*, shared_ptr<HTTPSessionInfo> >::iterator iter = sessionlist.find(commu);
			if (iter == sessionlist.end()) return;

			info = iter->second;
			freesessionlist.push_back(info);
			sessionlist.erase(iter);
		}

		shared_ptr<WebSocketSession> websocketsession = info->websocketsession.lock();
		if (websocketsession) websocketsession->disconnected();

		shared_ptr<ServerSession> httpsession = info->httpsessionback.lock();
		if (httpsession) httpsession->disconnected();
	}
	virtual void onSessionFinish(Communication* commu)
	{
		Guard locker(mutex);
		std::map<Communication*, shared_ptr<HTTPSessionInfo> >::iterator iter = sessionlist.find(commu);
		if (iter == sessionlist.end()) return;

		//session finish ,check keepalive http
		shared_ptr<Header> recvheader = iter->second->commu->recvHeader;
		shared_ptr<Header> sendheader = iter->second->commu->sendHeader;

		if (iter->second->httpsession && recvheader && sendheader && recvheader->header(CONNECTION).readString() == CONNECTION_KeepAlive && sendheader->header(CONNECTION).readString() == CONNECTION_KeepAlive)
		{
			//reset http
			iter->second->websocketsession = weak_ptr<WebSocketSession>();
			iter->second->httpsession = shared_ptr<ServerSession>();
			iter->second->httpsessionback = weak_ptr<ServerSession>();
			iter->second->listeninfo = ListenInfo();
			iter->second->nosessiontime = 0;
			iter->second->commu->restart();
		}
		else
		{
			iter->second->finishtime = Time::getCurrentMilliSecond();
		}
	}
	Server::WebsocketCallback findWebsocketCallback(const shared_ptr<Communication>& commu)
	{
		URL url(commu->recvHeader->url);

		Guard locker(mutex);
		std::string requestPathname = url.pathname;
		
		for (std::map<std::string, Server::WebsocketCallback>::iterator iter = websocketlistencallbackmap.begin(); iter != websocketlistencallbackmap.end(); iter++)
		{
			RegEx oRegex(iter->first,RegExType_InCase);
			if (iter->first == "*" || RegEx::regex_match(requestPathname, oRegex))
			{
				return iter->second;
			}
		}

		return websocketdefaultcallback;
	}

	ListenInfo findHttpCallback(const shared_ptr<Communication>& commu)
	{
		URL url(commu->recvHeader->url);

		ListenInfo liteninfo;
		{
			std::string requestPathname = url.pathname;
			std::string method =commu->recvHeader->method;

			Guard locker(mutex);

			for (std::map<std::string, std::map<std::string, ListenInfo> >::iterator citer = httplistencallbackmap.begin(); citer != httplistencallbackmap.end() && !liteninfo.callback; citer++)
			{
				RegEx oRegex(citer->first, RegExType_InCase);
				if(citer->first!= "*" && !RegEx::regex_match(requestPathname,oRegex))
				{
					continue;
				}
				for (std::map<std::string, ListenInfo>::iterator miter = citer->second.begin() ; miter != citer->second.end() && !liteninfo.callback; miter++)
				{
					if (String::strcasecmp(method.c_str(), miter->first.c_str()) == 0)
					{
						liteninfo = miter->second;
						break;
					}
				}
			}
			for (std::map<std::string, ListenInfo>::iterator miter = httpdefaultlistencallback.begin() ; miter != httpdefaultlistencallback.end() && !liteninfo.callback; miter++)
			{
				if (String::strcasecmp(method.c_str(), miter->first.c_str()) == 0)
				{
					liteninfo = miter->second;
					break;
				}
			}
		}
		
		return liteninfo;
	}

	void onPoolTimerProc(unsigned long)
	{
		std::map< Communication*, shared_ptr<HTTPSessionInfo> > sessionlisttmp;
		std::list< shared_ptr< HTTPSessionInfo> >	freelisttmp;
		{
			Guard locker(mutex);
			sessionlisttmp = sessionlist;
			freelisttmp = freesessionlist;
			freesessionlist.clear();
		}
		uint64_t nowtime = Time::getCurrentMilliSecond();
		for (std::map< Communication*, shared_ptr<HTTPSessionInfo> >::iterator iter = sessionlisttmp.begin(); iter != sessionlisttmp.end(); iter++)
		{
			iter->second->commu->onPoolTimerProc();
			if (iter->second->commu->isTimeout() || iter->second->commu->socket->getStatus() != NetStatus_connected)
			{
				{
					Guard locker(mutex);
					sessionlist.erase(iter->first);
				}
				freelisttmp.push_back(iter->second);

				continue;
			}
			if (iter->second->finishtime != 0 && nowtime - iter->second->finishtime >= COMMUFINISHTIMEOUT)
			{
				{
					Guard locker(mutex);
					sessionlist.erase(iter->first);
				}
				freelisttmp.push_back(iter->second);

				continue;
			}

			shared_ptr<WebSocketSession> websocketsession = iter->second->websocketsession.lock();
			shared_ptr<ServerSession> httpsession = iter->second->httpsessionback.lock();
			if (iter->second->nosessiontime == 0 && websocketsession == NULL && httpsession == NULL)
			{
				iter->second->nosessiontime = nowtime;
			}
			else if (iter->second->nosessiontime != 0 && nowtime - iter->second->nosessiontime >= NOSESSIONTIMEOUT)
			{
				{
					Guard locker(mutex);
					sessionlist.erase(iter->first);
				}
				freelisttmp.push_back(iter->second);

				continue;
			}
		}
	}
};

struct Server::ServerInternal
{
	shared_ptr<Server_IMPL> manager;
};

Server::Server(const shared_ptr<IOWorker>& worker, const std::string& useragent)
{
	internal = new ServerInternal();
	internal->manager = make_shared<Server_IMPL>();
	internal->manager->worker = worker;
	internal->manager->useragent = useragent;

	if (internal->manager->worker == NULL) internal->manager->worker = IOWorker::defaultWorker();
}
Server::~Server()
{
	internal->manager = NULL;
	SAFE_DELETE(internal);
}

bool Server::listen(const std::string& path, const std::string& method, const HTTPCallback& callback, CacheType type)
{
	std::string flag1 =path;
	std::string flag2 = method;

	Guard locker(internal->manager->mutex);

	std::map<std::string, std::map<std::string, ListenInfo> >::iterator citer = internal->manager->httplistencallbackmap.find(String::tolower(flag1));
	if (citer == internal->manager->httplistencallbackmap.end())
	{
		internal->manager->httplistencallbackmap[flag1] = std::map<std::string, ListenInfo>();
		citer = internal->manager->httplistencallbackmap.find(String::tolower(flag1));
	}
	ListenInfo info;
	info.callback = callback;
	info.type = type;

	citer->second[flag2] = info;

	return true;
}
bool Server::listen(const std::string& path, const Server::WebsocketCallback& callback)
{
	std::string flag1 = path;
	
	Guard locker(internal->manager->mutex);

	internal->manager->websocketlistencallbackmap[flag1] = callback;

	return true;
}
bool Server::defaultListen(const WebsocketCallback& callback)
{
	internal->manager->websocketdefaultcallback = callback;

	return true;
}
bool Server::defaultListen(const std::string& method, const HTTPCallback& callback, CacheType type)
{
	std::string flag2 = method;
	
	Guard locker(internal->manager->mutex);

	ListenInfo info;
	info.callback = callback;
	info.type = type;

	internal->manager->httpdefaultlistencallback[flag2] = info;

	return true;
}

void Server::addAcceptSocket(const shared_ptr<Socket>& sock, const char* appendBufAddr, uint32_t appendBufLen)
{
	internal->manager->addAcceptSocket(sock, appendBufAddr, appendBufLen);
}
bool Server::run(uint32_t httpport)
{
	Guard locker(internal->manager->mutex);
	internal->manager->httpport = httpport;
	internal->manager->tcpServer = TCPServer::create(internal->manager->worker,httpport);
    if (internal->manager->tcpServer == shared_ptr<Socket>())
    {
        return false;
    }
	internal->manager->tcpServer->async_accept(Socket::AcceptedCallback(&Server_IMPL::onAcceptCallback, internal->manager));

	return true;
}

uint32_t Server::listenPort() const
{
	return internal->manager->httpport;
}


}
}
}
