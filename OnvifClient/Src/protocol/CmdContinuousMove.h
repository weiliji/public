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
		XML::Child& continuousmove = body().addChild("ContinuousMove");

		{
			continuousmove.addAttribute("xmlns", "http://www.onvif.org/ver20/ptz/wsdl");
		}

		//add token
		{
			continuousmove.addChild("ProfileToken", token);
		}

		//add velocity
		{
			XML::Child& velocity = continuousmove.addChild("Velocity");

			if (ptzctrl.ctrlType == OnvifClientDefs::PTZCtrl::PTZ_CTRL_PAN)
			{
				XML::Child& pantilt = velocity.addChild("PanTilt");

				pantilt.addAttribute("x", Value(ptzctrl.panTiltX));
				pantilt.addAttribute("y", Value(ptzctrl.panTiltY));
				pantilt.addAttribute("xmlns", "http://www.onvif.org/ver10/schema");
			}
			else if (ptzctrl.ctrlType == OnvifClientDefs::PTZCtrl::PTZ_CTRL_ZOOM)
			{
				XML::Child& zoom = velocity.addChild("Zoom");
				zoom.addAttribute("x", Value(ptzctrl.zoom));
				zoom.addAttribute("xmlns", "http://www.onvif.org/ver10/schema");
			}
		}
		//add timeout
		if (ptzctrl.duration != 0)
		{
			XML::Child& timeout = continuousmove.addChild("Timeout");

			timeout.data("PT"+Value(ptzctrl.duration).readString()+"S");
		}

        std::string str = CmdObject::build(URL);
		return str;
	}

	virtual bool parse(const XML::Child& body)
	{
		const XML::Child& respchild = body.getChild("ContinuousMoveResponse");

		return !respchild.isEmpty();
	}
private:
	OnvifClientDefs::PTZCtrl		ptzctrl;
	std::string token;
};



#endif //__ONVIFPROTOCOL_H__
