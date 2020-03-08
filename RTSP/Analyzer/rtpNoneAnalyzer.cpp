#include "rtpNoneAnalyzer.h"


RtpNoneAnalyzer::RtpNoneAnalyzer(const CBFreamCallBack& callback, FrameType type, CodeID code)
{
	m_pFramCallBack = callback;

	m_codeid = code;
	m_nFreamType			= type;

	m_nLastSeq = 0;
	m_lossstatus = false;
}

RtpNoneAnalyzer::~RtpNoneAnalyzer(void)
{
}

int RtpNoneAnalyzer::InputData(const shared_ptr<STREAM_TRANS_INFO>& transinfo, const shared_ptr<RTPPackage>& rtp)
{
	const RTPHEADER& m_stRtpHeader = rtp->rtpHeader();
	const char* pBuf = rtp->rtpDataAddr();
	uint32_t nBufLen = rtp->rtpDataLen();
	uint32_t extBufLen = rtp->rtpExternLen();

	//遇到mark将丢包状态修改，下一个包开始组
	if (m_lossstatus && m_stRtpHeader.m)
	{
		m_lossstatus = false;
		return 0;
	}

	//数据丢包
	if (m_nLastSeq != 0 && (uint16_t)(m_nLastSeq + 1) != ntohs(m_stRtpHeader.seq))
	{
		//assert(0);

		m_nLastSeq = 0;
		m_lossstatus = true;
		m_frame = NULL;

		return 0;
	}

	if (m_frame == NULL)
	{
		m_frame = make_shared<RTPFrame>();
	}
	if (extBufLen > 0)
	{
		m_frame->extendData(String((const char*)pBuf, extBufLen));

		pBuf += extBufLen;
		nBufLen -= extBufLen;
	}

	m_frame->pushRTPPackage(rtp);

	m_nLastSeq = ntohs(m_stRtpHeader.seq);
	
	if (nBufLen > 0)
	{
		m_frame->pushRTPBuffer(pBuf, nBufLen);
	}

	if (m_stRtpHeader.m)
	{
		m_frame->codeId(m_codeid);
		m_frame->frameType(m_nFreamType);
		m_frame->timestmap(ntohl(m_stRtpHeader.ts));

		m_pFramCallBack(m_frame);

		m_frame = NULL;
	}

	return 0;
}
