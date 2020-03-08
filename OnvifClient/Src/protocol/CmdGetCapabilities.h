#ifndef __ONVIFPROTOCOLCAPABITIES_H__
#define __ONVIFPROTOCOLCAPABITIES_H__
#include "CmdObject.h"


class CMDGetCapabilities :public CmdObject
{
public:
	enum CapabilitiesType
	{
		CAP_ALL = 0,
		CAP_MEDIA = 1,
		CAP_DEVICE = 2,
		CAP_ANALYTICS = 3,
		CAP_EVENTS = 4,
		CAP_IMAGING = 5,
		CAP_PTZ = 6,
	};
public:
	CMDGetCapabilities(CapabilitiesType _cap = CAP_ALL) :CmdObject(URL_ONVIF_DEVICE_SERVICE),cap(_cap)
	{
	}
	virtual ~CMDGetCapabilities() {}

	virtual std::string build(const URL& URL)
	{
		XML::Child& getcapabilities = body().addChild("GetCapabilities");

		getcapabilities.addAttribute("xmlns","http://www.onvif.org/ver10/device/wsdl");

		//add category
		getcapabilities.addChild("Category",get_cap_str());


		return CmdObject::build(URL);
	}
	OnvifClientDefs::Capabilities capabilities;
	virtual bool parse(const XML::Child& p_xml)
	{
		const XML::Child& resp = p_xml.getChild("GetCapabilitiesResponse");
		if (!resp) return false;

		const XML::Child& cap = resp.getChild("Capabilities");
		if (!cap) return false;

		const XML::Child& media = cap.getChild("Media");
		if(media) capabilities.media.support = parseMedia(media);
		
		const XML::Child& ptz = cap.getChild("PTZ");
		if(ptz) capabilities.ptz.support = parsePtz(ptz);
		
		const XML::Child& events = cap.getChild("Events");
		if (events) capabilities.events.support = parseEvents(events);

		const XML::Child& device = cap.getChild("Device");
		if (device) parseDevice(device);

		return true;
	}
private:
	virtual bool parseMedia(const XML::Child& body)
	{
		const XML::Child& xaddr = body.getChild("XAddr");
		if (xaddr)
		{
			capabilities.media.xaddr = xaddr.data();
		}
		else
		{
			return false;
		}

		const XML::Child& cap = body.getChild("StreamingCapabilities");
		if (!cap) return false;
		
		const XML::Child& rtpmult = cap.getChild("RTPMulticast");
		if (rtpmult)
		{
			capabilities.media.rtpMulticast = rtpmult.data().readBool();
		}
		const XML::Child& rtptcp = cap.getChild("RTP_TCP");
		if (rtptcp)
		{
			capabilities.media.rtp_tcp = rtptcp.data().readBool();
		}
		const XML::Child& rtsp = cap.getChild("RTP_RTSP_TCP");
		if (rtsp)
		{
			capabilities.media.rtp_rtsp_tcp = rtsp.data().readBool();
		}

		return true;
	}


	bool parsePtz(const XML::Child& ptz)
	{
		const XML::Child& xaddr = ptz.getChild("XAddr");
		if (!xaddr) return false;

		capabilities.ptz.xaddr = xaddr.data();

		return true;
	}

	bool parseEvents(const XML::Child& p_events)
	{
		const XML::Child& xaddr = p_events.getChild("XAddr");
		if (!xaddr) return false;

		capabilities.events.xaddr = xaddr.data();

		return true;
	}

	bool parseDevice(const XML::Child& p_device)
	{
		XML::Child io = p_device.getChild("IO");
		if (!io) return false;


		for(XML::ChildIterator iter = io.child("InputConnectors");iter;iter++)
		{
			capabilities.device.io.alarminput.push_back(iter->data());
		}

		for(XML::ChildIterator iter = io.child("RelayOutputs");iter;iter++)
		{
			capabilities.device.io.alarmoutput.push_back(iter->data());
		}

		return true;
	}
private:
	std::string get_cap_str()
	{
		switch (cap)
		{
		case CAP_ALL:
			return "All";
			break;

		case CAP_ANALYTICS:
			return "Analytics";
			break;

		case CAP_DEVICE:
			return "Device";
			break;

		case CAP_EVENTS:
			return "Events";
			break;

		case CAP_IMAGING:
			return "Imaging";
			break;

		case CAP_MEDIA:
			return "Media";
			break;

		case CAP_PTZ:
			return "PTZ";
			break;
		}

		return "";
	}
private:
	CapabilitiesType cap;
};


#endif //__ONVIFPROTOCOL_H__
