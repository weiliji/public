/*

imap.cpp
--------

Copyright (C) 2016, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/


#pragma once

#include <chrono>
#include <list>
#include <map>
#include <stdexcept>
#include <string>
#include <tuple>
#include <cstdint>
#include "dialog.hpp"
#include "mailmessage.hpp"
#include "export.hpp"


namespace mailio
{


/**
IMAP client implementation.
**/
class MAILIO_EXPORT email
{
public:

	/**
	Messages indexed by their order number containing sizes in octets.
	**/
	typedef std::map<unsigned, unsigned long> message_list_t;

    /**
    Mailbox statistics structure.
    **/
    struct mailbox_stat_t
    {
        /**
        Statistics information to be retrieved.
        **/
        enum stat_info_t {DEFAULT = 0, UNSEEN = 1, UID_NEXT = 2, UID_VALIDITY = 4};

        /**
        Number of messages in the mailbox.
        **/
        unsigned long messages_no;

        /**
        Number of recent messages in the mailbox.
        **/
        unsigned long messages_recent;

        /**
        The non-zero number of unseen messages in the mailbox.

        Zero indicates the server did not report this and no assumptions can be made about the number of unseen messages.
        **/
        unsigned long messages_unseen;

        /**
        The non-zero message sequence number of the first unseen message in the mailbox.

        Zero indicates the server did not report this and no assumptions can be made about the first unseen message.
        **/
        unsigned long messages_first_unseen;

        /**
        The non-zero next unique identifier value of the mailbox.

        Zero indicates the server did not report this and no assumptions can be made about the next unique identifier.
        **/
        unsigned long uid_next;

        /**
        The non-zero unique identifier validity value of the mailbox.

        Zero indicates the server did not report this and does not support UIDs.
        **/
        unsigned long uid_validity;


		/**
		Size of the mailbox.
		**/
		unsigned long mailbox_size;

        /**
        Setting the number of messages to zero.
        **/
        mailbox_stat_t() : messages_no(0), messages_recent(0), messages_unseen(0), messages_first_unseen(0), uid_next(0), uid_validity(0), mailbox_size(0)
        {
        }
    };

    /**
    Mailbox folder tree.
    **/
    struct mailbox_folder
    {
        std::map<std::string, mailbox_folder> folders;
    };


	/**
	Available authentication methods.
	**/
	enum class auth_method_t { NONE, LOGIN, START_TLS };

    /**
    Single message ID or range of message IDs to be searched for,
    **/
    typedef std::pair<unsigned long, optional<unsigned long>> messages_range_t;

    /**
    Condition used by IMAP searching.

    It consists of key and value. Each key except the ALL has a value of the appropriate type: string, list of IDs, date.

    @todo Since both key and value types are known at compile time, perhaps they should be checked then instead at runtime.
    **/
    struct search_condition_t
    {
        /**
        Condition key to be used as message search criteria.
        **/
        enum key_type {ALL, ID_LIST, SUBJECT, FROM, TO, BEFORE_DATE, ON_DATE, SINCE_DATE} key;


        /**
        Condition value type to be used as message search criteria.

        Key ALL uses null because it does not need the value. Single ID can be given or range of IDs, or more than one range.
        **/
        typedef Variant  value_type;

        /**
        Condition value itself.
        **/
        value_type value;

        /**
        String used to send over IMAP.
        **/
        std::string imap_string;

        /**
        Creating the IMAP string of the given condition.

        @param condition_key   Key to search for.
        @param condition_value Value to search for, default (empty) value is meant for the ALL key.
        @throw imap_error      Invaid search condition.
        **/
        search_condition_t(key_type condition_key, value_type condition_value = value_type());
    };

    /**
    Creating a connection to a server.

    @param hostname Hostname of the server.
    @param port     Port of the server.
    @param timeout  Network timeout after which I/O operations fail. If zero, then no timeout is set i.e. I/O operations are synchronous.
    @throw *        `dialog::dialog(const string&, unsigned)`.
    **/
    email(){}

    /**
    Sending the logout command and closing the connection.
    **/    
    virtual ~email(){}

