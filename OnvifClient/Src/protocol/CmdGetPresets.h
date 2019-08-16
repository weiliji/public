#ifndef __ONVIFPROTOCOL_CmdGetStreamURL_H__GetPresets
#define __ONVIFPROTOCOL_CmdGetStreamURL_H__GetPresets
#include "CmdObject.h"


class CmdGetPresets :public CmdObject
{
public:
	CmdGetPresets(const std::string& _token):CmdObject(URL_ONVIF_PTZ),token(_token)
	{
	}
	virtual ~CmdGetPresets() {}

	std::string token;

	virtual std::string build(const URL& URL)
	{
		XMLObject::Child& getpreset = body().addChild("GetPresets");

		getpreset.attribute("xmlns","http://www.onvif.org/ver20/ptz/wsdl");

		getpreset.addChild("ProfileToken",token);

		return CmdObject::build(URL);
	}

	OnvifClientDefs::PresetInfos preset;

	virtual bool parse(const XMLObject::Child& body)
	{
		XMLObject::Child respchild = body.getChild("GetPresetsResponse");
		if (respchild.isEmpty()) return false;

		XMLObject::Child& presetchild = respchild.firstChild("Preset");
		while (!presetchild.isEmpty())
		{
			OnvifClientDefs::PresetInfo info;
			info.token = presetchild.attribute("token");
			info.name = presetchild.getChild("Name").data();

			preset.infos.push_back(info);

			presetchild = respchild.nextChild();
		}

		return true;
	}
};



#endif //__ONVIFPROTOCOL_H__
