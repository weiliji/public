#pragma  once
#include "Base/Base.h"
using namespace Public::Base;

class GSop_Envelop
{
#define GSOP_ENVELOP_NAME		"Envelope"
#define GSOP_BODY_NAME			"Body"
#define GSOP_HEADER_NAME		"Header"

public:
	struct Header
	{
		struct
		{
			std::string  username;
			std::string	 password;
			Time		 createTime = Time::getCurrentTime();
		}security;

		std::string action;
		std::string messageID;
		std::string replyTo;
		std::string to;
	};
	GSop_Envelop() {}
	~GSop_Envelop() {}

	Header& header() { return _header; }


	XML::Child& envelop()
	{
		return xml.body().getChild(GSOP_ENVELOP_NAME);
	}

	XML::Child& body() 
	{
		return envelop().getChild(GSOP_BODY_NAME);
	}

	std::string buildGSopProtocol()
	{
		buildHeader();

		return xml.toString();
	}

	bool parseGSopProtocol(const std::string& httpbody)
	{
		if (!xml.parseBuffer(httpbody)) return false;

		if (xml.body().getChild(GSOP_ENVELOP_NAME).isEmpty()) return false;

		if (body().isEmpty()) return false;

		return true;
	}
	bool initGSopProtocol()
	{
#define GSOP_ENVELOP_FNAME		"s:" GSOP_ENVELOP_NAME
#define GSOP_HEADER_FNAME		"s:" GSOP_HEADER_NAME
#define GSOP_BODY_FNAME			"s:" GSOP_BODY_NAME

		{
			XML::Child envelop(GSOP_ENVELOP_FNAME);
			envelop.addAttribute("xmlns:s", "http://www.w3.org/2003/05/soap-envelope");
			envelop.addAttribute("xmlns:a", "http://www.w3.org/2005/08/addressing");

			xml.body().addChild(envelop);
		}
		{
			envelop().addChild(GSOP_HEADER_FNAME);
		}

		{
			XML::Child newbody(GSOP_BODY_FNAME);
			newbody.addAttribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
			newbody.addAttribute("xmlns:xsd", "http://www.w3.org/2001/XMLSchema");

			envelop().addChild(newbody);
		}

		return true;
	}
private:
	void buildHeader()
	{
		XML::Child& header = envelop().getChild(GSOP_HEADER_NAME);
		if (_header.action.length() > 0)
		{
			XML::Child action("a:Action", _header.action);
			action.addAttribute("s:mustUnderstand", "1");

			header.addChild(action);
		}
		if (_header.messageID.length() > 0)
		{
			XML::Child messageid("a:MessageID", _header.messageID);

			header.addChild(messageid);
		}
		if (_header.replyTo.length() > 0)
		{
			XML::Child replayto("a:ReplyTo");
			{
				XML::Child addr("a:Address", _header.replyTo);

				replayto.addChild(addr);
			}

			header.addChild(replayto);
		}
		if (_header.security.username.length() > 0 && _header.security.password.length() > 0)
		{
			buildSecurity(header);
		}
		if (_header.to.length() > 0)
		{
			XML::Child to("a:To", _header.to);
			to.addAttribute("s:mustUnderstand", "1");

			header.addChild(to);
		}
	}
	void buildSecurity(XML::Child& header)
	{
		XML::Child usernametoken("UsernameToken");
		{
			std::string noneBase64, passwdBase64;
			std::string createTime = onvif_build_datetime(_header.security.createTime);

			buildUserAuthenInfo(noneBase64, passwdBase64, createTime, _header.security.password);

			{
				XML::Child username("Username", _header.security.username);

				usernametoken.addChild(username);
			}
			{
				XML::Child password("Password", passwdBase64);
				password.addAttribute("Type", "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-username-token-profile-1.0#PasswordDigest");

				usernametoken.addChild(password);
			}
			{
				XML::Child nonce("Nonce", noneBase64);
				nonce.addAttribute("EncodingType", "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-soap-message-security-1.0#Base64Binary");

				usernametoken.addChild(nonce);
			}
			{
				XML::Child created("Created", createTime);
				created.addAttribute("xmlns", "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-utility-1.0.xsd");

				usernametoken.addChild(created);
			}
		}

		XML::Child security("Security");
		{
			security.addAttribute("s:mustUnderstand", "1");
			security.addAttribute("xmlns", "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd");
		}
		security.addChild(usernametoken);

		header.addChild(security);
	}
	void buildUserAuthenInfo(std::string& noneBase64, std::string & passwdBase64, std::string& createTime, const std::string& password)
	{
		std::string noneString = onvif_calc_nonce();
		noneBase64 = Base64::encode(noneString);

		std::string basetmp = onvif_calc_digest(createTime, noneString, password);
		passwdBase64 = Base64::encode(basetmp);
	}
private:
	static std::string onvif_calc_nonce()
	{
		static unsigned short count = 0xCA53;
		static Mutex count_Mutex;

		Guard locker(count_Mutex);

		char buf[32] = { 0 };
		/* we could have used raw binary instead of hex as below */
		snprintf_x(buf, 31, "%8.8x%4.4hx%8.8x", (int)Time::getCurrentTime().makeTime(), count++, (int)rand());


		return buf;
	}

	static std::string onvif_calc_digest(const std::string& created, const std::string& nonce, const std::string& password)
	{
		Sha1 sha1;

		sha1.input(nonce.c_str(), nonce.length());
		sha1.input(created.c_str(), created.length());
		sha1.input(password.c_str(), password.length());

		return sha1.report(Sha1::REPORT_BIN);
	}
public:
	static std::string onvif_build_datetime(const Time& time = 0)
	{
		char buf[32] = { 0 };

		Time nowtime = time;
		if(nowtime.makeTime() == 0)
			nowtime = Time::getCurrentTime();
		
		snprintf_x(buf, 32, "%04d-%02d-%02dT%02d:%02d:%02dZ", nowtime.year, nowtime.month, nowtime.day, nowtime.hour, nowtime.minute, nowtime.second);

		return buf;
	}
private:
	XML			xml;
	Header				_header;
};