#include "Network/HTTP/Client.h"
#include "Network/Socket/TcpClient.h"
#include "Network/Socket/SSLSocket.h"
#include "HTTPCommunication.h"

namespace Public {
namespace Network {
namespace HTTP {

struct Client_IMPL :public HTTPCommunicationHandler, public enable_shared_from_this<Client_IMPL>
{
	typedef Function<shared_ptr<Socket>(const shared_ptr<Socket>&)> ResetSSLSocketCallback;


	shared_ptr<IOWorker>	worker;
	shared_ptr<Socket>		socket;

	shared_ptr<Communication>	commu;
	shared_ptr<ClientRequest>	request;
	shared_ptr<ClientResponse>	response;

	std::string				useragent;

	bool					connecteddisconected = false;
    
	uint64_t				startconnecttime = Time::getCurrentMilliSecond();
	std::string				responsesavefile;

	Client::HTTPCallback	asynccallback;

	bool					sessionfinish = false;
	Base::Semaphore			sessionfinishsem;

	bool					haveBuildAuthen = false;

	uint64_t				starttime = Time::getCurrentMilliSecond();

	std::list<shared_ptr<Socket> > freesocketlist;

	ResetSSLSocketCallback		resetSSLSocketCallback;


	Client_IMPL(const shared_ptr<IOWorker>& _worker, const std::string& _useragent) :worker(_worker), useragent(_useragent), connecteddisconected(false), startconnecttime(Time::getCurrentMilliSecond())
	{
		if (worker == NULL) worker = NULL;
		sessionfinish = false;
	}
	~Client_IMPL()
	{
		close();
		if (socket) socket->disconnect();
		socket = NULL;
        request = NULL;
        response = NULL;
		commu = NULL;
	}
	void close()
	{
		if (commu) commu->close();
	}

	bool start(const shared_ptr<ClientRequest>& req, const std::string & savefile)
	{
		responsesavefile = savefile;
		startconnecttime = Time::getCurrentMilliSecond();

		URL url1(request ? request->header()->url : "");
		URL url2(req ? req->header()->url : "");

		//socket断开了链接，获取请求和应答都是keepalive，可以使用原来的socket
		if (connecteddisconected || (socket && socket->getStatus() != NetStatus_connected) || url1.getHostname() != url2.getHostname() ||
			!(commu && commu->sendHeader && commu->sendHeader->header(CONNECTION).readString() == CONNECTION_KeepAlive
				&& commu->recvHeader && commu->recvHeader->header(CONNECTION).readString() == CONNECTION_KeepAlive))
		{
			freesocketlist.push_back(socket);
			socket = NULL;
            response = NULL;
            if (commu)
            {
                commu->close();
            }
			commu = NULL;
			connecteddisconected = false;
		}
		uint32_t timeout = req->timeout();
		if (timeout < 1000) timeout = 1000;

		request = req;

		if (socket == NULL)
		{
			socket = TCPClient::create(worker);

			socket->async_connect(NetAddr(url2.getHostname(), url2.getPort(80)), Socket::ConnectedCallback(&Client_IMPL::socketConnectCallback, weak_ptr<Client_IMPL>(shared_from_this())), timeout);
		}
		else
		{
			socketConnectCallback(socket, true, "");
		}

		return true;
	}

	void socketConnectCallback(const weak_ptr<Socket>& sock, bool status, const std::string& errmsg)
	{
		if (!status)
		{
			connecteddisconected = true;
			return;
		}

		if (!commu)
		{
			if (resetSSLSocketCallback)
			{
				socket = resetSSLSocketCallback(socket);
			}

			//Communication(bool _isserver,const shared_ptr<Socket> & _sock,const shared_ptr<HTTPCommunicationHandler>& _handler,const std::string& _useragent)
			commu = make_shared<Communication>(false, socket, shared_from_this(), useragent);
			commu->start();
		}
		else
		{
			commu->restart();
		}

		{
			URL url(request->header()->url);

			request->header()->push("Host", url.getHost());
		}

		commu->setSendHeaderContentAndPostSend(request->header(), request->content());
	}

	virtual bool onRecvHeaderOk(Communication*)
	{
		response = make_shared<ClientResponse>(commu, responsesavefile.length() == 0 ? CacheType_Mem : CacheType_File, responsesavefile);
		commu->recvContent = response->content();
        if (commu->recvContent == NULL)
        {
        //    int a = 0;
        }


		return true;
	}
	virtual void onRecvContentOk(Communication* commu)
	{
	}
	virtual void onDisconnect(Communication* commu)
	{
		connecteddisconected = true;

        shared_ptr<IOWorker> worktmp = worker;
        if (worktmp) worktmp->postEvent(Function<void(void*)>(&Client_IMPL::doDisconnect, weak_ptr<Client_IMPL>(shared_from_this())), NULL);
	}
   
