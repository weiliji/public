#ifndef __ONVIFPROTOCOL_CmdGetStreamURL_H__RemovePresets
#define __ONVIFPROTOCOL_CmdGetStreamURL_H__RemovePresets
#include "CmdObject.h"


class CmdRemovePreset :public CmdObject
{
public:
	CmdRemovePreset(uint32_t _index , const std::string& _token):CmdObject(URL_ONVIF_PTZ),token(_token), index(_index)
	{
	}
	virtual ~CmdRemovePreset() {}

	std::string token;
    uint32_t index;

	virtual std::string build(const URL& URL)
	{
		XML::Child& removepreset = body().addChild("RemovePreset");

		removepreset.addAttribute("xmlns", "http://www.onvif.org/ver20/ptz/wsdl");

		removepreset.addChild("ProfileToken", token);
		removepreset.addChild("PresetToken", index);

		return CmdObject::build(URL);
	}
	
	virtual bool parse(const XML::Child& body)
	{
		const XML::Child& respchild = body.getChild("RemovePresetResponse");
		if (respchild.isEmpty()) return false;

		return true;
	}
};



#endif //__ONVIFPROTOCOL_H__
