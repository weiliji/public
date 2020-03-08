#ifndef __ONVIFPROTOCOL_CmdGetStreamURL_H__CmdGetImageOptions
#define __ONVIFPROTOCOL_CmdGetStreamURL_H__CmdGetImageOptions
#include "CmdObject.h"


class CmdGetImageOptions :public CmdObject
{
public:
	CmdGetImageOptions(const std::string& _token):CmdObject(URL_ONVIF_IMAGING),token(_token)
	{
	}
	virtual ~CmdGetImageOptions() {}

    std::string token;
	virtual std::string build(const URL& URL)
	{
		XML::Child& method = body().addChild("GetOptions");

        method.addAttribute("xmlns","http://www.onvif.org/ver20/imaging/wsdl");

        method.addChild("VideoSourceToken", token);

		return CmdObject::build(URL);
	}

	OnvifClientDefs::ImageOptions options;
	virtual bool parse(const XML::Child& body)
	{
		const XML::Child& resp = body.getChild("GetOptionsResponse");
		if (!resp) return false;

		const XML::Child& imagingOptions = resp.getChild("ImagingOptions");
		if (!imagingOptions) return false;

		options.brightness.min = imagingOptions.getChild("Brightness").getChild("Min") .data().readUint32();
		options.brightness.max = imagingOptions.getChild("Brightness").getChild("Max").data().readUint32();

		options.colorSaturation.min = imagingOptions.getChild("ColorSaturation").getChild("Min").data().readUint32();
		options.colorSaturation.max = imagingOptions.getChild("ColorSaturation").getChild("Max").data().readUint32();

		options.contrast.min = imagingOptions.getChild("Contrast").getChild("Min").data().readUint32();
		options.contrast.max = imagingOptions.getChild("Contrast").getChild("Max").data().readUint32();

		options.sharpness.min = imagingOptions.getChild("Sharpness").getChild("Min").data().readUint32();
		options.sharpness.max = imagingOptions.getChild("Sharpness").getChild("Max").data().readUint32();

		return true;
	}
};



#endif //__ONVIFPROTOCOL_H__
