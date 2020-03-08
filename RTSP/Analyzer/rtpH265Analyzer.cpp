#include "rtpH265Analyzer.h"
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#ifdef WIN32
#include <WinSock.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#endif

RtpH265Analyzer::RtpH265Analyzer(const CBFreamCallBack& callback)
	: m_pFramCallBack(callback)
{
	pHevcFrame = make_shared<H265Frame>();
	m_pFramCallBack = callback;
}

RtpH265Analyzer::~RtpH265Analyzer()
{

}

int RtpH265Analyzer::InputData(const shared_ptr<STREAM_TRANS_INFO>& transinfo, const shared_ptr<RTPPackage>& rtp)
{
	int iGoodFrame = pHevcFrame->handleHevcFrame(transinfo,rtp);
	if (iGoodFrame <= 0)
	{
		return 0;
	}

	shared_ptr<RTPFrame> frame = pHevcFrame->m_frame;
	frame->codeId(CodeID_Video_H265);
	frame->timestmap(ntohl(rtp->rtpHeader().ts));

	pHevcFrame->resetBuffer();

	m_pFramCallBack(frame);
	
	return 0;
}
