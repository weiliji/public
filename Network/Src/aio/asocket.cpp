#pragma  once
#include "win32/_winsocket.h"
#include "asocket.h"
shared_ptr<ASocket> ASocket::create(const shared_ptr<IOWorker>& _ioworker, const shared_ptr<IOServer>& _ioserver, const shared_ptr<Socket>& _sockptr, NetType _type)
{
	shared_ptr<ASocket> sock(new _WinSocket(_ioworker, _ioserver, _sockptr, _type));

	if (!sock->creatSocket(_type))
	{
		assert(0);
	}
	
	sock->resourece = _ioserver->addResource(sock->sock, _sockptr, sock->userthread);
	

	return sock;
}

shared_ptr<ASocket> ASocket::create(const shared_ptr<IOWorker>& _ioworker, const shared_ptr<IOServer>& _ioserver, const shared_ptr<Socket>& _sockptr, const NewSocketInfo& newsock)
{
	shared_ptr<ASocket> sock(new _WinSocket(_ioworker, _ioserver, _sockptr, newsock));

	sock->resourece = _ioserver->addResource(sock->sock, _sockptr, sock->userthread);

	return sock;
}
