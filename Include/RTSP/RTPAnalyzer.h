#pragma once

#include "RTSP/Defs.h"
#include "RTSP/RTSPStructs.h"
#include "Base/Base.h"
#include "RTSP/RTPFrame.h"
#include "RTSP/RTSPStructs.h"
using namespace Public::Base;


namespace Public {
namespace RTSP {

//将RTP包数据进行分解为相应的流数据
class RTSP_API RTPAnalyzer
{
public:
	//数据回调
//const unsigned char *pBuf 帧数据
//int nFreamSize			帧长度
//int nFreamType			帧类型
//long lTimestamp			时间戳,如果是分包以第一个RTP包时间戳为准
	typedef Function<void(const shared_ptr<RTPFrame>&)> FrameCallback;
public:
	RTPAnalyzer(const FrameCallback& framcallback);
	~RTPAnalyzer();

	///输入RTP包
	///@[in] pBuf RTP 包数据
	///@[in] nBufSize RTP 包长度
	///@[in] nOffset 传入buffer的偏移量
	///@[return] 0:传入数据不够一帧，继续等待数据, -1:出现错误。
	bool inputRtpPacket(const shared_ptr<STREAM_TRANS_INFO>& transportinfo, const shared_ptr<RTPPackage>& rtppackage);

	///清空缓存区
	bool reset();
private:
	struct RTPAnalyzerInternal;
	RTPAnalyzerInternal* internal;

};

//将完整的音视频数据包打包成RTP包
class RTSP_API RTPBuilder
{
public:
	RTPBuilder(const shared_ptr<STREAM_TRANS_INFO>& rttransinfo);

	~RTPBuilder();

	//输入数据必须为一个完整帧
	std::vector<shared_ptr<RTPPackage>> inputFrame(const shared_ptr<RTPFrame>& frame);

	void setAutoTimeStampEnable(bool enable);
private:
	struct RTPBuilderInternal;
	RTPBuilderInternal* internal;
};

}
}
