#pragma once

#include "irtpAnalyzer.h"

#define JPEG_HEADER_LEN		8

class RtpNoneAnalyzer : public IRtpAnalyzer
{
public:
	RtpNoneAnalyzer(const CBFreamCallBack& callback, FrameType type, CodeID code);
	~RtpNoneAnalyzer(void);

	int InputData(const shared_ptr<STREAM_TRANS_INFO>& transinfo, const shared_ptr<RTPPackage>& rtp);
private:
	shared_ptr<RTPFrame>	m_frame;

	CBFreamCallBack m_pFramCallBack;

//	String			m_pVideoBuf;
//	int				m_nVideoBufLen;
	
	CodeID			m_codeid;
	FrameType		m_nFreamType;
	uint16_t		m_nLastSeq;
	bool			m_lossstatus;
};
