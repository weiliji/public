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
		XML::Child& getdeviceinfo = body().addChild("GetDeviceInformation");
		getdeviceinfo.addAttribute("xmlns", "http://www.onvif.org/ver10/device/wsdl");

		return CmdObject::build(URL);
	}
	OnvifClientDefs::Info devinfo;

	virtual bool parse(const XML::Child& body)
	{
		const XML::Child& response = body.getChild("GetDeviceInformationResponse");
		if (response.isEmpty()) return false;

		const XML::Child& manu = response.getChild("Manufacturer");
		if (!manu.isEmpty()) devinfo.manufacturer = manu.data().readString();

		const XML::Child& model = response.getChild("Model");
		if (!model.isEmpty()) devinfo.model = model.data().readString();

		const XML::Child& fireware = response.getChild("FirmwareVersion");
		if (!fireware.isEmpty()) devinfo.firmwareVersion = fireware.data().readString();

		const XML::Child& sn = response.getChild("SerialNumber");
		if (!sn.isEmpty()) devinfo.serialNumber = sn.data().readString();

		const XML::Child& hardware = response.getChild("HardwareId");
		if (!hardware.isEmpty()) devinfo.hardwareId = hardware.data().readString();

		return true;
	}
};

#endif //__ONVIFPROTOCOL_H__
