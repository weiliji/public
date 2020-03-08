#ifndef __ONVIFPROTOCOL_CmdGetStreamURL_H__CmdGetImageSettings
#define __ONVIFPROTOCOL_CmdGetStreamURL_H__CmdGetImageSettings
#include "CmdObject.h"


class CmdGetImageSettings :public CmdObject
{
public:
    CmdGetImageSettings(const std::string& _token):CmdObject(URL_ONVIF_IMAGING),token(_token)
	{
	}
	virtual ~CmdGetImageSettings() {}

    std::string token;
	virtual std::string build(const URL& URL)
	{
		XML::Child& method = body().addChild("GetImagingSettings");

        method.addAttribute("xmlns","http://www.onvif.org/ver20/imaging/wsdl");

        method.addChild("VideoSourceToken", token);

		return CmdObject::build(URL);
	}

	OnvifClientDefs::ImageSettingInfo settingInfo;
	virtual bool parse(const XML::Child& body)
	{
		const XML::Child& resp = body.getChild("GetImagingSettingsResponse");
		if (!resp) return false;

		const XML::Child& imageSettings = resp.getChild("ImagingSettings");
		if (!imageSettings) return false;

        settingInfo.brightness = imageSettings.getChild("Brightness").data().readUint32();
        settingInfo.colorSaturation = imageSettings.getChild("ColorSaturation").data().readUint32();
        settingInfo.contrast = imageSettings.getChild("Contrast").data().readUint32();
        settingInfo.sharpness = imageSettings.getChild("Sharpness").data().readUint32();

        const XML::Child& exposure = imageSettings.getChild("Exposure");
		if (exposure)
		{
            settingInfo.exposure.mode = exposure.getChild("Mode").data().readString();
            settingInfo.exposure.minIris = exposure.getChild("MinIris").data().readUint32();
            settingInfo.exposure.maxIris = exposure.getChild("MaxIris").data().readUint32();
            settingInfo.exposure.iris = exposure.getChild("Iris").data().readUint32();
		}
        const XML::Child& focus = imageSettings.getChild("Focus");
        if (focus)
        {
            settingInfo.focus.mode = focus.getChild("AutoFocusMode").data().readString();
            settingInfo.focus.defaultSpeed = focus.getChild("DefaultSpeed").data().readFloat();
        }

		return true;
	}
};



#endif //__ONVIFPROTOCOL_H__
