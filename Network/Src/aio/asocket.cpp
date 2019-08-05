#pragma  once
#include "win32/_winsocket.h"
#include "asocket.h"
shared_ptr<ASocket> ASocket::create(const shared_ptr<IOWorker>& _ioworker, const shared_ptr<IOServer>& _ioserver, const shared_ptr<Socket>& _sockptr, NetType _type)
{
	shared_ptr<ASocket> sock(new _WinSocket(_ioworker, _ioserver, _sockptr, _type));

	if (sock->creatSocket(_type))
	{
		sock->ioserver->create(sock->sock);
	}

	if (sock->type == NetType_TcpClient)
	{
		int flag = 1;
		sock->setSocketOpt(IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
	}

	return sock;
}

shared_ptr<ASocket> ASocket::create(const shared_ptr<IOWorker>& _ioworker, const shared_ptr<IOServer>& _ioserver, const shared_ptr<Socket>& _sockptr, const NewSocketInfo& newsock)
{
	shared_ptr<ASocket> sock(new _WinSocket(_ioworker, _ioserver, _sockptr, newsock));

	sock->ioserver->create(sock->sock);

	return sock;
}
