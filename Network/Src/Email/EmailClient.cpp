#include "Network/Email/Client.h"
#include "mailio/pop3.hpp"
#include "mailio/imap.hpp"
#include "mailio/smtp.hpp"
#include <fstream>

namespace Public{
namespace Network{
namespace Email{
struct Client::ClientInternal
{
	shared_ptr<mailio::email>	email;

	ProtocolType				type;

	shared_ptr<IOWorker>		worker;
	std::string					emailserveraddr;
	uint32_t					emailserverport = 0;

	std::string					loginemail;

	void mailioattach(shared_ptr<mailio::message>& mailmsg, const std::string& filename)
	{
		if (!File::access(filename, File::accessExist)) return;

		std::string suffix;
		{
			size_t i = String::lastIndexOf(filename, ".");
			if (i != (size_t)-1) suffix = filename.c_str() + i;
		}

		mailio::mime::media_type_t type = mailio::mime::media_type_t::TEXT;
		if (String::iequals(suffix, "mp4") || String::iequals(suffix, "avi") || String::iequals(suffix, "mov") || String::iequals(suffix, "qt")
			|| String::iequals(suffix, "rm") || String::iequals(suffix, "navi") || String::iequals(suffix, "aiff")
			|| String::iequals(suffix, "mpeg") || String::iequals(suffix, "mpg") || String::iequals(suffix, "ram") || String::iequals(suffix, "viv")
			|| String::iequals(suffix, "rm") || String::iequals(suffix, "rmvb") || String::iequals(suffix, "wmv")
			|| String::iequals(suffix, "vob"))
		{
			type = mailio::mime::media_type_t::VIDEO;
		}
		else if (String::iequals(suffix, "mp3") || String::iequals(suffix, "cda") || String::iequals(suffix, "wav") || String::iequals(suffix, "mp2")
			|| String::iequals(suffix, "mp1") || String::iequals(suffix, "vqf") || String::iequals(suffix, "ra") || String::iequals(suffix, "ram")
			|| String::iequals(suffix, "asf") || String::iequals(suffix, "aca") || String::iequals(suffix, "acm") || String::iequals(suffix, "aif")
			|| String::iequals(suffix, "aifc") || String::iequals(suffix, "aiff") || String::iequals(suffix, "au") || String::iequals(suffix, "aac")
			|| String::iequals(suffix, "wma") || String::iequals(suffix, "mmf") || String::iequals(suffix, "amr") || String::iequals(suffix, "flac"))
		{
			type = mailio::mime::media_type_t::AUDIO;
		}
		else if (String::iequals(suffix, "bmp") || String::iequals(suffix, "gif") || String::iequals(suffix, "png") || String::iequals(suffix, "tif")
			|| String::iequals(suffix, "jpg") || String::iequals(suffix, "pic"))
		{
			type = mailio::mime::media_type_t::IMAGE;
		}
		else if (String::iequals(suffix, "exe") || String::iequals(suffix, "dll") || String::iequals(suffix, "com"))
		{
			type = mailio::mime::media_type_t::APPLICATION;
		}
		else if (String::iequals(suffix, "txt") || String::iequals(suffix, "doc") || String::iequals(suffix, "hlp") || String::iequals(suffix, "wps")
			|| String::iequals(suffix, "rtf") || String::iequals(suffix, "html") || String::iequals(suffix, "pdf") || String::iequals(suffix, "css")
			|| String::iequals(suffix, "js"))
		{
			type = mailio::mime::media_type_t::TEXT;
		}
		else
		{
			type = mailio::mime::media_type_t::NONE;
		}

		std::string name;
		{
			size_t i = String::lastIndexOf(filename, "/");
			if (i != (size_t)-1)
			{
				name = filename.c_str() + i;
			}
			else
			{
				size_t i = String::lastIndexOf(filename, "\\");
				if (i != (size_t)-1)
				{
					name = filename.c_str() + i;
				}
			}
		}

		ifstream ifs1(filename);
		if (!ifs1.is_open()) return;

		mailmsg->attach(ifs1, name, type, suffix);
	}
	shared_ptr<mailio::message> transMessage(const shared_ptr<Message>& msg)
	{
		shared_ptr<mailio::message> mailmsg = make_shared<mailio::message>();

		{
			const Address& addr = msg->from();

			if (addr.emtpy())
			{
				std::string name;
				{
					size_t i = String::indexOf(loginemail, "@");
					if (i != (size_t)-1) name = std::string(loginemail.c_str(), i);
				}
				
				mailio::mail_address maddr(name, loginemail);
				mailmsg->add_from(maddr);
			}
			else
			{
				mailio::mail_address maddr(addr.emailName(), addr.emailAddr());
				mailmsg->add_from(maddr);
			}
		}
		
		{
			const std::vector<Address>& addr = msg->to();

			for (size_t i = 0; i < addr.size(); i++)
			{
				mailio::mail_address maddr(addr[i].emailName(), addr[i].emailAddr());
				mailmsg->add_recipient(maddr);
			}
		}

		{
			const std::vector<Address>& addr = msg->copyTo();

			for (size_t i = 0; i < addr.size(); i++)
			{
				mailio::mail_address maddr(addr[i].emailName(), addr[i].emailAddr());
				mailmsg->add_cc_recipient(maddr);
			}
		}

		{
			const std::vector<Address>& addr = msg->bccTo();

			for (size_t i = 0; i < addr.size(); i++)
			{
				mailio::mail_address maddr(addr[i].emailName(), addr[i].emailAddr());
				mailmsg->add_bcc_recipient(maddr);
			}
		}

		mailmsg->subject(msg->subject());
		mailmsg->content(msg->content());

		{
			const std::vector<std::string>& attachments = msg->attachments();
			for (size_t i = 0; i < attachments.size(); i++)
			{
				mailioattach(mailmsg, attachments[i]);
			}
		}

		return mailmsg;
	}
};
Client::Client(const shared_ptr<IOWorker>& worker, const std::string& emailserveraddr, uint32_t emailserverport, ProtocolType type)
{
	internal = new ClientInternal;
	internal->type = type;
	internal->worker = worker;
	internal->emailserveraddr = emailserveraddr;
	internal->emailserverport = emailserverport;
}
Client::~Client()
{
	internal->email = NULL;
	SAFE_DELETE(internal);
}

ErrorInfo Client::login(const std::string& email, const std::string& password, uint32_t timeout)
{
	try
	{
		internal->loginemail = email;
		internal->email = NULL;

		switch (internal->type)
		{
		case ProtocolType_SMTP:
			internal->email = make_shared<mailio::smtp>(internal->worker, internal->emailserveraddr, internal->emailserverport, timeout);
			break;
		case ProtocolType_IMAP:
			internal->email = make_shared<mailio::imap>(internal->worker, internal->emailserveraddr, internal->emailserverport, timeout);
			break;
		case ProtocolType_POP3:
			internal->email = make_shared<mailio::pop3>(internal->worker, internal->emailserveraddr, internal->emailserverport, timeout);
			break;

#ifdef SUPPORT_OPENSSL
		case ProtocolType_SMTP_SSL:
			internal->email = make_shared<mailio::smtps>(internal->worker, internal->emailserveraddr, internal->emailserverport, timeout);
			break;
		case ProtocolType_IMAP_SSL:
			internal->email = make_shared<mailio::imaps>(internal->worker, internal->emailserveraddr, internal->emailserverport, timeout);
			break;
		case ProtocolType_POP3_SSL:
			internal->email = make_shared<mailio::pop3s>(internal->worker, internal->emailserveraddr, internal->emailserverport, timeout);
			break;
#endif

		default:
			return ErrorInfo(Error_Code_NotSupport, "协议不支持");
		}

		internal->email->authenticate(email, password, mailio::email::auth_method_t::LOGIN);
	}
	catch (const mailio::email_error& err)
	{
		internal->email = NULL;
		return ErrorInfo(Error_Code_Authen, err.what());
	}

	return ErrorInfo();
}
void Client::loginout()
{
	internal->email = NULL;
}

ErrorInfo Client::submit(const shared_ptr<Message>& msg)
{
	if (internal->type != ProtocolType_SMTP && internal->type != ProtocolType_SMTP_SSL) return ErrorInfo(Error_Code_NotSupport);

	if (internal->email == NULL) return ErrorInfo(Error_Code_Authen, "未认证");

	if (msg == NULL) return ErrorInfo(Error_Code_Param);

	shared_ptr<mailio::message> maimsg = internal->transMessage(msg);

	try {

		internal->email->submit(*maimsg.get());
	}
	catch (const mailio::email_error& err)
	{
		internal->email = NULL;
		return ErrorInfo(Error_Code_Authen, err.what());
	}

	return ErrorInfo();
}

ErrorInfo Client::fetch(uint32_t index, shared_ptr<Message>& msg)
{
	if (internal->type == ProtocolType_SMTP || internal->type == ProtocolType_SMTP_SSL) return ErrorInfo(Error_Code_NotSupport);

	return ErrorInfo(Error_Code_NotSupport);
}
}
};
};



