#ifndef __ONVIFPROTOCOL_CmdGetStreamURL_H__GetNetworkInterfaces
#define __ONVIFPROTOCOL_CmdGetStreamURL_H__GetNetworkInterfaces
#include "CmdObject.h"


class CmdGetNetworkInterfaces :public CmdObject
{
public:
	CmdGetNetworkInterfaces():CmdObject(URL_ONVIF_DEVICE_SERVICE)
	{
	}
	virtual ~CmdGetNetworkInterfaces() {}

	virtual std::string build(const URL& URL)
	{
		XML::Child& getnetwork = body().addChild("GetNetworkInterfaces");

		getnetwork.addAttribute("xmlns","http://www.onvif.org/ver10/device/wsdl");

		return CmdObject::build(URL);
	}

	OnvifClientDefs::NetworkInterfaces network;
	virtual bool parse(const XML::Child& body)
	{
		const XML::Child& resp = body.getChild("GetNetworkInterfacesResponse");
		if (!resp) return false;

		const XML::Child& networkinterface = resp.getChild("NetworkInterfaces");
		if (!networkinterface) return false;

		network.name = networkinterface.getChild("Info").getChild("Name").data().readString();
		network.macaddr = parseMacAddr(networkinterface.getChild("Info").getChild("HwAddress").data().readString());

		const XML::Child& manual = networkinterface.getChild("IPv4").getChild("Config").getChild("Manual");
		if (manual)
		{
			if (!manual.data().empty()) network.dhcp = !manual.data().readBool();
			else
			{
				network.ipaddr = manual.getChild("Address").data().readString();
			}
		}

		const XML::Child& dhcp = networkinterface.getChild("IPv4").getChild("Config").getChild("DHCP");
		if (dhcp)
		{
			if (!dhcp.data().empty()) network.dhcp = dhcp.data().readBool();
			else
			{
				network.ipaddr = dhcp.getChild("Address").data().readString();
			}
		}

		return true;
	}
};



#endif //__ONVIFPROTOCOL_H__
