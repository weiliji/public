#ifndef __ONVIFPROTOCOL_H__GetConfigurationOptions
#define __ONVIFPROTOCOL_H__GetConfigurationOptions
#include "CmdObject.h"
#if 0

class CmdGetConfigurationOptions :public CmdObject
{
public:
	CmdGetConfigurationOptions(const std::string& _token) :token(_token)
	{
		action = "http://www.onvif.org/ver20/ptz/wsdl/GetConfigurationOptions";
	}
	virtual ~CmdGetConfigurationOptions() {}

	virtual std::string build(const URL& URL)
	{
		stringstream stream;

		stream << "<s:Envelope " << onvif_xml_ns << ">"
			<< buildHeader(URL)
			<< "<s:Body>"
			<< "<GetConfigurationOptions xmlns=\"http://www.onvif.org/ver20/ptz/wsdl\">"
			<< "<ConfigurationToken>"<< token <<"</ConfigurationToken>"
			<< "</GetConfigurationOptions>"
			<< "</s:Body></s:Envelope>";

		return stream.str();
	}
	shared_ptr<OnvifClientDefs::ConfigurationOptions> options;
	virtual bool parse(const XMLObject::Child& body)
	{
		options = make_shared<OnvifClientDefs::ConfigurationOptions>();
		
		const XMLObject::Child& resp = body.getChild("GetConfigurationOptionsResponse");
		if(!resp)	return false;
		
		const XMLObject::Child& ptzopt = resp.getChild("PTZConfigurationOptions");
		if (!ptzopt)	return false;

		const XMLObject::Child& space = ptzopt.getChild("Spaces");
		if (!space)	return false;


		const XMLObject::Child& abspan = space.getChild("AbsolutePanTiltPositionSpace");
		if (abspan)
		{
			const XMLObject::Child& xrange = abspan.getChild("XRange");
			if (xrange)
			{
				const XMLObject::Child& pmin = xrange.getChild("Min");
				if (pmin)  options->absolute_pantilt_x.min = pmin.data().readFloat();

				const XMLObject::Child& pmax = xrange.getChild("Max");
				if (pmax)  options->absolute_pantilt_x.max = pmax.data().readFloat();
			}

			const XMLObject::Child& yrange = abspan.getChild("YRange");
			if (yrange)
			{
				const XMLObject::Child& pmin = yrange.getChild("Min");
				if (pmin)  options->absolute_pantilt_y.min = pmin.data().readFloat();

				const XMLObject::Child& pmax = yrange.getChild("Max");
				if (pmax)  options->absolute_pantilt_y.max = pmax.data().readFloat();
			}
		}

		const XMLObject::Child& abszoom = space.getChild("AbsoluteZoomPositionSpace");
		if (abszoom)
		{
			const XMLObject::Child& xrange = abszoom.getChild("XRange");
			if (xrange)
			{
				const XMLObject::Child& pmin = xrange.getChild("Min");
				if (pmin)  options->absolute_zoom.min = pmin.data().readFloat();

				const XMLObject::Child& pmax = xrange.getChild("Max");
				if (pmax)  options->absolute_zoom.max = pmax.data().readFloat();
			}
		}

		const XMLObject::Child& pantilt = space.getChild("RelativePanTiltTranslationSpace");
		if (pantilt)
		{
			const XMLObject::Child& xrange = pantilt.getChild("XRange");
			if (xrange)
			{
				const XMLObject::Child& pmin = xrange.getChild("Min");
				if (pmin)  options->relative_pantilt_x.min = pmin.data().readFloat();

				const XMLObject::Child& pmax = xrange.getChild("Max");
				if (pmax)  options->relative_pantilt_x.max = pmax.data().readFloat();
			}

			const XMLObject::Child& yrange = pantilt.getChild("YRange");
			if (yrange)
			{
				const XMLObject::Child& pmin = yrange.getChild("Min");
				if (pmin)  options->relative_pantilt_y.min = pmin.data().readFloat();

				const XMLObject::Child& pmax = yrange.getChild("Max");
				if (pmax)  options->relative_pantilt_y.max = pmax.data().readFloat();
			}
		}

		const XMLObject::Child& panzoom = space.getChild("RelativeZoomTranslationSpace");
		if (panzoom)
		{
			const XMLObject::Child& xrange = panzoom.getChild("XRange");
			if (xrange)
			{
				const XMLObject::Child& pmin = xrange.getChild("Min");
				if (pmin)  options->relative_zoom.min = pmin.data().readFloat();

				const XMLObject::Child& pmax = xrange.getChild("Max");
				if (pmax)  options->relative_zoom.max = pmax.data().readFloat();
			}
		}

		const XMLObject::Child& continuous = space.getChild("ContinuousPanTiltVelocitySpace");
		if (continuous)
		{
			const XMLObject::Child& xrange = continuous.getChild("XRange");
			if (xrange)
			{
				const XMLObject::Child& pmin = xrange.getChild("Min");
				if (pmin)  options->continuous_pantilt_x.min = pmin.data().readFloat();

				const XMLObject::Child& pmax = xrange.getChild("Max");
				if (pmax)  options->continuous_pantilt_x.max = pmax.data().readFloat();
			}

			const XMLObject::Child& yrange = continuous.getChild("YRange");
			if (yrange)
			{
				const XMLObject::Child& pmin = yrange.getChild("Min");
				if (pmin)  options->continuous_pantilt_y.min = pmin.data().readFloat();

				const XMLObject::Child& pmax = yrange.getChild("Max");
				if (pmax)  options->continuous_pantilt_y.max = pmax.data().readFloat();
			}
		}

		const XMLObject::Child& panzoomzoom = space.getChild("ContinuousZoomVelocitySpace");
		if (panzoomzoom)
		{
			const XMLObject::Child& xrange = panzoomzoom.getChild("XRange");
			if (xrange)
			{
				const XMLObject::Child& pmin = xrange.getChild("Min");
				if (pmin)  options->continuous_zoom.min = pmin.data().readFloat();

				const XMLObject::Child& pmax = xrange.getChild("Max");
				if (pmax)  options->continuous_zoom.max = pmax.data().readFloat();
			}
		}

		const XMLObject::Child& speed = space.getChild("PanTiltSpeedSpace");
		if (speed)
		{
			const XMLObject::Child& xrange = speed.getChild("XRange");
			if (xrange)
			{
				const XMLObject::Child& pmin = xrange.getChild("Min");
				if (pmin)  options->pantilt_speed.min = pmin.data().readFloat();

				const XMLObject::Child& pmax = xrange.getChild("Max");
				if (pmax)  options->pantilt_speed.max = pmax.data().readFloat();
			}
		}

		const XMLObject::Child& zoomspeed = space.getChild("ZoomSpeedSpace");
		if (zoomspeed)
		{
			const XMLObject::Child& xrange = zoomspeed.getChild("XRange");
			if (xrange)
			{
				const XMLObject::Child& pmin = xrange.getChild("Min");
				if (pmin)  options->zoom_speed.min = pmin.data().readFloat();

				const XMLObject::Child& pmax = xrange.getChild("Max");
				if (pmax)  options->zoom_speed.max = pmax.data().readFloat();
			}
		}

		const XMLObject::Child& timeout = ptzopt.getChild("PTZTimeout");
		if (timeout)
		{
			const XMLObject::Child& pmin = timeout.getChild("Min");
			if (pmin)  options->timeout.min = (float)pmin.data().readInt();

			const XMLObject::Child& pmax = timeout.getChild("Max");
			if (pmax)  options->timeout.max = (float)pmax.data().readInt();
		}

		options->used = 1;

		return true;
	}

private:
	std::string token;
};


#endif

#endif //__ONVIFPROTOCOL_H__
