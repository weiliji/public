#ifndef __ONVIFPROTOCOL_H__GetSnapURL
#define __ONVIFPROTOCOL_H__GetSnapURL
#include "CmdObject.h"


class CmdGetSnapURL :public CmdObject
{
public:
	CmdGetSnapURL(const std::string& _token) :token(_token)
	{
		action = "http://www.onvif.org/ver10/media/wsdl/GetSnapshotURL";

		requesturl = MEDIAREQUESTURL;
	}
	virtual ~CmdGetSnapURL() {}

	virtual std::string build(const URL& URL)
	{
		stringstream stream;

		stream << "<s:Envelope " << onvif_xml_ns << ">"
			<< buildHeader(URL)
			<< "<s:Body xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">"
			<< "<GetSnapshotUri xmlns=\"http://www.onvif.org/ver10/media/wsdl\">"
			<< "<ProfileToken>" << token << "</ProfileToken>"
			<< "</GetSnapshotUri>"
			<< "</s:Body></s:Envelope>";

		return stream.str();
	}
	shared_ptr<OnvifClientDefs::SnapUrl> snapurl;
	virtual bool parse(const XMLObject::Child& body)
	{
		snapurl = make_shared<OnvifClientDefs::SnapUrl>();

		const XMLObject::Child& resp = body.getChild("GetSnapshotUriResponse");
		if (!resp) return false;

		snapurl->url = resp.getChild("MediaUri").getChild("Uri").data();

		if (snapurl->url == "") return false;

		return true;
	}

private:
	std::string token;
};



#endif //__ONVIFPROTOCOL_H__
