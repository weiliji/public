#ifndef __ONVIFPROTOCOL_CmdGetStreamURL_H__ContinuousMove
#define __ONVIFPROTOCOL_CmdGetStreamURL_H__ContinuousMove
#include "CmdObject.h"


class CmdContinuousMove :public CmdObject
{
public:
	CmdContinuousMove(const OnvifClientDefs::PTZCtrl& _ptz,const std::string& _token):ptzctrl(_ptz),token(_token) 
	{
		action = "http://www.onvif.org/ver20/ptz/wsdl/ContinuousMove";

		requesturl = PTZREQUESTURL;
	}
	virtual ~CmdContinuousMove() {}

	virtual std::string build(const URL& URL)
	{
		stringstream stream;

		stream << "<s:Envelope "<< onvif_xml_ns  << ">"
			<< buildHeader(URL)
			<< "<s:Body xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">"
			<< "<ContinuousMove xmlns=\"http://www.onvif.org/ver20/ptz/wsdl\">"
			<< "<ProfileToken>" << token << "</ProfileToken>"
			<< "<Velocity>";
		if (ptzctrl.ctrlType == OnvifClientDefs::PTZCtrl::PTZ_CTRL_PAN)
		{
			stream << "<PanTilt x=\""<< ptzctrl.panTiltX << "\" y=\"" << ptzctrl.panTiltY << "\" ";
		}
		else if (ptzctrl.ctrlType == OnvifClientDefs::PTZCtrl::PTZ_CTRL_ZOOM)
		{
			stream << "<Zoom x=\""<< ptzctrl.zoom<<"\" ";
		}
		stream << "xmlns=\"http://www.onvif.org/ver10/schema\" />"
			<< "</Velocity>";
		if (ptzctrl.duration != 0)
			stream << "<Timeout>PT" << ptzctrl.duration << "S</Timeout>";
		stream << "</ContinuousMove>"
			<< "</s:Body></s:Envelope>";

		return stream.str();
	}

	virtual bool parse(const XMLObject::Child& body)
	{
		const XMLObject::Child& respchild = body.getChild("ContinuousMoveResponse");

		return !respchild.isEmpty();
	}
private:
	OnvifClientDefs::PTZCtrl		ptzctrl;
	std::string token;
};



#endif //__ONVIFPROTOCOL_H__
