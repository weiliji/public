#ifndef __ONVIFPROTOCOL_DEVICEINFORMATSION_H__
#define __ONVIFPROTOCOL_DEVICEINFORMATSION_H__
#include "CmdObject.h"

class CMDGetDeviceInformation :public CmdObject
{
public:
	CMDGetDeviceInformation():CmdObject(URL_ONVIF_DEVICE_SERVICE)
	{
	}
	virtual ~CMDGetDeviceInformation() {}

	virtual std::string build(const URL& URL)
	{
		XMLObject::Child& getdeviceinfo = body().addChild("GetDeviceInformation");
		getdeviceinfo.attribute("xmlns", "http://www.onvif.org/ver10/device/wsdl");

		return CmdObject::build(URL);
	}
	OnvifClientDefs::Info devinfo;

	virtual bool parse(const XMLObject::Child& body)
	{
		const XMLObject::Child& response = body.getChild("GetDeviceInformationResponse");
		if (response.isEmpty()) return false;

		const XMLObject::Child& manu = response.getChild("Manufacturer");
		if (!manu.isEmpty()) devinfo.Manufacturer = manu.data().readString();

		const XMLObject::Child& model = response.getChild("Model");
		if (!model.isEmpty()) devinfo.Model = model.data().readString();

		const XMLObject::Child& fireware = response.getChild("FirmwareVersion");
		if (!fireware.isEmpty()) devinfo.FirmwareVersion = fireware.data().readString();

		const XMLObject::Child& sn = response.getChild("SerialNumber");
		if (!sn.isEmpty()) devinfo.SerialNumber = sn.data().readString();

		const XMLObject::Child& hardware = response.getChild("HardwareId");
		if (!hardware.isEmpty()) devinfo.HardwareId = hardware.data().readString();

		return true;
	}
};

#endif //__ONVIFPROTOCOL_H__
