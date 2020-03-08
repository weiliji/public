#ifndef __ONVIFPROTOCOL_CmdGetStreamURL_H__SetPreset
#define __ONVIFPROTOCOL_CmdGetStreamURL_H__SetPreset
#include "CmdObject.h"


class CmdSetPreset :public CmdObject
{
public:
	CmdSetPreset(uint32_t _index, const std::string& _presetname,const std::string& _ptztoken):CmdObject(URL_ONVIF_PTZ), index(_index),presetname(_presetname),ptztoken(_ptztoken)
	{
	}
	virtual ~CmdSetPreset() {}

    uint32_t index;
	std::string presetname;
	std::string ptztoken;

	virtual std::string build(const URL& URL)
	{
		XML::Child& setpreset = body().addChild("SetPreset");

		setpreset.addAttribute("xmlns", "http://www.onvif.org/ver20/ptz/wsdl");

		setpreset.addChild("ProfileToken", ptztoken);
        setpreset.addChild("PresetToken", index);
		setpreset.addChild("PresetName", presetname);

		return CmdObject::build(URL);
	}
	virtual bool parse(const XML::Child& body)
	{
		const XML::Child& responseval = body.getChild("SetPresetResponse");

		return !responseval.isEmpty();
	}
};



#endif //__ONVIFPROTOCOL_H__
