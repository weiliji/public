#ifndef __ONVIFPROTOCOL_CmdGetStreamURL_H__GetPresets
#define __ONVIFPROTOCOL_CmdGetStreamURL_H__GetPresets
#include "CmdObject.h"


class CmdGetPresets :public CmdObject
{
public:
	CmdGetPresets(const std::string& _token):token(_token)
	{
		action = "http://www.onvif.org/ver20/ptz/wsdl/GetPresets";

		requesturl = PTZREQUESTURL;
	}
	virtual ~CmdGetPresets() {}

	std::string token;

	virtual std::string build(const URL& URL)
	{
		stringstream stream;

		stream << "<s:Envelope " << onvif_xml_ns << ">"
			<< buildHeader(URL)
			<< "<s:Body xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">"
			<< "<GetPresets xmlns=\"http://www.onvif.org/ver20/ptz/wsdl\">"
			<< "<ProfileToken>" << token << "</ProfileToken>"
			<< "</GetPresets>"
			<< "</s:Body></s:Envelope>";

		return stream.str();
	}

	OnvifClientDefs::PresetInfos preset;

	virtual bool parse(const XMLObject::Child& body)
	{
		XMLObject::Child respchild = body.getChild("GetPresetsResponse");
		if (respchild.isEmpty()) return false;

		XMLObject::Child& presetchild = respchild.firstChild("Preset");
		while (!presetchild.isEmpty())
		{
			OnvifClientDefs::PresetInfo info;
			info.token = presetchild.attribute("token");
			info.name = presetchild.getChild("Name").data();

			preset.infos.push_back(info);

			presetchild = respchild.nextChild();
		}

		return true;
	}
};



#endif //__ONVIFPROTOCOL_H__
