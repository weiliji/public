#ifndef __ONVIFPROTOCOL_CmdGetStreamURL_H__Stop
#define __ONVIFPROTOCOL_CmdGetStreamURL_H__Stop
#include "CmdObject.h"


class CmdStopPTZ :public CmdObject
{
public:
	CmdStopPTZ(const OnvifClientDefs::PTZCtrl& _ptz, const std::string& _token) :CmdObject(URL_ONVIF_PTZ),ptzctrl(_ptz), token(_token)
	{
	}
	virtual ~CmdStopPTZ() {}

	virtual std::string build(const URL& URL)
	{
		XMLObject::Child& stop = body().addChild("Stop");
		stop.attribute("xmlns", "http://www.onvif.org/ver20/ptz/wsdl");
		

		//add token
		{
			stop.addChild("ProfileToken", token);
		}

		stop.addChild("PanTilt", ptzctrl.ctrlType == OnvifClientDefs::PTZCtrl::PTZ_CTRL_PAN?"true":"false");
		stop.addChild("Zoom", ptzctrl.ctrlType == OnvifClientDefs::PTZCtrl::PTZ_CTRL_ZOOM ? "true" : "false");
		
		return CmdObject::build(URL);
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
