#include "RTSP/RTPPackage.h"

namespace Public {
namespace RTSP {

struct RTPPackage::RTPPackageInternal
{
	String	data;
	uint32_t offset;
	uint32_t len;

	RTPPackageInternal():offset(0),len(0){}
};


RTPPackage::RTPPackage()
{
	internal = new RTPPackageInternal();
}
RTPPackage::RTPPackage(const String& data, uint32_t pos, uint32_t len)
{
	internal = new RTPPackageInternal();
	internal->data = data;
	internal->offset = pos;
	internal->len = len;
}
RTPPackage::RTPPackage(const RTPPackage& package)
{
	internal = new RTPPackageInternal();
	internal->data = package.internal->data;
	internal->offset = package.internal->offset;
	internal->len = package.internal->len;
}
RTPPackage::~RTPPackage()
{
	SAFE_DELETE(internal);
}


const RTPHEADER* RTPPackage::header() const
{
	const char* buffer = internal->data.c_str();
	uint32_t bufferlen = (uint32_t)internal->data.length();

	if (buffer == NULL || bufferlen < sizeof(RTPHEADER))
	{
		return NULL;
	}

	return (RTPHEADER*)(buffer + internal->offset);
}
const char* RTPPackage::data() const
{
	const char* buffer = internal->data.c_str();
	uint32_t bufferlen = (uint32_t)internal->data.length();

	if (buffer == NULL || bufferlen < sizeof(RTPHEADER))
	{
		return NULL;
	}

	return buffer + sizeof(RTPHEADER);
}
uint32_t RTPPackage::datalen() const
{
	const char* buffer = internal->data.c_str();
	uint32_t bufferlen = (uint32_t)internal->data.length();

	if (buffer == NULL || bufferlen < sizeof(RTPHEADER))
	{
		return 0;
	}

	return bufferlen - sizeof(RTPHEADER);
}
const char* RTPPackage::buffer() const
{
	const char* buffer = internal->data.c_str();
	uint32_t bufferlen = (uint32_t)internal->data.length();

	if (buffer == NULL || bufferlen < sizeof(RTPHEADER))
	{
		return NULL;
	}

	return (buffer + internal->offset);
}
uint32_t RTPPackage::bufferlen() const
{
	const char* buffer = internal->data.c_str();
	uint32_t bufferlen = (uint32_t)internal->data.length();

	if (buffer == NULL || bufferlen < sizeof(RTPHEADER))
	{
		return 0;
	}

	return bufferlen;
}

}
}