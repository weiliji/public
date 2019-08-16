#ifndef __ONVIFPROTOCOL_CmdGetStreamURL_H__SetHomePosition
#define __ONVIFPROTOCOL_CmdGetStreamURL_H__SetHomePosition
#include "CmdObject.h"


class CmdSetHomePosition :public CmdObject
{
public:
	CmdSetHomePosition()
	{
		action = "http://www.onvif.org/ver20/ptz/wsdl/SetHomePosition";
	}
	virtual ~CmdSetHomePosition() {}

	virtual std::string build(const URL& URL)
	{
		stringstream stream;

		stream << "<s:Envelope " << onvif_xml_ns << ">"
			<< buildHeader(URL)
			<< "<s:Body  xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">"
			<< "<SetHomePosition xmlns=\"http://www.onvif.org/ver20/ptz/wsdl\" />"
			<< "</s:Body></s:Envelope>";

		return stream.str();
	}
	virtual bool parse(const XMLObject::Child& body) = 0;
};



#endif //__ONVIFPROTOCOL_H__
