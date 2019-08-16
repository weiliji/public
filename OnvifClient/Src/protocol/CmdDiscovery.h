#ifndef __ONVIFPROTOCOL_PROFILES_H__CmdDiscovery
#define __ONVIFPROTOCOL_PROFILES_H__CmdDiscovery
#include "CmdObject.h"


class CmdDiscovery :public CmdObject
{
public:
	CmdDiscovery():CmdObject("")
	{
	}
	virtual ~CmdDiscovery() {}

	virtual std::string build(const URL& URL)
	{
		{
			envelop().attribute("xmlns:a", "http://schemas.xmlsoap.org/ws/2004/08/addressing");
			body().removeAttribute("xmlns:xsi");
			body().removeAttribute("xmlns:xsd");
		}	


		header().action = "http://schemas.xmlsoap.org/ws/2005/04/discovery/Probe";
		header().messageID = "uuid:" + Guid::createGuid().getStringStream();
		header().replyTo = "http://schemas.xmlsoap.org/ws/2004/08/addressing/role/anonymous";
		header().to = "urn:schemas-xmlsoap-org:ws:2005:04:discovery";

			

		XMLObject::Child& probe = body().addChild("Probe");
		probe.attribute("xmlns","http://schemas.xmlsoap.org/ws/2005/04/discovery");

		XMLObject::Child& types = probe.addChild("d:Types","dp0:NetworkVideoDisplay");
		types.attribute("xmlns:d","http://schemas.xmlsoap.org/ws/2005/04/discovery");
		types.attribute("xmlns:dp0", "http://www.onvif.org/ver10/network/wsdl");

		return CmdObject::build(URL);
	}

	std::string parseMacAddr(const std::string& macstr)
	{
		int mac[6] = {0,};

		char macaddrstr[64] = { 0 };
		if (sscanf(macstr.c_str(), "%02d%02d%02d%02d%02d%02d", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]) == 6)
		{
			sprintf(macaddrstr, "%02d-%02d-%02d-%02d-%02d-%02d", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
		}

		return macaddrstr;
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
				if (pos != -1)
				{
					info->name = scopevals[i].c_str() + strlen(nameflag);
				}

				const char* macflag = "onvif://www.onvif.org/macaddress/";
				
				pos = String::indexOfByCase(scopevals[i], macflag);
				if (pos != -1)
				{
					info->name = parseMacAddr(scopevals[i].c_str() + strlen(macflag));
				}
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
