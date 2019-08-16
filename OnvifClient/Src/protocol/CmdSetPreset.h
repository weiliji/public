#ifndef __ONVIFPROTOCOL_CmdGetStreamURL_H__SetPreset
#define __ONVIFPROTOCOL_CmdGetStreamURL_H__SetPreset
#include "CmdObject.h"


class CmdSetPreset :public CmdObject
{
public:
	CmdSetPreset(const std::string& _presetname,const std::string& _ptztoken):CmdObject(URL_ONVIF_PTZ),presetname(_presetname),ptztoken(_ptztoken)
	{
	}
	virtual ~CmdSetPreset() {}

	std::string presetname;
	std::string ptztoken;

	virtual std::string build(const URL& URL)
	{
		XMLObject::Child& setpreset = body().addChild("SetPreset");

		setpreset.attribute("xmlns", "http://www.onvif.org/ver20/ptz/wsdl");

		setpreset.addChild("ProfileToken", ptztoken);
		setpreset.addChild("PresetName", presetname);

		return CmdObject::build(URL);
	}
	virtual bool parse(const XMLObject::Child& body)
	{
		const XMLObject::Child& responseval = body.getChild("SetPresetResponse");

		return !responseval.isEmpty();
	}
};



#endif //__ONVIFPROTOCOL_H__
