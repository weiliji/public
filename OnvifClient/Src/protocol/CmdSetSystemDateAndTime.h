#ifndef __ONVIFPROTOCOL_CmdGetStreamURL_H__SetSystemDateAndTime
#define __ONVIFPROTOCOL_CmdGetStreamURL_H__SetSystemDateAndTime
#include "CmdObject.h"


class CmdSetSystemDateAndTime :public CmdObject
{
public:
	CmdSetSystemDateAndTime(const Time& _time) :CmdObject(URL_ONVIF_DEVICE_SERVICE),time(_time)
	{
	}
	virtual ~CmdSetSystemDateAndTime() {}

	virtual std::string build(const URL& URL)
	{
		XMLObject::Child& setsystemdatetime = body().addChild("SetSystemDateAndTime");

		setsystemdatetime.attribute("xmlns", "http://www.onvif.org/ver10/device/wsdl");

		setsystemdatetime.addChild("DateTimeType","Manual");
		setsystemdatetime.addChild("DaylightSavings", "false");

		XMLObject::Child& utcdatetime = setsystemdatetime.addChild("UTCDateTime");
		
		XMLObject::Child& timev = utcdatetime.addChild("Time");
		timev.attribute("xmlns","http://www.onvif.org/ver10/schema");
		timev.addChild("Hour",time.hour);
		timev.addChild("Minute", time.minute);
		timev.addChild("Second", time.second);

		XMLObject::Child& datev = utcdatetime.addChild("Date");
		datev.attribute("xmlns", "http://www.onvif.org/ver10/schema");
		datev.addChild("Year", time.year);
		datev.addChild("Month", time.month);
		datev.addChild("Day", time.day);


		return CmdObject::build(URL);
	}
	virtual bool parse(const XMLObject::Child& body) { return true; }
private:
	Time time;
};



#endif //__ONVIFPROTOCOL_H__
