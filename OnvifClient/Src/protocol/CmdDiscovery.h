#ifndef __ONVIFPROTOCOL_PROFILES_H__CmdDiscovery
#define __ONVIFPROTOCOL_PROFILES_H__CmdDiscovery
#include "CmdObject.h"


class CmdDiscovery :public CmdObject
{
public:
	CmdDiscovery()
	{
		action = "http://schemas.xmlsoap.org/ws/2005/04/discovery/Probe";
	}
	virtual ~CmdDiscovery() {}

	virtual std::string build(const URL& URL)
	{
		stringstream stream;

		stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
			<< "<s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\" xmlns:a=\"http://www.w3.org/2005/08/addressing\">"
			<< "<s:Header>"
			<< "<a:Action s:mustUnderstand=\"1\">http://schemas.xmlsoap.org/ws/2005/04/discovery/Probe</a:Action>"
			<< "<a:MessageID>uuid:" << Guid::createGuid().getStringStream() << "</a:MessageID>"
			<< "<a:ReplyTo>"
			<< "<a:Address>http://schemas.xmlsoap.org/ws/2004/08/addressing/role/anonymous</a:Address>"
			<< "</a:ReplyTo>"
			<< "<a:To s:mustUnderstand=\"1\">urn:schemas-xmlsoap-org:ws:2005:04:discovery</a:To>"
			<< "</s:Header>"
			<< "<s:Body><Probe xmlns=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\">"
			<< "<d:Types xmlns:d=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\" xmlns:dp0=\"http://www.onvif.org/ver10/network/wsdl\">dp0:NetworkVideoTransmitter</d:Types>"
			<< "</Probe></s:Body></s:Envelope>";

		return stream.str();
	}

	shared_ptr<OnvifClientDefs::DiscoveryInfo>	info;
	virtual bool parse(const XMLObject::Child& body)
	{
		info = make_shared<OnvifClientDefs::DiscoveryInfo>();

		const XMLObject::Child& probeval = body.getChild("ProbeMatches").getChild("ProbeMatch");
		if (probeval.isEmpty()) return false;

		const XMLObject::Child& typeval = probeval.getChild("Types");
		if (typeval.isEmpty()) return false;

		if (String::indexOfByCase(typeval.data(), " tds:Device") == -1)
		{
			return false;
		}

		const XMLObject::Child& scopesval = probeval.getChild("Scopes");
		if (scopesval.isEmpty()) return false;

		//parse name
		{
			std::vector<std::string> scopevals = String::split(scopesval.data(), " ");
			for (size_t i = 0; i < scopevals.size(); i++)
			{
				const char* nameflag = "onvif://www.onvif.org/name/";

				size_t pos = String::indexOfByCase(scopevals[i], nameflag);
				if(pos == -1) continue;

				info->name = scopevals[i].c_str() + strlen(nameflag);
				break;
			}
		}

		const XMLObject::Child& addrval = probeval.getChild("XAddrs");
		if (addrval.isEmpty()) return false;

		{
			std::vector<std::string> addrvals = String::split(addrval.data(), " ");
			for (size_t i = 0; i < addrvals.size(); i++)
			{
				URL url(addrvals[i]);

				std::string addr = url.getHost();

				info->addrs.push_back(addr);
			}
		}


		return info->addrs.size() > 0;
	}
};



#endif //__ONVIFPROTOCOL_H__
