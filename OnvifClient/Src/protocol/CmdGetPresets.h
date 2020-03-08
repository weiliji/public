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
		XML::Child& getpreset = body().addChild("GetPresets");

		getpreset.addAttribute("xmlns","http://www.onvif.org/ver20/ptz/wsdl");

		getpreset.addChild("ProfileToken",token);

		return CmdObject::build(URL);
	}

	OnvifClientDefs::PresetInfos preset;

	virtual bool parse(const XML::Child& body)
	{
		XML::Child respchild = body.getChild("GetPresetsResponse");
		if (respchild.isEmpty()) return false;

		for(XML::ChildIterator iter = respchild.child("Preset");iter;iter++)
		{
			const XML::Child& presetchild = *iter;
			OnvifClientDefs::PresetInfo info;
			info.token = presetchild.getAttribute("token").readUint32();
			info.name = presetchild.getChild("Name").data().readString();

			preset.infos.push_back(info);
		}

		return true;
	}
};



#endif //__ONVIFPROTOCOL_H__
