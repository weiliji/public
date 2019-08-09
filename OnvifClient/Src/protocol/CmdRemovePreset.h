#ifndef __ONVIFPROTOCOL_CmdGetStreamURL_H__RemovePresets
#define __ONVIFPROTOCOL_CmdGetStreamURL_H__RemovePresets
#include "CmdObject.h"


class CmdRemovePreset :public CmdObject
{
public:
	CmdRemovePreset(const OnvifClientDefs::PresetInfo& _presetinfo,const std::string& _token):presetinfo(_presetinfo),token(_token)
	{
		action = "http://www.onvif.org/ver20/ptz/wsdl/GetPresets";

		requesturl = PTZREQUESTURL;
	}
	virtual ~CmdRemovePreset() {}

	std::string token;
	OnvifClientDefs::PresetInfo presetinfo;

	virtual std::string build(const URL& URL)
	{
		stringstream stream;

		stream << "<s:Envelope " << onvif_xml_ns << ">"
			<< buildHeader(URL)
			<< "<s:Body xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">"
			<< "<RemovePreset xmlns=\"http://www.onvif.org/ver20/ptz/wsdl\">"
			<< "<ProfileToken>" << token << "</ProfileToken>"
			<< "<PresetToken>" << presetinfo.token << "</PresetToken>"
			<< "</RemovePreset>"
			<< "</s:Body></s:Envelope>";

		return stream.str();
	}
	
	virtual bool parse(const XMLObject::Child& body)
	{
		const XMLObject::Child& respchild = body.getChild("RemovePresetResponse");
		if (respchild.isEmpty()) return false;

		return true;
	}
};



#endif //__ONVIFPROTOCOL_H__
