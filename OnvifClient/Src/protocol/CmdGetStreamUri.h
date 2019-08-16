#ifndef __ONVIFPROTOCOL_CmdGetStreamURL_H__
#define __ONVIFPROTOCOL_CmdGetStreamURL_H__
#include "CmdObject.h"


class CmdGetStreamURL :public CmdObject
{
public:
	CmdGetStreamURL(const std::string& _token) :token(_token)
	{
		action = "http://www.onvif.org/ver10/media/wsdl/GetStreamURL";

		requesturl = MEDIAREQUESTURL;
	}
	virtual ~CmdGetStreamURL() {}

	virtual std::string build(const URL& URL)
	{
		stringstream stream;

		stream << "<s:Envelope " << onvif_xml_ns << ">"
			<< buildHeader(URL)
			<< "<s:Body xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">"
			<< "<GetStreamUri xmlns=\"http://www.onvif.org/ver10/media/wsdl\">"
			<< "<StreamSetup>"
			<< "<Stream xmlns=\"http://www.onvif.org/ver10/schema\">RTP-Unicast</Stream>"
			<< "<Transport xmlns=\"http://www.onvif.org/ver10/schema\">"
			<< "<Protocol>RTSP</Protocol>"
			<< "</Transport>"
			<< "</StreamSetup>"
			<< "<ProfileToken>" << token << "</ProfileToken>"
			<< "</GetStreamUri>"
			<<"</s:Body></s:Envelope>";

		return stream.str();
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
