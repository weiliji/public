/*

dialog.cpp
----------

Copyright (C) 2016, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/

#include <string>
#include <algorithm>
#include "dialog.hpp"


using std::string;
using std::to_string;
using std::move;
using std::istream;


namespace mailio
{

dialog::dialog(const shared_ptr<IOWorker>& worker, const std::string& hostname, unsigned port, uint32_t timeout)
:_worker(worker),_hostname(hostname),_port(port),_timeout(timeout){}

dialog::dialog(const shared_ptr<dialog>& dlg)
	:_worker(dlg->_worker),_hostname(dlg->_hostname),_port(dlg->_port),_timeout(dlg->_timeout){}

dialog::~dialog()
{
	if (_socket) _socket->disconnect();
	_socket = NULL;
}

bool dialog::connect()
{
	_socket = TCPClient::create(_worker);
	if (_socket == NULL) return false;


	return _socket->connect(NetAddr(_hostname, _port), _timeout);
}
void dialog::send(const string& line)
{
	if (_socket == NULL) return;

	std::string linestr = line + "\r\n";

	_socket->send(linestr.c_str(), (uint32_t)linestr.length(),_timeout);
}


// TODO: perhaps the implementation should be common with `receive_raw()`
string dialog::receive()
{
	if (_socket == NULL) return "";

	uint32_t maxrecvlen = 1024 * 1024;
	string recvbuffer;
	recvbuffer.resize(maxrecvlen);

	int recvlen = _socket->recv((char*)recvbuffer.c_str(), maxrecvlen,_timeout);
	if (recvlen <= 0) return "";

	recvbuffer.resize(recvlen);

	return recvbuffer;
}

#ifdef SUPPORT_OPENSSL

dialog_ssl::dialog_ssl(const shared_ptr<IOWorker>& worker, const std::string& hostname, unsigned port, uint32_t timeout)
	:dialog(worker,hostname,port,timeout){}

dialog_ssl::dialog_ssl(const shared_ptr<dialog>& dlg): dialog(dlg){}

bool dialog_ssl::connect()
{
	bool ret = dialog::connect();
	if (!ret) return false;

	_socket = SSLSocket::create(_socket,_timeout);
	if (_socket == NULL) return false;

	return true;
}
dialog_ssl::~dialog_ssl() 
{
	if (_socket) _socket->disconnect();
}

#endif

} // namespace mailio
