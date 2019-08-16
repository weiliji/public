#ifndef __ONVIFPROTOCOL_CmdGetStreamURL_H__GotoHomePosition
#define __ONVIFPROTOCOL_CmdGetStreamURL_H__GotoHomePosition
#include "CmdObject.h"

#if 0

class CmdGotoHomePosition :public CmdObject
{
public:
	CmdGotoHomePosition()
	{
		action = "http://www.onvif.org/ver20/ptz/wsdl/GotoHomePosition";
	}
	virtual ~CmdGotoHomePosition() {}

	virtual std::string build(const URL& URL)
	{
		stringstream stream;

		stream << "<s:Envelope " << onvif_xml_ns << ">"
			<< buildHeader(URL)
			<< "<s:Body  xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">"
			<< "<GotoHomePosition xmlns=\"http://www.onvif.org/ver20/ptz/wsdl\" />"
			<< "</s:Body></s:Envelope>";

		return stream.str();
	}
	virtual bool parse(const XMLObject::Child& body) = 0;
};

#endif

#endif //__ONVIFPROTOCOL_H__
