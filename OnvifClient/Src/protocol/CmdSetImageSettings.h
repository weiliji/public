#ifndef __ONVIFPROTOCOL_CmdGetStreamURL_H__CmdSetImageSettings
#define __ONVIFPROTOCOL_CmdGetStreamURL_H__CmdSetImageSettings
#include "CmdObject.h"


class CmdSetImageSettings :public CmdObject
{
public:
    CmdSetImageSettings(const std::string& _token, const OnvifClientDefs::ImageSettingInfo& _info) :CmdObject(URL_ONVIF_IMAGING), token(_token), info(_info)
    {
    }
    virtual ~CmdSetImageSettings() {}

    std::string token;
    OnvifClientDefs::ImageSettingInfo info;

    virtual std::string build(const URL& URL)
    {
        XML::Child& method = body().addChild("SetImagingSettings");
        method.addAttribute("xmlns", "http://www.onvif.org/ver20/imaging/wsdl");

        method.addChild("VideoSourceToken", token);
        method.addChild("ForcePersistence", true);

        XML::Child& imagingSettings = method.addChild("ImagingSettings");
        imagingSettings.addChild("Brightness", info.brightness).addAttribute("xmlns", "http://www.onvif.org/ver10/schema");
        imagingSettings.addChild("ColorSaturation", info.colorSaturation).addAttribute("xmlns", "http://www.onvif.org/ver10/schema");
        imagingSettings.addChild("Contrast", info.contrast).addAttribute("xmlns", "http://www.onvif.org/ver10/schema");
        imagingSettings.addChild("Sharpness", info.sharpness).addAttribute("xmlns", "http://www.onvif.org/ver10/schema");

		if (info.exposure.maxIris == 0 && info.exposure.minIris == 0)
		{

		}
		else
		{
			XML::Child& exposure = imagingSettings.addChild("Exposure");
			exposure.addAttribute("xmlns", "http://www.onvif.org/ver10/schema");
			exposure.addChild("Mode", info.exposure.mode);
			exposure.addChild("MinIris", info.exposure.minIris);
			exposure.addChild("MaxIris", info.exposure.maxIris);
			exposure.addChild("Iris", info.exposure.iris);
		}
       
		if (info.focus.mode.empty())
		{

		}
		else
		{
			XML::Child& focus = imagingSettings.addChild("Focus");
			focus.addAttribute("xmlns", "http://www.onvif.org/ver10/schema");
			focus.addChild("AutoFocusMode", info.focus.mode);
			focus.addChild("DefaultSpeed", info.focus.defaultSpeed);
		}

        return CmdObject::build(URL);
    }
    virtual bool parse(const XML::Child& body)
    {
        const XML::Child& responseval = body.getChild("SetImagingSettingsResponse");

        return !responseval.isEmpty();
    }
};



#endif //__ONVIFPROTOCOL_H__
