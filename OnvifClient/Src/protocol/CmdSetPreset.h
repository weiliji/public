#ifndef __ONVIFPROTOCOL_CmdGetStreamURL_H__SetPreset
#define __ONVIFPROTOCOL_CmdGetStreamURL_H__SetPreset
#include "CmdObject.h"


class CmdSetPreset :public CmdObject
{
public:
	CmdSetPreset(const std::string& _presetname,const std::string& _ptztoken):presetname(_presetname),ptztoken(_ptztoken)
	{
		action = "http://www.onvif.org/ver20/ptz/wsdl/SetPreset";

		requesturl = PTZREQUESTURL;
	}
	virtual ~CmdSetPreset() {}

	std::string presetname;
	std::string ptztoken;

	virtual std::string build(const URL& URL)
	{
		stringstream stream;

		stream << "<s:Envelope " << onvif_xml_ns << ">"
			<< buildHeader(URL)
			<< "<s:Body xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">"
			<< "<SetPreset xmlns=\"http://www.onvif.org/ver20/ptz/wsdl\">"
			<< "<ProfileToken>" << ptztoken << "</ProfileToken>"
			<< "<PresetName>" << presetname << "</PresetName>"
			<< "</SetPreset>"
			<< "</s:Body></s:Envelope>";

		return stream.str();
	}
	virtual bool parse(const XMLObject::Child& body)
	{
		const XMLObject::Child& responseval = body.getChild("SetPresetResponse");

		return !responseval.isEmpty();
	}
};



#endif //__ONVIFPROTOCOL_H__
