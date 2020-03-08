#include "RTSP/RTPPackage.h"

namespace Public {
namespace RTSP {


RTPPackage::RTPPackage()
{
}
RTPPackage::RTPPackage(const String& _data, uint32_t _pos, uint32_t _len)
{
	data = _data;
	offset = _pos;
	len = _len;
}
RTPPackage::RTPPackage(const RTPPackage& package)
{
	data = package.data;
	offset = package.offset;
	len = package.len;
}
RTPPackage::~RTPPackage()
{
}


const RTPHEADER& RTPPackage::rtpHeader() const
{
	static RTPHEADER defaultheader;

	const char* buffer = data.c_str();
	uint32_t bufferlen = len;

	if (buffer == NULL || bufferlen < sizeof(RTPHEADER))
	{
		return defaultheader;
	}

	return *(RTPHEADER*)(buffer + offset);
}
const char* RTPPackage::rtpDataAddr() const
{
	const char* buffer = data.c_str();
	uint32_t bufferlen = len;

	if (buffer == NULL || bufferlen < sizeof(RTPHEADER))
	{
		return NULL;
	}

	return buffer + offset + sizeof(RTPHEADER);
}
uint32_t RTPPackage::rtpDataLen() const
{
	const char* buffer = data.c_str();
	uint32_t bufferlen = len;

	if (buffer == NULL || bufferlen < sizeof(RTPHEADER))
	{
		return 0;
	}

	return bufferlen - sizeof(RTPHEADER);
}
uint32_t RTPPackage::rtpExternLen() const
{
	const RTPHEADER& header = rtpHeader();
	if (header.x == 0 || sizeof(RTPEXTERNHEADER) > rtpDataLen()) return 0;

	RTPEXTERNHEADER* extheader = (RTPEXTERNHEADER*)rtpDataAddr();

	//扩展头为4的倍数
	return ntohs(extheader->len) * 4;
}
const char* RTPPackage::buffer() const
{
	const char* buffer = data.c_str();
	uint32_t bufferlen = len;

	if (buffer == NULL || bufferlen < sizeof(RTPHEADER))
	{
		return NULL;
	}

	return (buffer + offset);
}
uint32_t RTPPackage::bufferlen() const
{
	const char* buffer = data.c_str();
	uint32_t bufferlen = len;

	if (buffer == NULL || bufferlen < sizeof(RTPHEADER))
	{
		return 0;
	}

	return bufferlen;
}
RTPPackage& RTPPackage::operator = (const RTPPackage& package)
{
	data = package.data;
	offset = package.offset;
	len = package.len;

	return *this;
}

}
}