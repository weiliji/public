#include "RTSP/RTPAnalyzer.h"
#include "irtpAnalyzer.h"
#include "rtpH264Analyzer.h"
#include "rtpH265Analyzer.h"
#include "rtpNoneAnalyzer.h"
#include "rtpMjpegAnalyzer.h"
#include "spsParse.h"

using namespace  Public::RTSP;

struct RTPAnalyzer::RTPAnalyzerInternal
{
	shared_ptr<IRtpAnalyzer>		videoanalyze;

	shared_ptr<IRtpAnalyzer>		audioanalyze;

	FrameCallback					framcallback;
	

	shared_ptr<IRtpAnalyzer> createAnalyze(const shared_ptr<STREAM_TRANS_INFO>& transportinfo)
	{
		shared_ptr<IRtpAnalyzer> analyze;

		if (transportinfo->streaminfo.frametype == FrameType_Video)
		{
			if (transportinfo->streaminfo.codeId == CodeID_Video_H264)
			{
				int nWidth = 0;
				int nHeight = 0;

				std::string spsbuffer, ppsbuffer;

				int frameRate = 0;

				//解析sps信息，获取视频尺寸
				DecodeSpsPpsInfo(transportinfo->streaminfo.sprop_parameter_sets, nWidth, nHeight, spsbuffer, ppsbuffer, frameRate);

				analyze = make_shared<RtpH264Analyzer>(framcallback, spsbuffer, ppsbuffer);
			}
			else if (transportinfo->streaminfo.codeId == CodeID_Video_H265)
			{
				analyze = make_shared<RtpH265Analyzer>(framcallback);
			}
            else if (transportinfo->streaminfo.codeId == CodeID_Video_JPEG)
            {
                analyze = make_shared<rtpMjpegAnalyzer>(framcallback);
            }
		}
		if (analyze == NULL)
		{
			analyze = make_shared<RtpNoneAnalyzer>(framcallback, transportinfo->streaminfo.frametype, transportinfo->streaminfo.codeId);
		}

		return analyze;
	}

	shared_ptr<IRtpAnalyzer> getAnalyzeAndCreate(const shared_ptr<STREAM_TRANS_INFO>& transportinfo)
	{
		if (transportinfo->streaminfo.frametype == FrameType_Video)
		{
			if (videoanalyze == NULL)	videoanalyze = createAnalyze(transportinfo);
			
			return videoanalyze;
		}
		else if (transportinfo->streaminfo.frametype == FrameType_Audio)
		{
			if (audioanalyze == NULL) audioanalyze = createAnalyze(transportinfo);
			
			return audioanalyze;
		}

		return shared_ptr<IRtpAnalyzer>();
	}
};
RTPAnalyzer::RTPAnalyzer(const FrameCallback& framcallback)
{
	internal = new RTPAnalyzerInternal;
	internal->framcallback = framcallback;
}
RTPAnalyzer::~RTPAnalyzer()
{
	SAFE_DELETE(internal);
}

bool RTPAnalyzer::inputRtpPacket(const shared_ptr<STREAM_TRANS_INFO>& transportinfo, const shared_ptr<RTPPackage>& rtppackage)
{
	shared_ptr<IRtpAnalyzer> analyzer = internal->getAnalyzeAndCreate(transportinfo);
	if (analyzer == NULL || rtppackage == NULL)
	{
		return false;
	}
	

	if (analyzer->InputData(transportinfo,rtppackage) != 0)
	{
		analyzer = NULL;
	}

	return true;
}
bool RTPAnalyzer::reset()
{
	internal->videoanalyze = NULL;
	internal->audioanalyze = NULL;

	return true;
}

