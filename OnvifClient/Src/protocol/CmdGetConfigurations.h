#ifndef __ONVIFPROTOCOL_CmdGetStreamURL_H__GetConfigurations
#define __ONVIFPROTOCOL_CmdGetStreamURL_H__GetConfigurations
#include "CmdObject.h"


class CmdGetConfigurations :public CmdObject
{
public:
	CmdGetConfigurations()
	{
		action = "http://www.onvif.org/ver20/ptz/wsdl/GetConfigurations";
	}
	virtual ~CmdGetConfigurations() {}

	virtual std::string build(const URL& URL)
	{
		stringstream stream;

		stream << "<s:Envelope " << onvif_xml_ns << ">"
			<< buildHeader(URL)
			<< "<s:Body>"
			<< "<GetConfigurations xmlns=\"http://www.onvif.org/ver20/ptz/wsdl\" />"
			<< "</s:Body></s:Envelope>";

		return stream.str();
	}

	shared_ptr<OnvifClientDefs::PTZConfig> ptzcfg;
	virtual bool parse(const XMLObject::Child& body)
	{
		ptzcfg = make_shared<OnvifClientDefs::PTZConfig>();

		const XMLObject::Child& resp = body.getChild("GetConfigurationsResponse");
		if (!resp) return false;


		const XMLObject::Child& cfg = resp.getChild("PTZConfiguration");
		if (!cfg) return false;

		ptzcfg->token = cfg.attribute("token");

		ptzcfg->name = cfg.getChild("Name").data();
		ptzcfg->use_count = cfg.getChild("UseCount").data().readInt();
		ptzcfg->nodeToken = cfg.getChild("NodeToken").data();

		const XMLObject::Child& limits = cfg.getChild("PanTiltLimits");
		if (limits)
		{
			const XMLObject::Child& range = limits.getChild("Range");
			if (range)
			{
				const XMLObject::Child& xrange = range.getChild("XRange");
				if (xrange)
				{
					ptzcfg->pantilt_x.min = xrange.getChild("Min").data().readFloat();
					ptzcfg->pantilt_x.max = xrange.getChild("Max").data().readFloat();
				}
				const XMLObject::Child& yrange = range.getChild("YRange");
				if (yrange)
				{
					ptzcfg->pantilt_y.min = yrange.getChild("Min").data().readFloat();
					ptzcfg->pantilt_y.max = yrange.getChild("Max").data().readFloat();
				}
			}
		}

		const XMLObject::Child& zommlimits = cfg.getChild("ZoomLimits");
		if (zommlimits)
		{
			const XMLObject::Child& range = zommlimits.getChild("Range");
			if (range)
			{
				const XMLObject::Child& xrange = range.getChild("XRange");
				if (xrange)
				{
					ptzcfg->zoom.min = xrange.getChild("Min").data().readFloat();
					ptzcfg->zoom.max = xrange.getChild("Max").data().readFloat();
				}
			}
		}

		return true;
	}
};



#endif //__ONVIFPROTOCOL_H__
