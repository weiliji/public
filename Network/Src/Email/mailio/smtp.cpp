/*

smtp.cpp
---------------

Copyright (C) 2016, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/


#include <vector>
#include <string>
#include <stdexcept>
#include <tuple>
#include <algorithm>
#include "base64.hpp"
#include "smtp.hpp"


using std::ostream;
using std::istream;
using std::vector;
using std::string;
using std::to_string;
using std::tuple;
using std::stoi;
using std::move;
using std::runtime_error;
using std::out_of_range;
using std::invalid_argument;
using std::chrono::milliseconds;

namespace mailio
{

struct LineObject
{
	LineObject(const std::string& str)
	{
		lines = String::split(str, "\n");
	}
	LineObject(const LineObject& obj)
	{
		lines = obj.lines;
		index = obj.index;
	}

	tuple<int, bool, string> getLine()
	{
		try
		{
			if (index >= lines.size())
			{
				throw email_error("Parsing server failure.");
			}
			uint32_t getindex = index++;
			std::string line = lines[getindex];
			String::trim_if(line, [](char a)->bool {return a == '\r' || a == '\n'; });


			return make_tuple(stoi(line.substr(0, 3)), (line.at(3) == '-' ? false : true), line.substr(4));
		}
		catch (out_of_range&)
		{
			throw email_error("Parsing server failure.");
		}
		catch (invalid_argument&)
		{
			throw email_error("Parsing server failure.");
		}
	}

	uint32_t				 index = 0;
	std::vector<std::string> lines;
};


smtp::smtp(const shared_ptr<IOWorker>& worker, const std::string& hostname, unsigned port, uint32_t timeout) : _dlg(new dialog(worker,hostname, port, timeout))
{
    _src_host = read_hostname();
}


smtp::~smtp()
{
    try
    {
        _dlg->send("QUIT");
    }
    catch (...)
    {
    }
}


void smtp::authenticate(const string& username, const string& password, auth_method_t method)
{
	if (!_dlg->connect()) throw email_error("Connect Server Error");

    connect();
    if (method == auth_method_t::LOGIN)
    {
        ehlo();
        auth_login(username, password);
    }
}


void smtp::submit(const message& msg)
{
    if (!msg.sender().address.empty())
        _dlg->send("MAIL FROM: <" + msg.sender().address + ">");
    else
        _dlg->send("MAIL FROM: <" + msg.from().addresses.at(0).address + ">");

	LineObject lineobj(_dlg->receive());

    tuple<int, bool, string> tokens = lineobj.getLine();
    if (std::get<1>(tokens) && !positive_completion(std::get<0>(tokens)))
        throw email_error("Mail sender rejection.");

    for (const auto& rcpt : msg.recipients().addresses)
    {
        _dlg->send("RCPT TO: <" + rcpt.address + ">");

		lineobj = LineObject(_dlg->receive());
		tokens = lineobj.getLine();

        if (!positive_completion(std::get<0>(tokens)))
            throw email_error("Mail recipient rejection.");
    }

    for (const auto& rcpt : msg.recipients().groups)
    {
        _dlg->send("RCPT TO: <" + rcpt.name + ">");
		lineobj = LineObject(_dlg->receive());
		tokens = lineobj.getLine();
        if (!positive_completion(std::get<0>(tokens)))
            throw email_error("Mail group recipient rejection.");
    }

    for (const auto& rcpt : msg.cc_recipients().addresses)
    {
        _dlg->send("RCPT TO: <" + rcpt.address + ">");
		lineobj = LineObject(_dlg->receive());
		tokens = lineobj.getLine();
        if (!positive_completion(std::get<0>(tokens)))
            throw email_error("Mail cc recipient rejection.");
    }

    for (const auto& rcpt : msg.cc_recipients().groups)
    {
        _dlg->send("RCPT TO: <" + rcpt.name + ">");
		lineobj = LineObject(_dlg->receive());
		tokens = lineobj.getLine();
        if (!positive_completion(std::get<0>(tokens)))
            throw email_error("Mail group cc recipient rejection.");
    }

    for (const auto& rcpt : msg.bcc_recipients().addresses)
    {
        _dlg->send("RCPT TO: <" + rcpt.address + ">");
		lineobj = LineObject(_dlg->receive());
		tokens = lineobj.getLine();
        if (!positive_completion(std::get<0>(tokens)))
            throw email_error("Mail bcc recipient rejection.");
    }

    for (const auto& rcpt : msg.bcc_recipients().groups)
    {
        _dlg->send("RCPT TO: <" + rcpt.name + ">");
		lineobj = LineObject(_dlg->receive());
		tokens = lineobj.getLine();
        if (!positive_completion(std::get<0>(tokens)))
            throw email_error("Mail group bcc recipient rejection.");
    }

    _dlg->send("DATA");
	lineobj = LineObject(_dlg->receive());
	tokens = lineobj.getLine();
    if (!positive_intermediate(std::get<0>(tokens)))
        throw email_error("Mail message rejection.");

    string msg_str;
    msg.format(msg_str, true);
    _dlg->send(msg_str + "\r\n.");
	lineobj = LineObject(_dlg->receive());
	tokens = lineobj.getLine();
    if (!positive_completion(std::get<0>(tokens)))
        throw email_error("Mail message rejection.");
}


void smtp::source_hostname(const string& src_host)
{
    _src_host = src_host;
}


string smtp::source_hostname() const
{
    return _src_host;
}


void smtp::connect()
{
	LineObject lineobj = LineObject(_dlg->receive());
	tuple<int, bool, string> tokens = lineobj.getLine();
    if (std::get<0>(tokens) != 220)
        throw email_error("Connection rejection.");
}


void smtp::auth_login(const string& username, const string& password)
{
    _dlg->send("AUTH LOGIN");
	LineObject lineobj = LineObject(_dlg->receive());
	tuple<int, bool, string> tokens = lineobj.getLine();
    if (std::get<1>(tokens) && !positive_intermediate(std::get<0>(tokens)))
        throw email_error("Authentication rejection.");

    // TODO: use static encode from base64
    base64 b64;
    _dlg->send(b64.encode(username)[0]);
	lineobj = LineObject(_dlg->receive());
	tokens = lineobj.getLine();
    if (std::get<1>(tokens) && !positive_intermediate(std::get<0>(tokens)))
        throw email_error("Username rejection.");

    _dlg->send(b64.encode(password)[0]);
	lineobj = LineObject(_dlg->receive());
	tokens = lineobj.getLine();
    if (std::get<1>(tokens) && !positive_completion(std::get<0>(tokens)))
        throw email_error("Password rejection.");
}


void smtp::ehlo()
{
    _dlg->send("EHLO " + _src_host);
	LineObject lineobj = LineObject(_dlg->receive());
	tuple<int, bool, string> tokens = lineobj.getLine();  
    while (!std::get<1>(tokens))
    {
		tokens = lineobj.getLine();
    }

    if (!positive_completion(std::get<0>(tokens)))
    {
        _dlg->send("HELO " + _src_host);
        
		lineobj = LineObject(_dlg->receive());
		tokens = lineobj.getLine();
        while (!std::get<1>(tokens))
        {
			tokens = lineobj.getLine();
        }
        if (!positive_completion(std::get<0>(tokens)))
            throw email_error("Initial message rejection.");
    }
}


string smtp::read_hostname()
{
    try
    {
		char buffer[256] = { 0 };
#ifdef WIN32
		gethostname(buffer, sizeof(buffer));
#else
		intgethostname(buffer, sizeof(buffer));
#endif

        return buffer;
    }
    catch (system_error&)
    {
        throw email_error("Reading hostname failure.");
    }
}

inline bool smtp::positive_completion(int status)
{
    return status / 100 == smtp_status_t::POSITIVE_COMPLETION;
}


inline bool smtp::positive_intermediate(int status)
{
    return status / 100 == smtp_status_t::POSITIVE_INTERMEDIATE;
}


inline bool smtp::transient_negative(int status)
{
    return status / 100 == smtp_status_t::TRANSIENT_NEGATIVE;
}


inline bool smtp::permanent_negative(int status)
{
    return status / 100 == smtp_status_t::PERMANENT_NEGATIVE;
}

#ifdef SUPPORT_OPENSSL

smtps::smtps(const shared_ptr<IOWorker>& worker, const std::string& hostname, unsigned port, uint32_t timeout) : smtp(worker,hostname, port, timeout)
{
}


void smtps::authenticate(const string& username, const string& password, auth_method_t method)
{
	if (!_dlg->connect()) throw email_error("Connect Server Error");

    if (method == auth_method_t::NONE)
    {
        switch_to_ssl();
        connect();
        ehlo();
    }
    else if (method == auth_method_t::LOGIN)
    {
        switch_to_ssl();
        connect();
        ehlo();
        auth_login(username, password);
    }
    else if (method == auth_method_t::START_TLS)
    {
        connect();
        ehlo();
        start_tls();
        auth_login(username, password);
    }
}


void smtps::start_tls()
{
    _dlg->send("STARTTLS");
	LineObject lineobj = LineObject(_dlg->receive());
	tuple<int, bool, string> tokens = lineobj.getLine();
    if (std::get<1>(tokens) && std::get<0>(tokens) != 220)
        throw email_error("Start tls refused by server.");

    switch_to_ssl();
    ehlo();
}


void smtps::switch_to_ssl()
{
	_dlg = make_shared<dialog_ssl>(_dlg);

	if (!_dlg->connect()) throw email_error("Stich SSL To Server Error");
}

#endif

} // namespace mailio
