#ifndef __ONVIFPROTOCOL_PROFILES_H__SystemReboot
#define __ONVIFPROTOCOL_PROFILES_H__SystemReboot
#include "CmdObject.h"


class CMDSystemReboot :public CmdObject
{
public:
	CMDSystemReboot():CmdObject(URL_ONVIF_DEVICE_SERVICE)
	{
	}
	virtual ~CMDSystemReboot() {}

	virtual std::string build(const URL& URL)
	{
		XMLObject::Child& sysreboot = body().addChild("SystemReboot");

		sysreboot.attribute("xmlns", "http://www.onvif.org/ver10/device/wsdl");

		return CmdObject::build(URL);
	}
	virtual bool parse(const XMLObject::Child& body)
	{
		XMLObject::Child respchild = body.getChild("SystemRebootResponse");
		if (respchild.isEmpty()) return false;

		return true;
	}
};



#endif //__ONVIFPROTOCOL_H__
