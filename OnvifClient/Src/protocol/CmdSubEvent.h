#ifndef __ONVIFPROTOCOL_PROFILES_H__StartRecvAlarm
#define __ONVIFPROTOCOL_PROFILES_H__StartRecvAlarm
#include "CmdObject.h"


class CMDSubEvent :public CmdObject
{
public:
	CMDSubEvent()
	{
		action = "http://www.onvif.org/ver10/events/wsdl/EventPortType/CreatePullPointSubscriptionRequest";
		requesturl = EVENTSREQUESTURL;
	}
	virtual ~CMDSubEvent() {}

	virtual std::string build(const URL& URL)
	{
		stringstream stream;

		stream << "<s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\" xmlns:a=\"http://www.w3.org/2005/08/addressing\">"
			<< buildAlarmHeader(URL,true)
			<< "<s:Body xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">"
			<< "<CreatePullPointSubscription xmlns=\"http://www.onvif.org/ver10/events/wsdl\">"
			<< "<InitialTerminationTime>PT600S</InitialTerminationTime>"
			<< "</CreatePullPointSubscription>"
			<< "</s:Body></s:Envelope>";

		return stream.str();
	}

	OnvifClientDefs::SubEventResponse	subeventresp;
	virtual bool parse(const XMLObject::Child& body)
	{
		const XMLObject::Child& resp = body.getChild("CreatePullPointSubscriptionResponse");
		if (!resp) return false;

		subeventresp.xaddr = resp.getChild("SubscriptionReference").getChild("Address").data();
		subeventresp.currentTime = onvif_parse_datetime(resp.getChild("CurrentTime").data());
		subeventresp.terminationTime = onvif_parse_datetime(resp.getChild("TerminationTime").data());

		return true;
	}
};



#endif //__ONVIFPROTOCOL_H__
