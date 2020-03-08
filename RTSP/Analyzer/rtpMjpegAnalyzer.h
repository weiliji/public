#pragma once

#include "irtpAnalyzer.h"

class rtpMjpegAnalyzer : public IRtpAnalyzer
{
public:
    rtpMjpegAnalyzer(const CBFreamCallBack& callback);
	~rtpMjpegAnalyzer(void);

	int InputData(const shared_ptr<STREAM_TRANS_INFO>& transinfo, const shared_ptr<RTPPackage>& rtp);
	bool processSpecialHeader(shared_ptr<RTPFrame> &rtpframe, const std::shared_ptr<RTPPackage> &pkt);
private:
	shared_ptr<RTPFrame>	m_frame;

	CBFreamCallBack m_pFramCallBack;

	uint16_t		m_nLastSeq;
	bool			m_lossstatus;
};
