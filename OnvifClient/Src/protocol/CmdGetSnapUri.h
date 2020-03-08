#ifndef __ONVIFPROTOCOL_H__GetSnapURL
#define __ONVIFPROTOCOL_H__GetSnapURL
#include "CmdObject.h"


class CmdGetSnapURL :public CmdObject
{
public:
	CmdGetSnapURL(const std::string& _token) :CmdObject(URL_ONVIF_MEDIA),token(_token)
	{
	}
	virtual ~CmdGetSnapURL() {}

	virtual std::string build(const URL& URL)
	{
		XML::Child& getsnapuri = body().addChild("GetSnapshotUri");

		getsnapuri.addAttribute("xmlns", "http://www.onvif.org/ver10/media/wsdl");

		getsnapuri.addChild("ProfileToken", token);

		return CmdObject::build(URL);
	}
	OnvifClientDefs::SnapUrl snapurl;
	virtual bool parse(const XML::Child& body)
	{
		const XML::Child& resp = body.getChild("GetSnapshotUriResponse");
		if (!resp) return false;

		snapurl.url = resp.getChild("MediaUri").getChild("Uri").data().readString();

		if (snapurl.url == "") return false;

		return true;
	}

private:
	std::string token;
};



#endif //__ONVIFPROTOCOL_H__
