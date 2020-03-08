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
		XML::Child& getstreamuri = body().addChild("GetStreamUri");

		getstreamuri.addAttribute("xmlns", "http://www.onvif.org/ver10/media/wsdl");

		getstreamuri.addChild("ProfileToken", token);


		XML::Child& streamsetup = getstreamuri.addChild("StreamSetup");

		XML::Child& stream = streamsetup.addChild("Stream","RTP-Unicast");
		stream.addAttribute("xmlns","http://www.onvif.org/ver10/schema");
		
		XML::Child& transport = streamsetup.addChild("Transport");
		transport.addAttribute("xmlns", "http://www.onvif.org/ver10/schema");

		transport.addChild("Protocol","RTSP");

		return CmdObject::build(URL);
	}

	OnvifClientDefs::StreamUrl streamurl;
	virtual bool parse(const XML::Child& body)
	{
		const XML::Child& resp = body.getChild("GetStreamUriResponse");
		if (!resp) return false;

		streamurl.url = resp.getChild("MediaUri").getChild("Uri").data().readString();

		if (streamurl.url == "") return false;

		return true;
	}

private:
	std::string token;
};



#endif //__ONVIFPROTOCOL_H__
