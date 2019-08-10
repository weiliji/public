#pragma once
#pragma once
#include "Defs.h"
#include "Base/Base.h"
#include "RTSPStructs.h"
using namespace Public::Base;

namespace Public {
namespace RTSP {

class RTSP_API RTPPackage
{
public:
	RTPPackage();
	RTPPackage(const String& data,uint32_t offset,uint32_t len);
	RTPPackage(const RTPPackage& package);
	~RTPPackage();

	//RTP头
	const RTPHEADER* header() const;
	//RTP数据地址，不包含头数据
	const char* data() const;
	//RTP数据长度，不包含头数据
	uint32_t datalen() const;

	const char* buffer() const;
	uint32_t bufferlen() const;
private:
	struct RTPPackageInternal;
	RTPPackageInternal* internal;
};

}
}