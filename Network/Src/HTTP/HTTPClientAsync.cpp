#include "Network/HTTP/Client.h"
#include "HTTPCommunication.h"

namespace Public {
namespace Network{
namespace HTTP {

struct AsyncClient::AsyncClientInternal
{
	Mutex					mutex;
	std::map<Client*, weak_ptr<Client> > clientlist;
	shared_ptr<Timer>		poolTimer;


	AsyncClientInternal()
	{
		poolTimer = make_shared<Timer>("HTTPClientAsyncInternal");
		poolTimer->start(Timer::Proc(&AsyncClientInternal::onPoolTimerProc, this), 0, 1000);
	}
	~AsyncClientInternal()
	{
		poolTimer = NULL;
	}

	void onPoolTimerProc(unsigned long)
	{
		std::map<Client*, weak_ptr<Client> > clientlisttmp;

		{
			Guard locker(mutex);
			clientlisttmp = clientlist;
		}

		for (std::map<Client*, weak_ptr<Client> >::iterator iter = clientlisttmp.begin(); iter != clientlisttmp.end(); iter++)
		{
			shared_ptr<Client> client = iter->second.lock();

			if (client)
			{
				bool ret = client->onPoolTimerProc();
				if (ret)
				{
					Guard locker(mutex);
					clientlist.erase(iter->first);
				}
			}
			else
			{
				Guard locker(mutex);
				clientlist.erase(iter->first);
			}
		}
	}
};

AsyncClient::AsyncClient()
{
	internal = new AsyncClientInternal;
}
AsyncClient::~AsyncClient()
{
	{
		Guard locker(internal->mutex);
		internal->clientlist.clear();
	}

	SAFE_DELETE(internal);
}
void AsyncClient::addClient(const shared_ptr<Client>& client)
{
	Guard locker(internal->mutex);

	internal->clientlist[client.get()] = client;
}

}
}
}
