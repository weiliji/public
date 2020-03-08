#include "RTSP/RTPFrame.h"
#include "RTSP//RTPPackage.h"

namespace Public {
namespace RTSP {

struct FrameBufferInfo :public RTPFrame::FrameBuf
{
	String			data;
};

struct RTPFrame::RTPFrameInternal
{
	std::vector<shared_ptr<RTPPackage>>			rtplist;

	std::vector<shared_ptr<FrameBufferInfo>>	buflist;
	String										externBuf;

	CodeID			codeid = CodeID_Unknown;
	FrameType		frametype = FrameType_Unknown;
	uint32_t		timestmap = 0;
};


RTPFrame::RTPFrame()
{
	internal = new RTPFrameInternal;
}
RTPFrame::RTPFrame(const std::string& buffer)
{
	internal = new RTPFrameInternal;

	pushBuffer(buffer);
}
RTPFrame::RTPFrame(const void* buffer, uint32_t len)
{
	internal = new RTPFrameInternal;

	pushBuffer(buffer, len);
}
RTPFrame::RTPFrame(const RTPFrame& frame)
{
	internal = new RTPFrameInternal;

	*internal = *frame.internal;
}

RTPFrame::~RTPFrame()
{
	SAFE_DELETE(internal);
}

void RTPFrame::codeId(CodeID id) { internal->codeid = id; }
CodeID RTPFrame::codeId()const { return internal->codeid; }

void RTPFrame::frameType(FrameType type) { internal->frametype = type; }
FrameType RTPFrame::frameType() const 
{
	return internal->frametype; 
}

void RTPFrame::timestmap(uint32_t ts) { internal->timestmap = ts; }
uint32_t RTPFrame::timestmap() const { return internal->timestmap; }

uint32_t RTPFrame::frameSize() const
{
	uint32_t framesize = 0;

	for (size_t i = 0; i < internal->buflist.size(); i++)
	{
		framesize += internal->buflist[i]->size;
	}

	return framesize;
}

std::vector<RTPFrame::FrameBuf> RTPFrame::buffer() const
{
	std::vector<RTPFrame::FrameBuf> buflist;

	for (size_t i = 0; i < internal->buflist.size(); i++)
	{
		RTPFrame::FrameBuf buf = *internal->buflist[i];

		buflist.push_back(buf);
	}

	return buflist;
}
void RTPFrame::extendData(const String& data)
{
	internal->externBuf = data;
}
String RTPFrame::extendData() const
{
	return internal->externBuf;
}

String RTPFrame::framedata() const
{
	String framebuffer;
	{
		uint32_t framesize = frameSize();

		char* buf = framebuffer.alloc(framesize);

		std::vector<RTPFrame::FrameBuf> bufs = buffer();
		for (size_t i = 0; i < bufs.size(); i++)
		{
			memcpy(buf, bufs[i].buffer, bufs[i].size);
			buf += bufs[i].size;
		}

		framebuffer.resize(framesize);
	}
	return framebuffer;
}
uint32_t RTPFrame::copyTo(void* bufaddr, uint32_t buflen)
{
	uint32_t havecopylen = 0;

	std::vector<RTPFrame::FrameBuf> bufs = buffer();
	for (size_t i = 0; i < bufs.size(); i++)
	{
		uint32_t cancopylen = min(buflen - havecopylen, bufs[i].size);
		if (cancopylen <= 0) break;

		memcpy((char*)bufaddr + havecopylen, bufs[i].buffer, cancopylen);

		havecopylen += cancopylen;
	}

	return havecopylen;
}

std::vector<shared_ptr<RTPPackage>> RTPFrame::toRtpPackage() const
{
	return internal->rtplist;
}
void RTPFrame::pushRTPPackage(const shared_ptr<RTPPackage>& rtp)
{
	internal->rtplist.push_back(rtp);
}
void RTPFrame::pushBuffer(const std::string& buffer)
{
	shared_ptr<FrameBufferInfo> info = make_shared<FrameBufferInfo>();
	info->data = buffer;
	info->buffer = info->data.c_str();
	info->size = (uint32_t)info->data.length();

	internal->buflist.push_back(info);
}
void RTPFrame::pushBuffer(const void* buffer, uint32_t len)
{
	shared_ptr<FrameBufferInfo> info = make_shared<FrameBufferInfo>();
	info->data = String((const char *)buffer, len);

	info->buffer = info->data.c_str();
	info->size = (uint32_t)info->data.length();

	internal->buflist.push_back(info);
}
void RTPFrame::pushRTPBuffer(const void* buffer, uint32_t len)
{
	if (len <= 0) return;

	shared_ptr<FrameBufferInfo> info = make_shared<FrameBufferInfo>();
	
	info->buffer = (const char*)buffer;
	info->size = len;

	internal->buflist.push_back(info);
}

}
}
