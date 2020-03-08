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
			envelop().addAttribute("xmlns:a", "http://schemas.xmlsoap.org/ws/2004/08/addressing");
			body().removeAttribute("xmlns:xsi");
			body().removeAttribute("xmlns:xsd");
		}	


		header().action = "http://schemas.xmlsoap.org/ws/2005/04/discovery/Probe";
		header().messageID = "uuid:" + Guid::createGuid().getStringStream();
		header().replyTo = "http://schemas.xmlsoap.org/ws/2004/08/addressing/role/anonymous";
		header().to = "urn:schemas-xmlsoap-org:ws:2005:04:discovery";

			

		XML::Child& probe = body().addChild("Probe");
		probe.addAttribute("xmlns","http://schemas.xmlsoap.org/ws/2005/04/discovery");

		XML::Child& types = probe.addChild("d:Types","dp0:NetworkVideoTransmitter");
		types.addAttribute("xmlns:d","http://schemas.xmlsoap.org/ws/2005/04/discovery");
		types.addAttribute("xmlns:dp0", "http://www.onvif.org/ver10/network/wsdl");

		return CmdObject::build(URL);
	}

	shared_ptr<OnvifClientDefs::DiscoveryInfo>	info;
	virtual bool parse(const XML::Child& body)
	{
		info = make_shared<OnvifClientDefs::DiscoveryInfo>();

		const XML::Child& probeval = body.getChild("ProbeMatches").getChild("ProbeMatch");
		if (probeval.isEmpty()) return false;

		const XML::Child& typeval = probeval.getChild("Types");
		if (typeval.isEmpty()) return false;

		if (String::indexOfByCase(typeval.data(), " tds:Device") == (size_t)-1)
		{
			return false;
		}

		const XML::Child& scopesval = probeval.getChild("Scopes");
		if (scopesval.isEmpty()) return false;

		//parse name
		{
			std::vector<std::string> scopevals = String::split(scopesval.data(), " ");
			for (size_t i = 0; i < scopevals.size(); i++)
			{
				{
					const char* nameflag = "onvif://www.onvif.org/name/";

					size_t pos = String::indexOfByCase(scopevals[i], nameflag);
					if (pos != (size_t)-1)
					{
						info->name = URLEncoding::decode(scopevals[i].c_str() + pos + strlen(nameflag));
					}
				}
				
				{
					const char* macflag = "onvif://www.onvif.org/macaddress/";

					size_t pos = String::indexOfByCase(scopevals[i], macflag);
					if (pos != (size_t)-1)
					{
						info->mac = parseMacAddr(scopevals[i].c_str() + pos + strlen(macflag));
					}
				}
				
				{
					const char* modelflag = "onvif://www.onvif.org/model/";

					size_t pos = String::indexOfByCase(scopevals[i], modelflag);
					if (pos != (size_t)-1)
					{
						info->model = URLEncoding::decode(scopevals[i].c_str() + pos + strlen(modelflag));
					}
				}

                if(info->model == "")
                {
                    const char* modelflag = "onvif://www.onvif.org/hardware/";

                    size_t pos = String::indexOfByCase(scopevals[i], modelflag);
                    if (pos != (size_t)-1)
                    {
                        info->model = URLEncoding::decode(scopevals[i].c_str() + pos + strlen(modelflag));
                    }
                }
			}
		}

		const XML::Child& addrval = probeval.getChild("XAddrs");
		if (addrval.isEmpty()) return false;

		{
			std::vector<std::string> addrvals = String::split(addrval.data(), " ");
			for (size_t i = 0; i < addrvals.size(); i++)
			{
				URL url(addrvals[i]);

				info->addr= url.getHostname();
				info->port = url.getPort();
				break;
			}
		}

		return info->addr.length() > 0;
	}
};



#endif //__ONVIFPROTOCOL_H__
