/*

smtp.hpp
--------

Copyright (C) 2016, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/


#pragma once

#include <string>
#include <memory>
#include <tuple>
#include <stdexcept>
#include <chrono>
#include "mailmessage.hpp"
#include "dialog.hpp"
#include "export.hpp"
#include "email.hpp"

namespace mailio
{


/**
SMTP client implementation.
**/
class MAILIO_EXPORT smtp:public email
{
public:
    /**
    Making a connection to the server.

    @param hostname   Hostname of the server.
    @param port       Port of the server.
    @param timeout    Network timeout after which I/O operations fail. If zero, then no timeout is set i.e. I/O operations are synchronous.
    @throw email_error Empty source hostname not allowed.
    @throw *          `dialog::dialog`, `read_hostname`.
    **/
    smtp(const shared_ptr<IOWorker>& worker, const std::string& hostname, unsigned port, uint32_t timeout = 10000);

    /**
    Sending the quit command and closing the connection.
    **/
    virtual ~smtp();

    smtp(const smtp&) = delete;
    
    smtp(smtp&&) = delete;

    void operator=(const smtp&) = delete;

    void operator=(smtp&&) = delete;

    /**
    Authenticating with the given credentials.

    The method should be called only once on an existing object - it is not possible to authenticate again within the same connection.

    @param username Username to authenticate.
    @param password Password to authenticate.
    @param method   Authentication method to use.
    @throw *        `connect()`, `ehlo()`, `auth_login(const string&, const string&)`.
    **/
    void authenticate(const std::string& username, const std::string& password, auth_method_t method);

    /**
    Submitting a message.
    
    @param msg        Mail message to send.
    @throw email_error Mail sender rejection.
    @throw email_error Mail recipient rejection.
    @throw email_error Mail group recipient rejection.
    @throw email_error Mail cc recipient rejection.
    @throw email_error Mail group cc recipient rejection.
    @throw email_error Mail bcc recipient rejection.
    @throw email_error Mail group bcc recipient rejection.
    @throw email_error Mail message rejection.
    @throw *          `parse_line(const string&)`, `dialog::send(const string&)`, `dialog::receive()`.
    **/
    void submit(const message& msg);

    /**
    Setting the source hostname.

    @param src_host Source hostname to set.
    **/
    void source_hostname(const std::string& src_host);

    /**
    Getting source hostname.

    @return Source hostname.
    **/
    std::string source_hostname() const;

protected:

    /**
    SMTP response status.
    **/
    enum smtp_status_t {POSITIVE_COMPLETION = 2, POSITIVE_INTERMEDIATE = 3, TRANSIENT_NEGATIVE = 4, PERMANENT_NEGATIVE = 5};

    /**
    Initializing the connection to the server.

    @throw email_error Connection rejection.
    @throw *          `parse_line(const string&)`, `dialog::receive()`.
    **/
    void connect();

    /**
    Authenticating with the login method.

    @param username   Username to authenticate.
    @param password   Password to authenticate.
    @throw email_error Authentication rejection.
    @throw email_error Username rejection.
    @throw email_error Password rejection.
    @throw *          `parse_line(const string&)`, `dialog::send(const string&)`, `dialog::receive()`.
    **/
    void auth_login(const std::string& username, const std::string& password);

    /**
    Issuing `EHLO` and/or `HELO` commands.
    
    @throw email_error Initial message rejection.
    @throw *          `parse_line(const string&)`, `dialog::send(const string&)`, `dialog::receive()`.
    **/
    void ehlo();

    /**
    Reading the source hostname.

    @return           Source hostname.
    @throw email_error Reading hostname failure.
    **/
    std::string read_hostname();

    /**
    Checking if the status is 2XX.

    @param status Status to check.
    @return       True if does, false if not.
    **/
    static bool positive_completion(int status);

    /**
    Checking if the status is 3XX.

    @param status Status to check.
    @return       True if does, false if not.
    **/
    static bool positive_intermediate(int status);

    /**
    Checking if the status is 4XX.

    @param status Status to check.
    @return       True if does, false if not.
    **/
    static bool transient_negative(int status);

    /**
    Checking if the status is 5XX.

    @param status Status to check.
    @return       True if does, false if not.
    **/
    static bool permanent_negative(int status);

    /**
    Name of the host which client is connecting from.
    **/
    std::string _src_host;

    /**
    Dialog to use for send/receive operations.
    **/
    shared_ptr<dialog> _dlg;
};

#ifdef SUPPORT_OPENSSL
/**
Secure version of SMTP client.
**/
class MAILIO_EXPORT smtps : public smtp
{
public:

    /**
    Making a connection to the server.
    
    Parent constructor is called to do all the work.

    @param hostname Hostname of the server.
    @param port     Port of the server.
    @param timeout  Network timeout after which I/O operations fail. If zero, then no timeout is set i.e. I/O operations are synchronous.
    @throw *        `smtp::smtp(const string&, unsigned)`.
    **/
    smtps(const shared_ptr<IOWorker>& worker, const std::string& hostname, unsigned port, uint32_t timeout = 10000);

    /**
    Sending the quit command and closing the connection.
    
    Parent destructor is called to do all the work.
    **/
    ~smtps() = default;

    smtps(const smtps&) = delete;

    smtps(smtps&&) = delete;

    void operator=(const smtps&) = delete;

    void operator=(smtps&&) = delete;

    /**
    Authenticating with the given credentials.

    @param username Username to authenticate.
    @param password Password to authenticate.
    @param method   Authentication method to use.
    @throw *        `start_tls()`, `switch_to_ssl()`, `ehlo()`, `auth_login(const string&, const string&)`, `connect()`.
    **/
    void authenticate(const std::string& username, const std::string& password, auth_method_t method);

protected:

    /**
    Switching to TLS layer.
    
    @throw email_error Start TLS refused by server.
    @throw *          `parse_line(const string&)`, `ehlo()`, `dialog::send(const string&)`, `dialog::receive()`, `switch_to_ssl()`.
    **/
    void start_tls();

    /**
    Replaces TCP socket with SSL socket.

    @throw * `dialog_ssl::dialog_ssl(dialog_ssl&&)`.
    **/
    void switch_to_ssl();
};

#endif


} // namespace mailio
