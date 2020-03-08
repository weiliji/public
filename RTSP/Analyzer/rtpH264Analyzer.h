#pragma once

#include "irtpAnalyzer.h"

class RtpH264Analyzer : public IRtpAnalyzer
{
public:
	RtpH264Analyzer(const CBFreamCallBack& callback,const std::string& pSps, const std::string& pPps);
	~RtpH264Analyzer(void);

	virtual int InputData(const shared_ptr<STREAM_TRANS_INFO>& transinfo, const shared_ptr<RTPPackage>& rtp);
private:
	shared_ptr<RTPFrame>	m_frame;



	FU_INDICATOR	m_stFuIndictor;
	FU_HEADER		m_stNalHeader;

	CBFreamCallBack m_pFramCallBack; 

	String			m_pSpsBuffer;
	String			m_pPpsBuffer;

//	String			m_pVideoBuf;
	
	int				m_nLastSeq;
	FrameType		m_nFreamType;
	int				m_bFirstSeq;

	bool			m_bThrowPack;
	bool			m_bIsRecvingIDRFream;
	bool			m_bIsWaitingIDRFream;
};
