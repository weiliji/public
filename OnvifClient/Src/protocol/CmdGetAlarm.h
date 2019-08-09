#ifndef __ONVIFPROTOCOL_PROFILES_H__GetAlarm
#define __ONVIFPROTOCOL_PROFILES_H__GetAlarm
#include "CmdObject.h"


class CMDGetAlarm :public CmdObject
{
public:
	CMDGetAlarm()
	{
		action = "http://www.onvif.org/ver10/events/wsdl/PullPointSubscription/PullMessagesRequest";
	}
	virtual ~CMDGetAlarm() {}

	virtual std::string build(const URL& URL)
	{
		stringstream stream;

		stream << "<s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\" xmlns:a=\"http://www.w3.org/2005/08/addressing\">"
			<< buildAlarmHeader(URL,false)
			<< "<s:Body xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">"
			<< "<PullMessages xmlns=\"http://www.onvif.org/ver10/events/wsdl\">"
			<< "<Timeout>PT1M</Timeout>"
			<< "<MessageLimit>1024</MessageLimit>"
			<< "</PullMessages>"
			<< "</s:Body></s:Envelope>";

		return stream.str();
	}

	shared_ptr<OnvifClientDefs::RecvAlarmInfo> alarminfo;
	virtual bool parse(const XMLObject::Child& body)
	{
		alarminfo = make_shared<OnvifClientDefs::RecvAlarmInfo>();

		XMLObject::Child response = body.getChild("PullMessagesResponse");
		if (response.isEmpty()) return false;

		XMLObject::Child& messageval = response.firstChild("NotificationMessage");
		while (!messageval.isEmpty())
		{
			do 
			{
				OnvifClientDefs::AlarmInfo alarm;

				const XMLObject::Child& topicval = messageval.getChild("Topic");
				if (topicval.isEmpty()) break;

				alarm.topic = topicval.data();

				XMLObject::Child msgval = messageval.getChild("Message").getChild("Message");
				alarm.arrivalTime = onvif_parse_datetime(msgval.attribute("UtcTime"));
				alarm.operation = msgval.attribute("PropertyOperation");

				XMLObject::Child sourceitem = msgval.getChild("Source");
				XMLObject::Child &sourceval = sourceitem.firstChild("SimpleItem");
				while (!sourceval.isEmpty())
				{
					std::string name = sourceval.attribute("Name");
					std::string value = sourceval.attribute("Value");

					if (name.length() == 0) continue;

					alarm.sources[name] = value;

					sourceval = sourceitem.nextChild();
				}

				XMLObject::Child dataitem = msgval.getChild("Data");
				XMLObject::Child &dataval = dataitem.firstChild("SimpleItem");
				while (!dataval.isEmpty())
				{
					std::string name = dataval.attribute("Name");
					std::string value = dataval.attribute("Value");

					if (name.length() == 0) continue;

					alarm.datas[name] = value;

					dataval = dataitem.nextChild();
				}


				alarminfo->alarminfos.push_back(alarm);

			} while (0);			

			messageval = response.nextChild();
		}

		return true;
	}
};



#endif //__ONVIFPROTOCOL_H__
