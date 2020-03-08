#ifndef __ONVIFPROTOCOL_CmdGetStreamURL_H__GotoPreset
#define __ONVIFPROTOCOL_CmdGetStreamURL_H__GotoPreset
#include "CmdObject.h"


class CmdGotoPreset :public CmdObject
{
public:
	CmdGotoPreset(uint32_t _index, const std::string& _token):CmdObject(URL_ONVIF_PTZ), index(_index), token(_token)
	{
	}
	virtual ~CmdGotoPreset() {}

    uint32_t index;
	std::string token;

	virtual std::string build(const URL& URL)
	{
		XML::Child& gotoptreset = body().addChild("GotoPreset");

		gotoptreset.addAttribute("xmlns", "http://www.onvif.org/ver20/ptz/wsdl");

		gotoptreset.addChild("ProfileToken", token);
		gotoptreset.addChild("PresetToken", index);

		return CmdObject::build(URL);
	}
	virtual bool parse(const XML::Child& body)
	{
		const XML::Child& respchild = body.getChild("GotoPresetResponse");
		if (respchild.isEmpty()) return false;

		return true;
	}
};



#endif //__ONVIFPROTOCOL_H__