    /**
    Authenticating with the given credentials.

    The method should be called only once on an existing object - it is not possible to authenticate again within the same connection.
    
    @param username Username to authenticate.
    @param password Password to authenticate.
    @param method   Authentication method to use.
    @throw *        `connect()`, `auth_login(const string&, const string&)`.
    **/
    virtual void authenticate(const std::string& username, const std::string& password, auth_method_t method) = 0;

    /**
    Selecting a mailbox.
    
    @param folder_name Folder to list.
    @return            Mailbox statistics.
    @throw imap_error  Selecting mailbox failure.
    @throw imap_error  Parsing failure.
    @throw *           `parse_tag_result(const string&)`, `dialog::send(const string&)`, `dialog::receive()`.
    @todo              Add server error messages to exceptions.
    @todo              Catch exceptions of `stoul` function.
    **/
	virtual mailbox_stat_t select(const std::list<std::string>& folder_name, bool read_only = false) { return mailbox_stat_t(); }

    /**
    Fetching a message from the mailbox.
    
    Some servers report success if a message with the given number does not exist, so the method returns with the empty `msg`. Other considers
    fetching non-existing message to be an error, and an exception is thrown.
    
    @param mailbox     Mailbox to fetch from.
    @param message_no  Number of the message to fetch.
    @param msg         Message to store the result.
    @param header_only Flag if only the message header should be fetched.
    @throw imap_error  Fetching message failure.
    @throw imap_error  Parsing failure.
    @throw *           `parse_tag_result(const string&)`, `select(const string&)`, `parse_response(const string&)`,
                       `dialog::send(const string&)`, `dialog::receive()`, `message::parse(const string&, bool)`.
    @todo              Add server error messages to exceptions.
    **/
	virtual void fetch(const std::string& mailbox, unsigned long message_no, message& msg, bool header_only = false) {}

    /**
    Fetching a message from an already selected mailbox.

    A mailbox must already be selected before calling this method.

    Some servers report success if a message with the given number does not exist, so the method returns with the empty `msg`. Other considers
    fetching non-existing message to be an error, and an exception is thrown.

    @param message_no  Number of the message to fetch.
    @param msg         Message to store the result.
    @param is_uid      Using a message uid number instead of a message sequence number.
    @param header_only Flag if only the message header should be fetched.
    @throw imap_error  Fetching message failure.
    @throw imap_error  Parsing failure.
    @throw *           `parse_tag_result(const string&)`, `parse_response(const string&)`,
                       `dialog::send(const string&)`, `dialog::receive()`, `message::parse(const string&, bool)`.
    @todo              Add server error messages to exceptions.
    **/
	virtual void fetch(unsigned long message_no, message& msg, bool is_uid = false, bool header_only = false) {}

    /**
    Fetching messages from an already selected mailbox.

    A mailbox must already be selected before calling this method.

    Some servers report success if a message with the given number does not exist, so the method returns with the empty `msg`. Other considers
    fetching non-existing message to be an error, and an exception is thrown.

    @param messages_range Range of message numbers or UIDs to fetch.
    @param found_messages Map of messages to store the results, indexed by message number or uid.
                          It does not clear the map first, so that results can be accumulated.
    @param is_uids        Using message UID numbers instead of a message sequence numbers.
    @param header_only    Flag if only the message headers should be fetched.
    @param line_policy    Decoder line policy to use while parsing each message.
    @throw imap_error     Fetching message failure.
    @throw imap_error     Parsing failure.
    @throw *              `parse_tag_result(const string&)`, `parse_response(const string&)`,
                          `dialog::send(const string&)`, `dialog::receive()`, `message::parse(const string&, bool)`.
    @todo                 Add server error messages to exceptions.
    **/
	virtual void fetch(const std::list<messages_range_t> messages_range, std::map<unsigned long, message>& found_messages, bool is_uids = false,
		bool header_only = false, codec::line_len_policy_t line_policy = codec::line_len_policy_t::RECOMMENDED) {}

    /**
    Getting the mailbox statistics.

    The server might not support unseen, uidnext, or uidvalidity, which will cause an exception, so those parameters are optional.
    
    @param mailbox    Mailbox name.
    @param info       Statistics information to be retrieved.
    @return           Mailbox statistics.
    @throw imap_error Parsing failure.
    @throw imap_error Getting statistics failure.
    @throw *          `parse_tag_result(const string&)`, `parse_response(const string&)`, `dialog::send(const string&)`, `dialog::receive()`.
    @todo             Add server error messages to exceptions.
    @todo             Exceptions by `stoul()` should be rethrown as parsing failure.
    **/
	virtual mailbox_stat_t statistics(const std::string& mailbox, unsigned int info = mailbox_stat_t::DEFAULT) { return mailbox_stat_t(); }

