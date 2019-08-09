#ifndef __ONVIFPROTOCOL_CmdGetStreamURL_H__Stop
#define __ONVIFPROTOCOL_CmdGetStreamURL_H__Stop
#include "CmdObject.h"


class CmdStopPTZ :public CmdObject
{
public:
	CmdStopPTZ(const OnvifClientDefs::PTZCtrl& _ptz, const std::string& _token) :ptzctrl(_ptz), token(_token)
	{
		action = "http://www.onvif.org/ver20/ptz/wsdl/Stop";
		requesturl = PTZREQUESTURL;
	}
	virtual ~CmdStopPTZ() {}

	virtual std::string build(const URL& URL)
	{
		stringstream stream;

		stream << "<s:Envelope "<< onvif_xml_ns<<">"
			<< buildHeader(URL)
			<< "<s:Body  xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">"
			<< "<Stop xmlns=\"http://www.onvif.org/ver20/ptz/wsdl\">"
			<< "<ProfileToken>"<<token<<"</ProfileToken>"
			<< "<PanTilt>"<< ((ptzctrl.ctrlType == OnvifClientDefs::PTZCtrl::PTZ_CTRL_PAN) ? "true" : "false") << "</PanTilt>"
			<< "<Zoom>"<<((ptzctrl.ctrlType == OnvifClientDefs::PTZCtrl::PTZ_CTRL_ZOOM) ? "true" : "false") << "</Zoom>"
			<< "</Stop>"
			<< "</s:Body></s:Envelope>";

		return stream.str();
	}
	virtual bool parse(const XMLObject::Child& body) 
	{
		const XMLObject::Child& respchild = body.getChild("StopResponse");

		return !respchild.isEmpty();
	}
private:
	OnvifClientDefs::PTZCtrl		ptzctrl;
	std::string token;
};



#endif //__ONVIFPROTOCOL_H__