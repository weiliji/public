#ifndef __ONVIFPROTOCOL_CmdGetStreamURL_H__RemovePresets
#define __ONVIFPROTOCOL_CmdGetStreamURL_H__RemovePresets
#include "CmdObject.h"


class CmdRemovePreset :public CmdObject
{
public:
	CmdRemovePreset(const OnvifClientDefs::PresetInfo& _presetinfo,const std::string& _token):CmdObject(URL_ONVIF_PTZ),presetinfo(_presetinfo),token(_token)
	{
	}
	virtual ~CmdRemovePreset() {}

	std::string token;
	OnvifClientDefs::PresetInfo presetinfo;

	virtual std::string build(const URL& URL)
	{
		XMLObject::Child& removepreset = body().addChild("RemovePreset");

		removepreset.attribute("xmlns", "http://www.onvif.org/ver20/ptz/wsdl");

		removepreset.addChild("ProfileToken", token);
		removepreset.addChild("PresetToken", presetinfo.token);

		return CmdObject::build(URL);
	}
	
	virtual bool parse(const XMLObject::Child& body)
	{
		const XMLObject::Child& respchild = body.getChild("RemovePresetResponse");
		if (respchild.isEmpty()) return false;

		return true;
	}
};



#endif //__ONVIFPROTOCOL_H__
