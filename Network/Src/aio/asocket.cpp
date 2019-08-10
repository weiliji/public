#pragma  once
#include "win32/_winsocket.h"
#include "linux/_linuxsocket.h"
#include "asocket.h"
shared_ptr<ASocket> ASocket::create(const shared_ptr<IOWorker>& _ioworker, const shared_ptr<IOServer>& _ioserver,  NetType _type)
{
	shared_ptr<ASocket> sock(new _SystemSocket(_ioworker, _ioserver, _type));

	if (!sock->creatSocket(_type))
	{
		assert(0);
	}
	
	sock->socketptr = sock;
	sock->resourece = _ioserver->addResource(sock->sock, sock, sock->userthread);
	

	return sock;
}

shared_ptr<ASocket> ASocket::create(const shared_ptr<IOWorker>& _ioworker, const shared_ptr<IOServer>& _ioserver,  const TCPClient::NewSocketInfo& newsock)
{
	shared_ptr<ASocket> sock(new _SystemSocket(_ioworker, _ioserver, newsock));

	sock->socketptr = sock;
	sock->resourece = _ioserver->addResource(sock->sock, sock, sock->userthread);

	return sock;
}
