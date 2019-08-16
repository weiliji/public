#ifndef __ONVIFPROTOCOL_CmdGetStreamURL_H__
#define __ONVIFPROTOCOL_CmdGetStreamURL_H__
#include "CmdObject.h"


class CmdGetStreamURL :public CmdObject
{
public:
	CmdGetStreamURL(const std::string& _token) :CmdObject(URL_ONVIF_MEDIA),token(_token)
	{
	}
	virtual ~CmdGetStreamURL() {}

	virtual std::string build(const URL& URL)
	{
		XMLObject::Child& getstreamuri = body().addChild("GetStreamUri");

		getstreamuri.attribute("xmlns", "http://www.onvif.org/ver10/media/wsdl");

		getstreamuri.addChild("ProfileToken", token);


		XMLObject::Child& streamsetup = getstreamuri.addChild("StreamSetup");

		XMLObject::Child& stream = streamsetup.addChild("Stream","RTP-Unicast");
		stream.attribute("xmlns","http://www.onvif.org/ver10/schema");
		
		XMLObject::Child& transport = streamsetup.addChild("Transport");
		transport.attribute("xmlns", "http://www.onvif.org/ver10/schema");

		transport.addChild("Protocol","RTSP");

		return CmdObject::build(URL);
	}

	OnvifClientDefs::StreamUrl streamurl;
	virtual bool parse(const XMLObject::Child& body)
	{
		const XMLObject::Child& resp = body.getChild("GetStreamUriResponse");
		if (!resp) return false;

		streamurl.url = resp.getChild("MediaUri").getChild("Uri").data();

		if (streamurl.url == "") return false;

		return true;
	}

private:
	std::string token;
};



#endif //__ONVIFPROTOCOL_H__
