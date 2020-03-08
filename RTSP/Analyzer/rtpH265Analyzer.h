#pragma once

#include "irtpAnalyzer.h"
#include "h265frame.h"
#include "Base/Base.h"

class RtpH265Analyzer : public IRtpAnalyzer
{
public:
	RtpH265Analyzer(const CBFreamCallBack& callback);
	~RtpH265Analyzer(void);

	//virtual
	virtual int InputData(const shared_ptr<STREAM_TRANS_INFO>& transinfo, const shared_ptr<RTPPackage>& rtp);
private:
	shared_ptr<H265Frame> pHevcFrame;
	CBFreamCallBack		 m_pFramCallBack;
};
