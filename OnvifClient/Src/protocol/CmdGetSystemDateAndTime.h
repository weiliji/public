#ifndef __ONVIFPROTOCOL_CmdGetStreamURL_H__GetSystemDateAndTime
#define __ONVIFPROTOCOL_CmdGetStreamURL_H__GetSystemDateAndTime
#include "CmdObject.h"


class CmdGetSystemDateAndTime :public CmdObject
{
public:
	CmdGetSystemDateAndTime():CmdObject(URL_ONVIF_DEVICE_SERVICE)
	{
	}
	virtual ~CmdGetSystemDateAndTime() {}

	virtual std::string build(const URL& URL)
	{
		XML::Child& getsystemdatetime = body().addChild("GetSystemDateAndTime");

		getsystemdatetime.addAttribute("xmlns", "http://www.onvif.org/ver10/device/wsdl");
		
		return CmdObject::build(URL);
	}
	Time time;
	virtual bool parse(const XML::Child& body)
	{
		const XML::Child & resp = body.getChild("GetSystemDateAndTimeResponse");
		if (!resp) return false;

		const XML::Child& systemdate = resp.getChild("SystemDateAndTime");
		if (!systemdate) return false;

		const XML::Child& utcdate = systemdate.getChild("UTCDateTime");
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
