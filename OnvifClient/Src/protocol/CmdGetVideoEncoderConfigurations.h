#ifndef __ONVIFPROTOCOL_CmdGetStreamURL_H__GetVideoEncoderConfigurations
#define __ONVIFPROTOCOL_CmdGetStreamURL_H__GetVideoEncoderConfigurations
#include "CmdObject.h"


class CmdGetVideoEncoderConfigurations :public CmdObject
{
public:
	CmdGetVideoEncoderConfigurations():CmdObject(URL_ONVIF_MEDIA)
	{
	}
	virtual ~CmdGetVideoEncoderConfigurations() {}

	virtual std::string build(const URL& URL)
	{
		XML::Child& getvideoencoderconf = body().addChild("GetVideoEncoderConfigurations");

		getvideoencoderconf.addAttribute("xmlns", "http://www.onvif.org/ver10/media/wsdl");

		return CmdObject::build(URL);
	}
	shared_ptr<OnvifClientDefs::VideoEncoderConfigurations> encoder;
	virtual bool parse(const XML::Child& body) { return false; }
};



#endif //__ONVIFPROTOCOL_H__