	virtual void onSessionFinish(Communication*)
	{
        shared_ptr<IOWorker> worktmp = worker;
        if (worktmp) worktmp->postEvent(Function<void(void*)>(&Client_IMPL::doCheckSessionFinish, weak_ptr<Client_IMPL>(shared_from_this())), NULL);
	}
	shared_ptr<ClientResponse> errorResponse(uint32_t errcode, const std::string& errmsg)
	{
		shared_ptr<ClientResponse> response = make_shared<ClientResponse>(commu, CacheType_Mem);

		response->header()->statuscode = errcode;
		response->header()->statusmsg = errmsg;

		return response;
	}
    virtual void onRecvData(Communication *commu, const char* buffer, uint32_t len)
    {
        request->recvcallback()(commu->socket, commu->recvHeader,buffer, len);
    }
private:
    void doCheckSessionFinish(void*)
    {
        URL url(request->header()->url);
        std::string wwwauthen = response ? response->header()->header(HTTPHEADER_WWWAUTHENTICATE) : "";

        //如果是401，并且自己还有认证信息
        if (commu && commu->recvHeader && commu->recvHeader->statuscode == 401 && (url.authen.Password.length() > 0 || url.authen.Username.length() > 0) && !haveBuildAuthen && wwwauthen.length() > 0)
        {
            request->header()->headers[HTTPHEADER_AUTHENTICATE] = WWW_Authenticate::buildAuthorization(String::toupper(request->header()->method), url.authen.Username, url.authen.Password, url.pathname, wwwauthen);
            start(request, responsesavefile);
            haveBuildAuthen = true;

            return;
        }

        bool prevfinish = sessionfinish;
        sessionfinish = true;
        sessionfinishsem.post();

        if (!prevfinish)
        {
            asynccallback(request, response);

            asynccallback = NULL;
        }
    }
    void doDisconnect(void*)
    {
        request->discallback()(request, "disconntend");
    }
};

struct Client::ClientInternal
{
	shared_ptr<Client_IMPL> manager;


	bool onCheckSessionFinish()
	{
		if (manager == NULL /*|| manager->response*/ || manager->sessionfinish)
		{
			return true;
		}

		manager->freesocketlist.clear();
		uint64_t nowtime = Time::getCurrentMilliSecond();

		uint32_t timeout = manager->request->timeout();

		if (manager->commu)
			manager->commu->onPoolTimerProc();

		if (nowtime - manager->starttime >= timeout)
		{
			manager->response = manager->errorResponse(408, "Request Timeout");
			manager->asynccallback(manager->request, manager->response);
			manager->asynccallback = NULL;
			return true;
		}
		if (manager->connecteddisconected)
		{
			manager->response = manager->errorResponse(500, "Socket Disconnect");
			manager->asynccallback(manager->request, manager->response);
			manager->asynccallback = NULL;
			return true;
		}


		return false;
	}

	const shared_ptr<ClientResponse> depend(uint32_t timeout)
	{
		while (!onCheckSessionFinish())
		{
			manager->sessionfinishsem.pend(1000);
		}

		shared_ptr<ClientResponse> response = manager->response;
		if (response == NULL)
		{
			response = manager->errorResponse(408, "Request Timeout");
		}

		return response;
	}
};
Client::Client(const shared_ptr<IOWorker>& worker, const std::string& useragent)
{
	internal = new ClientInternal;
	internal->manager = make_shared<Client_IMPL>(worker, useragent);
}
Client::~Client()
{
	disconnect();
	
	internal->manager = NULL;
	SAFE_DELETE(internal);
}
void Client::disconnect()
{
	if (internal->manager) internal->manager->close();
}
bool Client::onPoolTimerProc()
{
	return internal->onCheckSessionFinish();
}

const shared_ptr<ClientResponse> Client::request(const shared_ptr<ClientRequest>& req, const std::string& saveasfile)
{
	internal->manager->start(req, saveasfile);

	return internal->depend(req->timeout());
}
bool Client::request(const shared_ptr<AsyncClient>& asyncManager, const shared_ptr<ClientRequest>& req, const HTTPCallback& callback, const std::string& saveasfile)
{
	if (asyncManager == NULL || req == NULL) return false;

	URL url(req->header()->url);
	if (url.authen.Username.length() > 0)
	{
		//req->header()->headers[HTTPHEADER_AUTHENTICATE] = WWW_Authenticate::buildAuthorization(String::toupper(req->header()->method), url.authen.Username, url.authen.Password, url.pathname, "Basic");
	}

	internal->manager->asynccallback = callback;
	if (!internal->manager->start(req, saveasfile)) return false;

	asyncManager->addClient(shared_from_this());

	return true;
}

shared_ptr<Socket> Client::getSocket() const
{
    return internal->manager->socket;
}

}

namespace HTTPS{
#ifdef SUPPORT_OPENSSL

shared_ptr<Socket> SSLSocketConnect(const shared_ptr<Socket>& sock)
{
	return SSLSocket::create(sock);
}


Client::Client(const shared_ptr<IOWorker>& worker, const std::string& useragent):HTTP::Client(worker,useragent)
{
	internal->manager->resetSSLSocketCallback = SSLSocketConnect;
}

Client::~Client()
{
	disconnect();
}

#endif

}

}
}