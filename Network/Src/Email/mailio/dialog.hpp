/*

dialog.hpp
----------

Copyright (C) 2016, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/ 


#pragma once

#include <string>
#include <stdexcept>
#include <chrono>
#include "export.hpp"


namespace mailio
{
/**
Dealing with network in a line oriented fashion.
**/
class MAILIO_EXPORT dialog
{
public:

    /**
    Making a connection to the server.

    @param hostname     Server hostname.
    @param port         Server port.
    @param timeout      Network timeout after which I/O operations fail. If zero, then no timeout is set i.e. I/O operations are synchronous.
    @throw dialog_error Server connecting failed.
    @throw *             `connect_async()`.
    **/
    dialog(const shared_ptr<IOWorker>& worker, const std::string& hostname, unsigned port, uint32_t timeout = 5000);
	dialog(const shared_ptr<dialog>& dlg);
    /**
    Closing the connection.
    **/
    virtual ~dialog();

	virtual bool connect();

    /**
    Sending a line to network synchronously or asynchronously, depending of the timeout value.

    @param line Line to send.
    @throw *    `send_sync<Socket>(Socket&, const std::string&)`, `send_async<Socket>(Socket&, const std::string&)`.
    **/
    virtual void send(const std::string& line);

    /**
    Receiving a line from network.

    @param raw Flag if the receiving is raw (no CRLF is truncated) or not.
    @return    Line read from network.
    @throw *   `receive_sync<Socket>(Socket&, bool)`, `receive_async<Socket>(Socket&, bool)`.
    **/
    virtual std::string receive();
protected:
    /**
    Server hostname.
    **/
    std::string _hostname;

    /**
    Server port.
    **/
    unsigned int _port = 0;
	uint32_t _timeout = 5000;

	shared_ptr<IOWorker>		_worker;
	shared_ptr<Socket>			_socket;
};

#ifdef SUPPORT_OPENSSL

/**
Secure version of `dialog` class.
**/
class dialog_ssl : public dialog
{
public:

    /**
    Making a connection to the server.

    @param hostname Server hostname.
    @param port     Server port.
    @param timeout  Network timeout after which I/O operations fail. If zero, then no timeout is set i.e. I/O operations are synchronous.
    @throw *        `dialog::dialog(const std::string&, unsigned)`.
    **/
    dialog_ssl(const shared_ptr<IOWorker>& worker, const std::string& hostname, unsigned port, uint32_t timeout = 5000);
	dialog_ssl(const shared_ptr<dialog>& dlg);

	virtual bool connect();

	virtual ~dialog_ssl();
};

#endif

} // namespace mailio
