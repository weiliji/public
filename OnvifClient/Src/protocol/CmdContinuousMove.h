#ifndef __ONVIFPROTOCOL_CmdGetStreamURL_H__ContinuousMove
#define __ONVIFPROTOCOL_CmdGetStreamURL_H__ContinuousMove
#include "CmdObject.h"


class CmdContinuousMove :public CmdObject
{
public:
	CmdContinuousMove(const OnvifClientDefs::PTZCtrl& _ptz,const std::string& _token):CmdObject(URL_ONVIF_PTZ),ptzctrl(_ptz),token(_token)
	{
	}
	virtual ~CmdContinuousMove() {}

	virtual std::string build(const URL& URL)
	{
		XMLObject::Child& continuousmove = body().addChild("ContinuousMove");

		{
			continuousmove.attribute("xmlns", "http://www.onvif.org/ver20/ptz/wsdl");
		}

		//add token
		{
			continuousmove.addChild("ProfileToken", token);
		}

		//add velocity
		{
			XMLObject::Child& velocity = continuousmove.addChild("Velocity");

			if (ptzctrl.ctrlType == OnvifClientDefs::PTZCtrl::PTZ_CTRL_PAN)
			{
				XMLObject::Child& pantilt = velocity.addChild("PanTilt");

				pantilt.attribute("x", Value(ptzctrl.panTiltX).readString());
				pantilt.attribute("y", Value(ptzctrl.panTiltY).readString());
				pantilt.attribute("xmlns", "http://www.onvif.org/ver10/schema");
			}
			else if (ptzctrl.ctrlType == OnvifClientDefs::PTZCtrl::PTZ_CTRL_ZOOM)
			{
				XMLObject::Child& zoom = velocity.addChild("Zoom");
				zoom.attribute("x", Value(ptzctrl.zoom).readString());
				zoom.attribute("xmlns", "http://www.onvif.org/ver10/schema");
			}
		}
		//add timeout
		if (ptzctrl.duration != 0)
		{
			XMLObject::Child& timeout = continuousmove.addChild("Timeout");

			timeout.data("PT"+Value(ptzctrl.duration).readString()+"S");
		}

		return CmdObject::build(URL);
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