    /**
    Removing a message from the given mailbox.
    
    @param mailbox    Mailbox to use.
    @param message_no Number of the message to remove.
    @throw imap_error Deleting message failure.
    @throw imap_error Parsing failure.
    @throw *          `select(const string&)`, `parse_tag_result(const string&)`, `remove(unsigned long, bool)`, `dialog::send(const string&)`, `dialog::receive()`.
    @todo             Add server error messages to exceptions.
    **/
    virtual void remove(const std::string& mailbox, unsigned long message_no) {}

    /**
    Removing a message from an already selected mailbox.
    
    @param message_no Number of the message to remove.
    @param is_uid     Using a message uid number instead of a message sequence number.
    @throw imap_error Deleting message failure.
    @throw imap_error Parsing failure.
    @throw *          `parse_tag_result(const string&)`, `dialog::send(const string&)`, `dialog::receive()`.
    @todo             Add server error messages to exceptions.
    @todo             Catch exceptions of `stoul` function.
    **/
    virtual void remove(unsigned long message_no, bool is_uid = false) {}

    /**
    Searching a mailbox.
    
    @param conditions  List of conditions taken in conjuction way.
    @param results     Store resulting list of message sequence numbers or UIDs here.
                       Does not clear the list first, so that results can be accumulated.
    @param want_uids   Return a list of message UIDs instead of message sequence numbers.
    @throw imap_error  Search mailbox failure.
    @throw imap_error  Parsing failure.
    @throw *           `parse_tag_result(const string&)`, `dialog::send(const string&)`, `dialog::receive()`.
    @todo              Add server error messages to exceptions.
    **/
    virtual void search(const std::list<search_condition_t>& conditions, std::list<unsigned long>& results, bool want_uids = false) {}

    /**
    Creating folder.

    @param folder_tree Folder to be created.
    @return            True if created, false if not.
    @throw imap_error  Parsing failure.
    @throw imap_error  Creating folder failure.
    @throw *           `folder_delimiter()`, `parse_tag_result(const string&)`, `dialog::send(const string&)`, `dialog::receive()`.
    **/
	virtual bool create_folder(const std::list<std::string>& folder_tree) { return false; }

    /**
    Listing folders.

    @param folder_name Folder to list.
    @return            Subfolder tree of the folder.
    **/
	virtual mailbox_folder list_folders(const std::list<std::string>& folder_name) {return mailbox_folder();}

    /**
    Deleting a folder.

    @param folder_name Folder to delete.
    @return            True if deleted, false if not.
    @throw imap_error  Parsing failure.
    @throw imap_error  Deleting folder failure.
    @throw *           `folder_delimiter()`, `parse_tag_result(const string&)`, `dialog::send(const string&)`, `dialog::receive()`.
    **/
	virtual bool delete_folder(const std::list<std::string>& folder_name) { return false; }

    /**
    Renaming a folder.

    @param old_name    Old name of the folder.
    @param new_name    New name of the folder.
    @return            True if renaming is successful, false if not.
    @throw imap_error  Parsing failure.
    @throw imap_error  Renaming folder failure.
    @throw *           `folder_delimiter()`, `parse_tag_result(const string&)`, `dialog::send(const string&)`, `dialog::receive()`.
    **/
	virtual bool rename_folder(const std::list<std::string>& old_name, const std::list<std::string>& new_name) { return false; }
   
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
    virtual void submit(const message& msg) {}

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
	virtual message_list_t list(unsigned message_no = 0) {return message_list_t();}
};


/**
Error thrown by IMAP client.
**/
class MAILIO_EXPORT email_error : public std::runtime_error
{
public:

    /**
    Calling parent constructor.

    @param msg  Error message.
    **/
    explicit email_error(const std::string& msg) : std::runtime_error(msg)
    {
    }

    /**
    Calling parent constructor.

    @param msg  Error message.
    **/
    explicit email_error(const char* msg) : std::runtime_error(msg)
    {
    }
};


} // namespace mailio
