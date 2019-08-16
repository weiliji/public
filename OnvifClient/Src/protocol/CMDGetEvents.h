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

		XMLObject::Child& pullmessage = body().addChild("PullMessages");

		pullmessage.attribute("xmlns","http://www.onvif.org/ver10/events/wsdl");

		pullmessage.addChild("Timeout","PT1M");
		pullmessage.addChild("MessageLimit", Value(1024));

		return CmdObject::build(URL);
	}

	OnvifClientDefs::EventInfos eventinfos;
	virtual bool parse(const XMLObject::Child& body)
	{
		XMLObject::Child response = body.getChild("PullMessagesResponse");
		if (response.isEmpty()) return false;

		XMLObject::Child& messageval = response.firstChild("NotificationMessage");
		while (!messageval.isEmpty())
		{
			do 
			{
				OnvifClientDefs::EventInfos::EventInfo event;

				const XMLObject::Child& topicval = messageval.getChild("Topic");
				if (topicval.isEmpty()) break;

				event.topic = topicval.data();

				XMLObject::Child msgval = messageval.getChild("Message").getChild("Message");
				event.arrivalTime = onvif_parse_datetime(msgval.attribute("UtcTime"));
				event.operation = msgval.attribute("PropertyOperation");

				XMLObject::Child sourceitem = msgval.getChild("Source");
				XMLObject::Child &sourceval = sourceitem.firstChild("SimpleItem");
				while (!sourceval.isEmpty())
				{
					std::string name = sourceval.attribute("Name");
					std::string value = sourceval.attribute("Value");

					if (name.length() == 0) continue;

					event.sources[name] = value;

					sourceval = sourceitem.nextChild();
				}

				XMLObject::Child dataitem = msgval.getChild("Data");
				XMLObject::Child &dataval = dataitem.firstChild("SimpleItem");
				while (!dataval.isEmpty())
				{
					std::string name = dataval.attribute("Name");
					std::string value = dataval.attribute("Value");

					if (name.length() == 0) continue;

					event.datas[name] = value;

					dataval = dataitem.nextChild();
				}


				eventinfos.eventInfos.push_back(event);

			} while (0);			

			messageval = response.nextChild();
		}

		return true;
	}
};



#endif //__ONVIFPROTOCOL_H__
