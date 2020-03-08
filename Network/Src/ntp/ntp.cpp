#include "Network/Socket/Udp.h"
#include "Network/ntp/ntp.h"
#include "ntpProtocol.h"

namespace Public {
namespace Network {
namespace NTP {

struct Server::ServerInternal
{
	shared_ptr<IOWorker>		worker;
	GetTimeCallback				getcallback;

	shared_ptr<Socket>			ntpsocket;

	void onSocketRecvCallback(const weak_ptr<Socket>&, const char* recvbuf, int len,const NetAddr& addr)
	{
		shared_ptr<Socket> sock = ntpsocket;

		if (sock)
		{
			char buffer[NTP_HLEN] = { 0 };
			Time nowtime;
			if (getcallback) nowtime = getcallback();
			else nowtime = Time::getCurrentTime();




			if (NTPProtocol::parseAndBuildResponse(recvbuf, len, buffer, nowtime))
			{
				sock->sendto(buffer, NTP_HLEN, addr);
			}

			sock->async_recvfrom(Socket::RecvFromCallback(&ServerInternal::onSocketRecvCallback, this));
		}
	}
};

Server::Server()
{
	internal = new ServerInternal;
}
Server::~Server()
{
	stop();

	SAFE_DELETE(internal);
}

ErrorInfo Server::start(const shared_ptr<IOWorker>& worker, uint32_t port, const GetTimeCallback& getcallback)
{
	internal->worker = worker;
	internal->getcallback = getcallback;

	internal->ntpsocket = UDP::create(worker);
	if (!internal->ntpsocket->bind(port)) return ErrorInfo(Error_Code_Fail, "Bind Error");

	internal->ntpsocket->async_recvfrom(Socket::RecvFromCallback(&ServerInternal::onSocketRecvCallback, internal));

	return ErrorInfo();
}
ErrorInfo Server::stop()
{
	if (internal->ntpsocket) internal->ntpsocket->disconnect();

	internal->ntpsocket = NULL;

	return ErrorInfo();
}


struct Client::ClientInternal
{
	SetTimeCallback				setcallback;
	FinishCallback				finishcallback;

	shared_ptr<IOWorker>		worker;
	shared_ptr<Socket>			ntpsocket;

	ErrorInfo					err = ErrorInfo(Error_Code_Fail);

	Base::Semaphore				waitsem;

	NetAddr						serveraddr;
	uint32_t					timeout;

	void onSocketRecvCallback(const weak_ptr<Socket>&, const char* buffer, int len, const NetAddr& addr)
	{
		Time nowtime = Time::getCurrentTime();
		Time recvtime;

		if (!NTPProtocol::parseResponse(buffer, len, nowtime, recvtime))
		{
			err = ErrorInfo(Error_Code_ParseObject);

			return doResult(Time());
		}
		else
		{
			err = ErrorInfo();

			return doResult(recvtime);
		}
	}

	void onSocketConnectCallback(const weak_ptr<Socket>&, bool ret, const std::string&)
	{
		if (!ret)
		{
			err = ErrorInfo(Error_Code_ConnectTimeout);
			return doResult(Time());
		}
		else
		{
			shared_ptr<Socket> sock = ntpsocket;
			if (sock)
			{
				char buffer[NTP_HLEN] = { 0 };
				Time nowtime;

				NTPProtocol::buildRequest(buffer, nowtime);
				sock->sendto(buffer, NTP_HLEN, serveraddr);

				sock->async_recvfrom(Socket::RecvFromCallback(&ClientInternal::onSocketRecvCallback,this), timeout);
			}
		}
	}
	void doResult(const Time& t)
	{
		if (!err)
		{
			if(setcallback) setcallback(t);
			else Time::setCurrentTime(t);
		}
		finishcallback(err);

		waitsem.post();
	}
};


Client::Client()
{
	internal = new ClientInternal();
}
Client::~Client()
{
	SAFE_DELETE(internal);
}

void Client::asyncUpdate(const shared_ptr<IOWorker>& worker, const std::string& serverip, uint32_t port, const FinishCallback& finishcallback, uint32_t timeout, const SetTimeCallback& setcallback)
{
	internal->worker = worker;
	internal->serveraddr = NetAddr(serverip, port);
	internal->finishcallback = finishcallback;
	internal->setcallback = setcallback;
	internal->timeout = timeout;

	internal->ntpsocket = UDP::create(worker);
	internal->ntpsocket->async_connect(internal->serveraddr, Socket::ConnectedCallback(&ClientInternal::onSocketConnectCallback, internal),timeout);
}
ErrorInfo Client::update(const shared_ptr<IOWorker>& worker, const std::string& serverip, uint32_t port, uint32_t timeout, const SetTimeCallback& setcallback)
{
	asyncUpdate(worker, serverip, port, FinishCallback(), timeout, setcallback);

	internal->waitsem.pend(timeout);

	return internal->err;
}


}
}
}