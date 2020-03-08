#include "rtpH264Analyzer.h"


#define H264_STATRTCODE_LEN 4
static const char H264_STATRTCODE_ADDR[4] = { 0x00,0x00,0x00,0x01 };

#define SAFE_COPY_VIDEO(videobuffer,src,len,maxlen) {\
	if(videobuffer.length() + (len) > (size_t)maxlen) return -1;\
	char* pBuftmp = (char*)videobuffer.c_str();\
	if(pBuftmp == NULL) assert(0);\
	memcpy(pBuftmp + videobuffer.length(), src, len);\
	videobuffer.resize(videobuffer.length() + (len));\
}

#define SAFE_SET_H264_STARTCODE(videobuffer,maxlen) {\
	if (videobuffer.length() + H264_STATRTCODE_LEN > (size_t)maxlen) return -1;\
	char* pBuftmp = (char*)videobuffer.c_str();\
	if(pBuftmp == NULL) assert(0);\
	memcpy(pBuftmp + videobuffer.length(),H264_STATRTCODE_ADDR,H264_STATRTCODE_LEN);\
	videobuffer.resize(videobuffer.length() + H264_STATRTCODE_LEN);\
}






RtpH264Analyzer::RtpH264Analyzer(const CBFreamCallBack& callback, const std::string& pSps, const std::string& pPps)
{
	m_pFramCallBack = callback;

	memset(&m_stFuIndictor, 0, sizeof(FU_INDICATOR));
	memset(&m_stNalHeader,	0, sizeof(FU_HEADER));

	m_nFreamType			= FrameType_Video;

	m_bFirstSeq				= -2;
	m_nLastSeq				= -2;

	m_bThrowPack			= false;
	m_bIsRecvingIDRFream	= false;
	m_bIsWaitingIDRFream	= false;
	
		
	m_pSpsBuffer			= pSps;
	m_pPpsBuffer			= pPps;
}

RtpH264Analyzer::~RtpH264Analyzer()
{

}

