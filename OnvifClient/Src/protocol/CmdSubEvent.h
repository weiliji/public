#ifndef __ONVIFPROTOCOL_PROFILES_H__StartRecvAlarm
#define __ONVIFPROTOCOL_PROFILES_H__StartRecvAlarm
#include "CmdObject.h"


class CMDSubEvent :public CmdObject
{
public:
	CMDSubEvent():CmdObject(URL_ONVIF_EVENTS)
	{
	}
	virtual ~CMDSubEvent() {}

	virtual std::string build(const URL& URL)
	{
		header().action = "http://www.onvif.org/ver10/events/wsdl/EventPortType/CreatePullPointSubscriptionRequest";
		header().messageID = "urn:uuid:" + Guid::createGuid().getStringStream();
		header().replyTo = "http://www.w3.org/2005/08/addressing/anonymous";
		header().to = "http://" + URL.getHost() + URL.getPath();

		XML::Child& createpullpoint = body().addChild("CreatePullPointSubscription");

		createpullpoint.addAttribute("xmlns", "http://www.onvif.org/ver10/events/wsdl");

		createpullpoint.addChild("InitialTerminationTime", "PT600S");

		return CmdObject::build(URL);
	}

	OnvifClientDefs::SubEventResponse	subeventresp;
	virtual bool parse(const XML::Child& body)
	{
		const XML::Child& resp = body.getChild("CreatePullPointSubscriptionResponse");
		if (!resp) return false;

		subeventresp.xaddr = resp.getChild("SubscriptionReference").getChild("Address").data();
		subeventresp.currentTime = onvif_parse_datetime(resp.getChild("CurrentTime").data());
		subeventresp.terminationTime = onvif_parse_datetime(resp.getChild("TerminationTime").data());

		return true;
	}
};



#endif //__ONVIFPROTOCOL_H__
