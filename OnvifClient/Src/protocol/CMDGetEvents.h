#ifndef __ONVIFPROTOCOL_PROFILES_H__GetAlarm
#define __ONVIFPROTOCOL_PROFILES_H__GetAlarm
#include "CmdObject.h"


class CMDGetEvents :public CmdObject
{
public:
	CMDGetEvents():CmdObject(URL_ONVIF_EVENTS)
	{
	}
	virtual ~CMDGetEvents() {}

	virtual std::string build(const URL& URL)
	{
		header().action = "http://www.onvif.org/ver10/events/wsdl/PullPointSubscription/PullMessagesRequest";
		header().messageID = "urn:uuid:" + Guid::createGuid().getStringStream();
		header().replyTo = "http://schemas.xmlsoap.org/ws/2004/08/addressing/role/anonymous";
		header().to = "http://"+URL.getHost()+URL.getPath();

		XML::Child& pullmessage = body().addChild("PullMessages");

		pullmessage.addAttribute("xmlns","http://www.onvif.org/ver10/events/wsdl");

		pullmessage.addChild("Timeout","PT1M");
		pullmessage.addChild("MessageLimit", Value(1024));

		return CmdObject::build(URL);
	}

	OnvifClientDefs::EventInfos eventinfos;
	virtual bool parse(const XML::Child& body)
	{
		XML::Child response = body.getChild("PullMessagesResponse");
		if (response.isEmpty()) return false;

		for(XML::ChildIterator iter = response.child("NotificationMessage");iter;iter++)
		{
			const XML::Child& messageval = *iter;
			do 
			{
				OnvifClientDefs::EventInfos::EventInfo event;

				const XML::Child& topicval = messageval.getChild("Topic");
				if (topicval.isEmpty()) break;

				event.topic = topicval.data().readString();

				XML::Child msgval = messageval.getChild("Message").getChild("Message");
				event.arrivalTime = onvif_parse_datetime(msgval.getAttribute("UtcTime").readString());
				event.operation = msgval.getAttribute("PropertyOperation").readString();

				XML::Child sourceitem = msgval.getChild("Source");
				for(XML::ChildIterator iter = sourceitem.child("SimpleItem");iter;iter++)
				{
					const XML::Child &sourceval = *iter;
					std::string name = sourceval.getAttribute("Name").readString();
					std::string value = sourceval.getAttribute("Value").readString();

					if (name.length() == 0) continue;

					event.sources[name] = value;
				}

				XML::Child dataitem = msgval.getChild("Data");
				for(XML::ChildIterator iter = dataitem.child("SimpleItem");iter;iter++)
				{
					const XML::Child &dataval = *iter;
					std::string name = dataval.getAttribute("Name").readString();
					std::string value = dataval.getAttribute("Value").readString();

					if (name.length() == 0) continue;

					event.datas[name] = value;
				}


				eventinfos.eventInfos.push_back(event);

			} while (0);
		}

		return true;
	}
};



#endif //__ONVIFPROTOCOL_H__