int RtpH264Analyzer::InputData(const shared_ptr<STREAM_TRANS_INFO>& transinfo, const shared_ptr<RTPPackage>& rtp)
{
	const RTPHEADER& m_stRtpHeader = rtp->rtpHeader();
	


	//检查是否有丢包
	int nCurrentSeq = ntohs(m_stRtpHeader.seq);

	//第一次来包的时候不进行判断丢包,此时的 m_nLastSeq 为初始状态,不算丢包
	if (m_nLastSeq + 1 < nCurrentSeq && m_nLastSeq != m_bFirstSeq)
	{
		m_bThrowPack   = true;
		if (true == m_bIsRecvingIDRFream)
		{
			m_bIsWaitingIDRFream = true;
		}
	}
	//65535表示seq循环的最后一个,下一个时为0,出现这种情况是正常的,不是序列混乱
	else if (m_nLastSeq + 1 > nCurrentSeq && m_nLastSeq != 65535)
	{
		//待处理
	}

	const char* pBuf = rtp->rtpDataAddr();
	uint32_t nBufLen = rtp->rtpDataLen();
	uint32_t extBufLen = rtp->rtpExternLen();

	if (m_frame == NULL) m_frame = make_shared<RTPFrame>();
	m_frame->pushRTPPackage(rtp);

	if (extBufLen > 0)
	{
		m_frame->extendData(String(pBuf, extBufLen));
	
		pBuf += extBufLen;
		nBufLen -= extBufLen;
	}
	m_nLastSeq = nCurrentSeq;
	if (nBufLen <= 0)
	{
		if (m_stRtpHeader.m && m_frame)
		{
			m_pFramCallBack(m_frame);

			m_frame = NULL;
		}

		return 0;
	}

	//get nalu header
	memcpy(&m_stFuIndictor, pBuf, 1);

	/*RFC3984 5.2  RTP荷载格式的公共结构
	  注释:本规范没有限制封装在单个NAL单元包和分片包的大小。封装在聚合包中的NAL单元大小为65535字节
	  单个NAL单元包: 荷载中只包含一个NAL单元。NAL头类型域等于原始NAL单元类型,即在范围1~23之间
	*/
	if ( m_stFuIndictor.TYPE >0 &&  m_stFuIndictor.TYPE < 24)
	{
		String videobuffer;
		if (m_stFuIndictor.TYPE != 5)
		{
			int nVideoBufferSize = H264_STATRTCODE_LEN + nBufLen;
			videobuffer.alloc(nVideoBufferSize + 100);

			//单个个NAL包数据,首先设置帧数据起始码
			SAFE_SET_H264_STARTCODE(videobuffer, nVideoBufferSize);

			SAFE_COPY_VIDEO(videobuffer, pBuf, nBufLen, nVideoBufferSize);
		}
		else
		{
			int nVideoBufferSize = H264_STATRTCODE_LEN + (int)m_pSpsBuffer.length() + (int)m_pPpsBuffer.length() + nBufLen;
			videobuffer.alloc(nVideoBufferSize + 100);

			SAFE_COPY_VIDEO(videobuffer, m_pSpsBuffer.c_str(), m_pSpsBuffer.length(), nVideoBufferSize);
			SAFE_COPY_VIDEO(videobuffer, m_pPpsBuffer.c_str(), m_pPpsBuffer.length(), nVideoBufferSize);

			SAFE_SET_H264_STARTCODE(videobuffer, nVideoBufferSize);

			SAFE_COPY_VIDEO(videobuffer, pBuf, nBufLen, nVideoBufferSize);
		}
		m_nFreamType = FrameType_Video_PFrame;
		switch (m_stFuIndictor.TYPE)
		{
		case 0://未定义
			break;
		case 1://非IDR帧图像不采用数据划分片段
			m_nFreamType = FrameType_Video_PFrame;
//			m_pFramCallBack(m_pVideoBuf, m_nVideoBufLen, m_nFreamType, ntohl(m_stRtpHeader.ts), (uint64_t)m_dwUser, m_lpUser);
			break;
		case 2://非IDR帧图像中A类数据划分片段
			break;
		case 3://非IDR帧图像中B类数据划分片段
			break;
		case 4://非IDR帧图像中C类数据划分片段
			break;
		case 5://IDR帧的图像片段
			m_nFreamType = FrameType_Video_IFrame;
//			m_pFramCallBack(m_pVideoBuf, m_nVideoBufLen, m_nFreamType, ntohl(m_stRtpHeader.ts), (uint64_t)m_dwUser, m_lpUser);
			break;
		case 6://补充增强信息(SEI)
			break;
		case 7://序列参数集(SPS)
			m_nFreamType = FrameType_Video_IFrame;
			m_pSpsBuffer = videobuffer;
//			m_pFramCallBack(m_pVideoBuf, m_nVideoBufLen, m_nFreamType, ntohl(m_stRtpHeader.ts), (uint64_t)m_dwUser, m_lpUser);
			break;
		case 8://图像参数集(PPS)
			m_nFreamType = FrameType_Video_IFrame;
			m_pPpsBuffer = videobuffer;
//			m_pFramCallBack(m_pVideoBuf, m_nVideoBufLen, m_nFreamType, ntohl(m_stRtpHeader.ts), (uint64_t)m_dwUser, m_lpUser);
			break;
		case 9://分隔符
			break;
		case 10://序列结束符
			break;
		case 11://流结束符
			break;
		case 12://填充数据
			break;
		case 13://序列参数集扩展
			break;
		case 14://保留
			break;
		case 15://保留
			break;
		case 16://保留
			break;
		case 17://保留
			break;
		case 18://保留
			break;
		case 19://未分割的辅助编码图像片段
			break;
		case 20://保留
			break;
		case 21://保留
			break;
		case 22://保留
			break;
		case 23://保留
			break;
		default://未定义
			break;
		}
		if (videobuffer.length() == 0)
		{
			return 0;
		}
		/*shared_ptr<RTPAnalyzer_FrameInfo> frame = make_shared<RTPAnalyzer_FrameInfo>();
		frame->codeid = CodeID_Video_H264;
		frame->frametype = m_nFreamType;
		frame->frame = videobuffer;
		frame->timestmap = ntohl(m_stRtpHeader.ts);

		m_pFramCallBack(frame);*/

		return 0;
	}
	/*RFC3984 5.2  RTP荷载格式的公共结构
	  聚合包: 本类型用于聚合多个NAL单元到单个RTP荷载中。本包有4种版本,单时间聚合包类型A(STAP-A)
	  单时间聚合包类型B(STAP-B),多时间聚合包类型(MTAP)16位位移(MTAP16), 多时间聚合包类型(MTAP)24位位移(MTAP24)
	  赋予 STAP-A, STAP-B, MTAP16, MTAP24的NAL单元类型号分别是24, 25, 26, 27。
	*/
	else if (24 == m_stFuIndictor.TYPE)//STAP-A
	{
	}
	else if (25 == m_stFuIndictor.TYPE)//STAP-B
	{
	}
	else if (26 == m_stFuIndictor.TYPE)//MTAP16
	{
	}
	else if (27 == m_stFuIndictor.TYPE)//MTAP24
	{
	}
	/*RFC3984 5.2  RTP荷载格式的公共结构
	  分片包: 用于分片单个NAL单元到多个RTP包, 现存在两个版本FU-A, FU-B, 用NAL单元类型28, 29标识。
	*/
	else if (28 == m_stFuIndictor.TYPE)//FU_A
	{
		//if (m_pVideoBuf.length() <= 0) m_pVideoBuf.alloc(VIEOD_FRAME_LEN + 100);

		int NALType = pBuf[1] & 0x1f;
		memset(&m_stNalHeader, 0 ,sizeof(m_stNalHeader));
		memcpy(&m_stNalHeader, pBuf + 1, 1);//FU_HEADER赋值
		//收到帧分片包的第一包数据
		if (1 == m_stNalHeader.S) 
		{
			//如果这时为丢包状态,那么停止丢包
			m_bThrowPack = false;
			//检测IDR帧
			//5 == m_stFuHeader.TYPE 标准判断方法
			//7 == m_stFuHeader.TYPE 为了兼容三星SNP3370
			m_nFreamType = FrameType_Video_PFrame;
			if (5 == m_stNalHeader.TYPE || 7 == m_stNalHeader.TYPE)
			{
				m_nFreamType = FrameType_Video_IFrame;
				m_bIsWaitingIDRFream = false;
				m_bIsRecvingIDRFream = true; //正在接收IDR帧

				//IDR帧前写入sps和pps
				m_frame->pushBuffer(m_pSpsBuffer);
				m_frame->pushBuffer(m_pPpsBuffer);
				//SAFE_COPY_VIDEO(m_pVideoBuf, m_pSpsBuffer.c_str(), m_pSpsBuffer.length(), VIEOD_FRAME_LEN);
				//SAFE_COPY_VIDEO(m_pVideoBuf, m_pPpsBuffer.c_str(), m_pPpsBuffer.length(), VIEOD_FRAME_LEN);
			}

			//设置帧数据的起始码
			m_frame->pushBuffer(H264_STATRTCODE_ADDR, H264_STATRTCODE_LEN);
			//SAFE_SET_H264_STARTCODE(m_pVideoBuf, VIEOD_FRAME_LEN);
						
			char pBufer1 = ( pBuf[0] & 0xE0 ) | NALType ;
			m_frame->pushBuffer(&pBufer1, 1);
			//SAFE_COPY_VIDEO(m_pVideoBuf, &pBufer1, 1, VIEOD_FRAME_LEN);

			//拷贝荷载数据,跳过前面的IDR帧检验位和FU_HEADER两个字节
			m_frame->pushRTPBuffer(pBuf + 2, nBufLen - 2);
			//SAFE_COPY_VIDEO(m_pVideoBuf, pBuf + 2, nBufLen - 2, VIEOD_FRAME_LEN);
		}
		//帧分片包的最后一包数据,为了更强的兼容性放宽限制
		//我们认为FU_HEADER的E字段和rtsp头得mark位符合其一都是有效的
		else if (1 == m_stRtpHeader.m || 1 == m_stNalHeader.E)
		{
			if (true == m_bThrowPack || true == m_bIsWaitingIDRFream)
			{
				m_frame = NULL;
				return -1;
			}
			m_frame->pushRTPBuffer(pBuf + 2, nBufLen - 2);
			m_frame->codeId(CodeID_Video_H264);
			m_frame->frameType(m_nFreamType);
			m_frame->timestmap(ntohl(m_stRtpHeader.ts));

//			SAFE_COPY_VIDEO(m_pVideoBuf, pBuf + 2, nBufLen - 2, VIEOD_FRAME_LEN);

			//shared_ptr<RTPAnalyzer_FrameInfo> frame = make_shared<RTPAnalyzer_FrameInfo>();
			//frame->codeid = CodeID_Video_H264;
			//frame->frametype = m_nFreamType;
			//frame->frame= m_pVideoBuf;
			//frame->timestmap = ntohl(m_stRtpHeader.ts);

			m_pFramCallBack(m_frame);

			m_frame = NULL;

			return 0;
		}
		else
		{
			//如果不是帧的第一包数据,而且处于丢帧状态,那么收到的所有包都丢弃
			if (true == m_bThrowPack || true == m_bIsWaitingIDRFream) 
			{
				m_frame = NULL;
				return -1;
			}

			//正常情况,拷贝数据
			m_frame->pushRTPBuffer(pBuf + 2, nBufLen - 2);
			//SAFE_COPY_VIDEO(m_pVideoBuf, pBuf + 2, nBufLen - 2, VIEOD_FRAME_LEN);
		}
	}
	else if (29 == m_stFuIndictor.TYPE)//FU-B
	{
	}
	else//30-31 未定义
	{
	}

	return 0;
}

