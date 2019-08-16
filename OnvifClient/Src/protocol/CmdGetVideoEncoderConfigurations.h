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
		XMLObject::Child& getvideoencoderconf = body().addChild("GetVideoEncoderConfigurations");

		getvideoencoderconf.attribute("xmlns", "http://www.onvif.org/ver10/media/wsdl");

		return CmdObject::build(URL);
	}
	shared_ptr<OnvifClientDefs::VideoEncoderConfigurations> encoder;
	virtual bool parse(const XMLObject::Child& body) { return false; }
};



#endif //__ONVIFPROTOCOL_H__
