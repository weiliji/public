#include "Network/Network.h"
using namespace Public::Network;

#if 1
class NetworkServerInfo
{
public:
	shared_ptr<Socket>	sock;

	String data;

	AtomicCount		sendcount;

	uint64_t		nowtime;
	NetworkServerInfo(const shared_ptr<Socket>& _sock)
	{
		sock = _sock;
		data.alloc(10240);
		data.resize(10240);

		const char* buffer = data.c_str();
		uint32_t bufferlen = data.length();

		shared_ptr<Socket> tmp = sock;
		if (tmp) tmp->async_send(buffer, bufferlen, Socket::SendedCallback(&NetworkServerInfo::_sendcallback, this));
	}
	~NetworkServerInfo()
	{
		if (sock) sock->disconnect();
	}

	void inputData()
	{
		if (++sendcount == 1)
		{
			const char* buffer = data.c_str();
			uint32_t bufferlen = data.length();

			shared_ptr<Socket> tmp = sock;
			if (tmp) tmp->async_send(buffer, bufferlen, Socket::SendedCallback(&NetworkServerInfo::_sendcallback, this));
		}
	}

	void _sendcallback(const weak_ptr<Socket>&, const char* buffer, int len)
	{
		printf("_sendcallback %d\r\n", len);
	//	if (--sendcount > 0)
		{

			const char* buffer = data.c_str();
			uint32_t bufferlen = data.length();

			shared_ptr<Socket> tmp = sock;
			if (tmp) tmp->async_send(buffer, bufferlen, Socket::SendedCallback(&NetworkServerInfo::_sendcallback, this));
		}
	}
};

Mutex servermutex;
std::map<Socket*, shared_ptr<NetworkServerInfo> > serverlist;
shared_ptr<Thread>	serverthreadex;
shared_ptr<Socket>	tcpserver;

void _socketRecv(const weak_ptr<Socket>& sock, const char* buffer, int len)
{
	shared_ptr<Socket> socktmp = sock.lock();
	if (socktmp) socktmp->async_recv(_socketRecv);
}
void _socketDisconnect(const weak_ptr<Socket>& sock, const std::string& err)
{
	shared_ptr<Socket> socktmp = sock.lock();
	if (socktmp == NULL) return;

	{
		Guard locker(servermutex);
		std::map<Socket*, shared_ptr<NetworkServerInfo> >::iterator iter = serverlist.find(socktmp.get());
		if (iter != serverlist.end())
		{
			serverlist.erase(iter);
		}
	}

}

void _socketAccpet(const weak_ptr<Socket>&sock, const shared_ptr<Socket>& newsock)
{
	{
		shared_ptr<NetworkServerInfo> info = make_shared<NetworkServerInfo>(newsock);
		info->sock->async_recv(_socketRecv);
		info->sock->setDisconnectCallback(_socketDisconnect);

		Guard locker(servermutex);
		serverlist[newsock.get()] = info;
	}
	
	shared_ptr<Socket> socktmp = sock.lock();
	if (socktmp == NULL) return;
	socktmp->async_accept(_socketAccpet);
}

void runServerThread(Thread* ex, void* param)
{
	String data;
	data.alloc(10240);
	data.resize(10240);

//	while (ex->looping())
	{
//		Thread::sleep(10000000000000);


		/*{
			Guard lock(servermutex);

			for (std::map<Socket*, shared_ptr<NetworkServerInfo> >::iterator iter = serverlist.begin(); iter != serverlist.end(); iter++)
			{
				shared_ptr<NetworkServerInfo> serverinfo = iter->second;
				if (serverinfo == NULL) continue;;

				serverinfo->inputData();
			}
		}*/
	}
}

void runServer(const shared_ptr<IOWorker>& worker)
{
	tcpserver = TCPServer::create(worker, 4444);
	tcpserver->async_accept(_socketAccpet);

	serverthreadex = ThreadEx::creatThreadEx("", runServerThread, NULL);
	serverthreadex->createThread();
}


std::map<Socket*, shared_ptr<Socket> > clientlist;

void _socketRecvcallback(const weak_ptr<Socket>& sock, const char* buf, int len)
{
	printf("_socketRecvcallback %d\r\n", len);

	shared_ptr<Socket> socktmp = sock.lock();
	if (socktmp == NULL) return;

	socktmp->async_recv(_socketRecvcallback,1024*1024);
}

void _socketConnectCallback(const weak_ptr<Socket>& sock,bool,const std::string&)
{
	shared_ptr<Socket> socktmp = sock.lock();
	if (socktmp == NULL) return;

	socktmp->async_recv(_socketRecvcallback, 1024 * 1024);
}

void runClient(const shared_ptr<IOWorker>& worker,uint32_t size)
{
//	while(1)
	{
		if (clientlist.size() > size)
		{
		//	break;
		//	shared_ptr<Socket> sock = clientlist.begin()->second;
			clientlist.erase(clientlist.begin());
		//	sock->disconnect();
		}
		shared_ptr<Socket> sock = TCPClient::create(worker);
		clientlist[sock.get()] = sock;

		sock->async_connect(NetAddr("192.168.2.46", 4444), _socketConnectCallback);

		Thread::sleep(500);
	}
}


int main(int args,const char* argv[])
{
	shared_ptr<IOWorker>	worker = make_shared<IOWorker>(4);
	
	if (args == 1)
	{
		runServer(worker);
	}
	else
	{
		runClient(worker, 10);
	}


	getchar();

	return 0;
}

#endif