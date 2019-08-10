#include "RTSP/RTPAnalyzer.h"
#include "irtpAnalyzer.h"
#include "rtpH264Analyzer.h"
#include "rtpH265Analyzer.h"
#include "rtpNoneAnalyzer.h"
#include "spsParse.h"

using namespace  Public::RTSP;

struct RTPAnalyzer::RTPAnalyzerInternal
{
	shared_ptr<IRtpAnalyzer> analyze;
	FrameCallback			 framcallback;
	shared_ptr<STREAM_TRANS_INFO>	transportinfo;

	void createAnalyze()
	{
		if (transportinfo == NULL || !framcallback)
		{
			return;
		}

		if (strcasecmp(transportinfo->streaminfo.szMediaName.c_str(), "video") == 0)
		{
			if (String::indexOfByCase(transportinfo->streaminfo.szCodec, "h264") != (size_t)-1)
			{
				int nWidth = 0;
				int nHeight = 0;

				std::string spsbuffer, ppsbuffer;

				int frameRate = 0;

				//解析sps信息，获取视频尺寸
				DecodeSpsPpsInfo(transportinfo->streaminfo.szSpsPps, nWidth, nHeight, spsbuffer, ppsbuffer, frameRate);

				analyze = make_shared<RtpH264Analyzer>(framcallback, spsbuffer, ppsbuffer);
			}
			else if (String::indexOfByCase(transportinfo->streaminfo.szCodec, "h265") != (size_t)-1)
			{
				analyze = make_shared<RtpH265Analyzer>(framcallback);
			}
		}

		if (analyze == NULL)
		{
			FrameType type = FrameType_AUDIO_FRAME;
			if (strcasecmp(transportinfo->streaminfo.szMediaName.c_str(), "video") == 0) type = FrameType_VIDEO_FRAME;
			else if (strcasecmp(transportinfo->streaminfo.szMediaName.c_str(), "audio") == 0) type = FrameType_AUDIO_FRAME;
			else return;

			analyze = make_shared<RtpNoneAnalyzer>(framcallback, type);
		}
	}
};
RTPAnalyzer::RTPAnalyzer(const shared_ptr<STREAM_TRANS_INFO>& transportinfo, const FrameCallback& framcallback)
{
	internal = new RTPAnalyzerInternal;
	internal->framcallback = framcallback;

	internal->createAnalyze();
}
RTPAnalyzer::~RTPAnalyzer()
{
	SAFE_DELETE(internal);
}

bool RTPAnalyzer::inputRtpPacket(const RTPPackage& rtppackage)
{
	const RTPHEADER* rtpheader = rtppackage.header();
	const char* bufferaddr = rtppackage.data();
	uint32_t bufferlen = rtppackage.datalen();

	if (internal->analyze == NULL || rtpheader  == NULL||bufferaddr == NULL || bufferlen <= 0) return false;

	if (internal->analyze->InputData(*rtpheader, bufferaddr, bufferlen) != 0) return false;

	return true;
}
bool RTPAnalyzer::reset()
{
	internal->analyze = NULL;
	internal->createAnalyze();

	return true;
}

