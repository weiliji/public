/*

pop3.hpp
--------

Copyright (C) 2016, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/ 


#pragma once

#include <string>
#include <vector>
#include <map>
#include <utility>
#include <istream>
#include <chrono>
#include "dialog.hpp"
#include "mailmessage.hpp"
#include "export.hpp"
#include "email.hpp"

namespace mailio
{


/**
POP3 client implementation.
**/
class MAILIO_EXPORT pop3:public email
{
public:
    /**
    Making connection to a server.

    @param hostname Hostname of the server.
    @param port     Port of the server.
    @param timeout  Network timeout after which I/O operations fail. If zero, then no timeout is set i.e. I/O operations are synchronous.
    @throw *        `dialog::dialog(const string&, unsigned)`.
    **/
    pop3(const shared_ptr<IOWorker>& worker, const std::string& hostname, unsigned port, uint32_t timeout = 10000);

    /**
    Sending the quit command and closing the connection.
    **/    
    virtual ~pop3();

    pop3(const pop3&) = delete;

    pop3(pop3&&) = delete;

    void operator=(const pop3&) = delete;

    void operator=(pop3&&) = delete;

    /**
    Authentication with the given credentials.

    The method should be called only once on an existing object - it is not possible to authenticate again within the same connection.

    @param username Username to authenticate.
    @param password Password to authenticate.
    @param method   Authentication method to use.
    @throw *        `connect()`, `auth_login(const string&, const string&)`.
    **/
    void authenticate(const std::string& username, const std::string& password, auth_method_t method);

    /**
    Listing the size in octets of a message or all messages in a mailbox.
    
    @param message_no Number of the message to list. If zero, then all messages are listed.
    @return           Message list.
    @throw email_error Listing message failure.
    @throw email_error Listing all messages failure.
    @throw email_error Parser failure.
    @throw *          `parse_status(const string&)`, `dialog::send(const string&)`, `dialog::receive()`.
    @todo             This method is perhaps useless and should be removed.
    **/
    message_list_t list(unsigned message_no = 0);

    /**
    Fetching the mailbox statistics.
    
    @return           Number of messages and mailbox size in octets.
    @throw email_error Reading statistics failure.
    @throw email_error Parser failure.
    @throw *          `parse_status(const string&)`, `dialog::send(const string&)`, `dialog::receive()`.
    **/
    mailbox_stat_t statistics();

    /**
    Fetching a message.

    The flag for fetching the header only uses a different POP3 command (than for retrieving the full messsage) which is not mandatory by POP3. In case the
    command fails, the method will not report an error but rather the `msg` parameter will be empty.
    
    @param message_no  Message number to fetch.
    @param msg         Fetched message.
    @param header_only Flag if only the message header should be fetched.
    @throw email_error  Fetching message failure.
    @throw *          `parse_status(const string&)`, `dialog::send(const string&)`, `dialog::receive()`.
    **/
    void fetch(unsigned long message_no, message& msg, bool header_only = false);

    /**
    Removing a message in the mailbox.
    
    @param message_no Message number to remove.
    @throw email_error Removing message failure.
    @throw *          `parse_status(const string&)`, `dialog::send(const string&)`, `dialog::receive()`.
    **/
    void remove(unsigned long message_no);

protected:

    /**
    Initializing a connection to the server.

    @throw email_error Connection to server failure.
    @throw *          `parse_status(const string&)`, `dialog::receive()`.
    **/
    void connect();

    /**
    Authentication of a user.

    @param username    Username to authenticate.
    @param password    Password to authenticate.
    @throw email_error  Username rejection.
    @throw email_error  Password rejection.
    @throw *           `parse_status(const string&)`, `dialog::send(const string&)`, `dialog::receive()`.
    **/
    void auth_login(const std::string& username, const std::string& password);

    /**
    Parsing a response line for the status.
    
    @param line       Response line to parse.
    @return           Tuple with the status and rest of the line.
    @throw email_error Response status unknown.
    **/
    std::tuple<std::string, std::string> parse_status(const std::string& line);

    /**
    Dialog to use for send/receive operations.
    **/
    shared_ptr<dialog> _dlg;
};

#ifdef SUPPORT_OPENSSL
/**
Secure version of POP3 client.
**/
class MAILIO_EXPORT pop3s : public pop3
{
public:

    /**
    Available authentication methods.
    **/
    enum class auth_method_t {LOGIN, START_TLS};

    /**
    Making a connection to server.
    
    Parent constructor is called to do all the work.

    @param hostname Hostname of the server.
    @param port     Port of the server.
    @param timeout  Network timeout after which I/O operations fail. If zero, then no timeout is set i.e. I/O operations are synchronous.
    @throw *        `pop3::pop3(const string&, unsigned)`.
    **/
    pop3s(const shared_ptr<IOWorker>& worker, const std::string& hostname, unsigned port, uint32_t timeout = 10000);

    /**
    Sending the quit command and closing the connection.

    Parent destructor is called to do all the work.
    **/
    ~pop3s() = default;

    pop3s(const pop3s&) = delete;

    pop3s(pop3s&&) = delete;

    void operator=(const pop3s&) = delete;

    void operator=(pop3s&&) = delete;

    /**
    Authenticating with the given credentials.

    @param username Username to authenticate.
    @param password Password to authenticate.
    @param method   Authentication method to use.
    @throw *        `start_tls()`, `pop3::auth_login(const string&, const string&)`.
    **/
    void authenticate(const std::string& username, const std::string& password, auth_method_t method);

protected:

    /**
    Switching to TLS layer.
    
    @throw email_error Start TLS failure.
    @throw *          `parse_status(const string&)`, `dialog::send(const string&)`, `dialog::receive()`, `switch_to_ssl()`.
    **/
    void start_tls();

    /**
    Replacing a TCP socket with an SSL one.

    @throw * `dialog_ssl::dialog_ssl(dialog&&)`.
    **/
    void switch_to_ssl();
};

#endif

} // namespace mailio
