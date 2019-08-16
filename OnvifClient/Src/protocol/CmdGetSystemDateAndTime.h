#ifndef __ONVIFPROTOCOL_CmdGetStreamURL_H__GetSystemDateAndTime
#define __ONVIFPROTOCOL_CmdGetStreamURL_H__GetSystemDateAndTime
#include "CmdObject.h"


class CmdGetSystemDateAndTime :public CmdObject
{
public:
	CmdGetSystemDateAndTime()
	{
		action = "http://www.onvif.org/ver10/device/wsdl/GetSystemDateAndTime";
	}
	virtual ~CmdGetSystemDateAndTime() {}

	virtual std::string build(const URL& URL)
	{
		stringstream stream;

		stream << "<s:Envelope " << onvif_xml_ns << ">"
			<< buildHeader(URL)
			<< "<s:Body>"
			<< "<GetSystemDateAndTime xmlns=\"http://www.onvif.org/ver10/device/wsdl\" />"
			<< "</s:Body></s:Envelope>";

		return stream.str();
	}
	Time time;
	virtual bool parse(const XMLObject::Child& body)
	{
		time.breakTime(Time::getCurrentTime().makeTime());

		const XMLObject::Child & resp = body.getChild("GetSystemDateAndTimeResponse");
		if (!resp) return false;

		const XMLObject::Child& systemdate = resp.getChild("SystemDateAndTime");
		if (!systemdate) return false;

		const XMLObject::Child& utcdate = systemdate.getChild("UTCDateTime");
		if (!utcdate) return false;
		
		time.hour = utcdate.getChild("Time").getChild("Hour").data().readInt();
		time.minute = utcdate.getChild("Time").getChild("Minute").data().readInt();
		time.second = utcdate.getChild("Time").getChild("Second").data().readInt();

		time.year = utcdate.getChild("Date").getChild("Year").data().readInt();
		time.month = utcdate.getChild("Date").getChild("Month").data().readInt();
		time.day = utcdate.getChild("Date").getChild("Day").data().readInt();
		
		
		return true; 
	}
};



#endif //__ONVIFPROTOCOL_H__
