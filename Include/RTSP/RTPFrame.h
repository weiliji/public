#pragma once

#include "RTSP/Defs.h"
#include "RTSP/RTSPStructs.h"
#include "RTSP/RTPPackage.h"
#include "Base/Base.h"
using namespace Public::Base;


namespace Public {
namespace RTSP {

class RTPPackage;

//该对象零拷贝
class RTSP_API RTPFrame
{
public:
	struct FrameBuf
	{
		const char* buffer;
		uint32_t	size;
	};
public:
	RTPFrame();
	RTPFrame(const std::string& buffer);
	RTPFrame(const void* buffer, uint32_t len);
	RTPFrame(const RTPFrame& frame);

	virtual ~RTPFrame();

	void codeId(CodeID id);
	CodeID codeId()const;

	void frameType(FrameType type);
	FrameType frameType() const;

	//ms
	void timestmap(uint32_t ts);
	uint32_t timestmap() const;

	uint32_t frameSize() const;

	std::vector<FrameBuf> buffer() const;

	String framedata() const;

	void extendData(const String& data);
	String extendData() const;

	//获取其中的rtp包
	//toRtpPackage 该接口只能 RTPAnalyzer 分析出来的支持零拷贝
	std::vector<shared_ptr<RTPPackage>> toRtpPackage() const;

	void pushRTPPackage(const shared_ptr<RTPPackage>& rtp);
	void pushBuffer(const std::string& buffer);
	void pushBuffer(const void* buffer, uint32_t len);
	void pushRTPBuffer(const void* buffer, uint32_t len);

	uint32_t copyTo(void* buffer, uint32_t buflen);
private:
	struct RTPFrameInternal;
	RTPFrameInternal* internal;
};

}
}
