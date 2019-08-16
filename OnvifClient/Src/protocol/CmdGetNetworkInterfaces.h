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
		XMLObject::Child& getnetwork = body().addChild("GetNetworkInterfaces");

		getnetwork.attribute("xmlns","http://www.onvif.org/ver10/device/wsdl");

		return CmdObject::build(URL);
	}

	OnvifClientDefs::NetworkInterfaces network;
	virtual bool parse(const XMLObject::Child& body)
	{
		const XMLObject::Child& resp = body.getChild("GetNetworkInterfacesResponse");
		if (!resp) return false;

		const XMLObject::Child& networkinterface = resp.getChild("NetworkInterfaces");
		if (!networkinterface) return false;

		network.name = networkinterface.getChild("Info").getChild("Name").data();
		network.macaddr = networkinterface.getChild("Info").getChild("HwAddress").data();

		const XMLObject::Child& manual = networkinterface.getChild("IPv4").getChild("Config").getChild("Manual");
		if (manual)
		{
			if (!manual.data().empty()) network.dhcp = !manual.data().readBool();
			else
			{
				network.ipaddr = manual.getChild("Address").data();
			}
		}

		const XMLObject::Child& dhcp = networkinterface.getChild("IPv4").getChild("Config").getChild("DHCP");
		if (dhcp)
		{
			if (!dhcp.data().empty()) network.dhcp = dhcp.data().readBool();
			else
			{
				network.ipaddr = dhcp.getChild("Address").data();
			}
		}

		return true;
	}
};



#endif //__ONVIFPROTOCOL_H__
