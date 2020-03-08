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
		XML::Child& setsystemdatetime = body().addChild("SetSystemDateAndTime");

		setsystemdatetime.addAttribute("xmlns", "http://www.onvif.org/ver10/device/wsdl");

		setsystemdatetime.addChild("DateTimeType","Manual");
		setsystemdatetime.addChild("DaylightSavings", "false");

		XML::Child& utcdatetime = setsystemdatetime.addChild("UTCDateTime");
		
		XML::Child& timev = utcdatetime.addChild("Time");
		timev.addAttribute("xmlns","http://www.onvif.org/ver10/schema");
		timev.addChild("Hour",time.hour);
		timev.addChild("Minute", time.minute);
		timev.addChild("Second", time.second);

		XML::Child& datev = utcdatetime.addChild("Date");
		datev.addAttribute("xmlns", "http://www.onvif.org/ver10/schema");
		datev.addChild("Year", time.year);
		datev.addChild("Month", time.month);
		datev.addChild("Day", time.day);


		return CmdObject::build(URL);
	}
	virtual bool parse(const XML::Child& body) { return true; }
private:
	Time time;
};



#endif //__ONVIFPROTOCOL_H__
