#ifndef __ONVIFPROTOCOL_CmdGetStreamURL_H__GotoPreset
#define __ONVIFPROTOCOL_CmdGetStreamURL_H__GotoPreset
#include "CmdObject.h"


class CmdGotoPreset :public CmdObject
{
public:
	CmdGotoPreset(const OnvifClientDefs::PresetInfo& _presetinfo,const std::string& _token):CmdObject(URL_ONVIF_PTZ),presetinfo(_presetinfo),token(_token)
	{
	}
	virtual ~CmdGotoPreset() {}

	OnvifClientDefs::PresetInfo presetinfo;
	std::string token;

	virtual std::string build(const URL& URL)
	{
		XMLObject::Child& gotoptreset = body().addChild("GotoPreset");

		gotoptreset.attribute("xmlns", "http://www.onvif.org/ver20/ptz/wsdl");

		gotoptreset.addChild("ProfileToken", token);
		gotoptreset.addChild("PresetToken", presetinfo.token);

		return CmdObject::build(URL);
	}
	virtual bool parse(const XMLObject::Child& body)
	{
		const XMLObject::Child& respchild = body.getChild("GotoPresetResponse");
		if (respchild.isEmpty()) return false;

		return true;
	}
};



#endif //__ONVIFPROTOCOL_H__
