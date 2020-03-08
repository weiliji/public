#include "RTSP/RTSPStatistics.h"
#include "rtspDefine.h"

namespace Public {
namespace RTSP {


struct RTSPStatistics::RTSPStatisticsInternal
{
	uint32_t interval			= 10000;	

	RecvStatistics					recvstatistics;
	SendStatustics					sendstatistics;
	
	uint64_t						startStatisRecvTime = 0;
	uint64_t						prevRecvSn = 0;
};

RTSPStatistics::RTSPStatistics(uint32_t interval)
{
	internal = new RTSPStatisticsInternal;
	internal->interval = interval;

	memset(&internal->recvstatistics, 0, sizeof(internal->recvstatistics));
	memset(&internal->sendstatistics, 0, sizeof(internal->sendstatistics));
}
RTSPStatistics::~RTSPStatistics()
{
	SAFE_DELETE(internal);
}

void RTSPStatistics::inputRecvPackage(const shared_ptr<RTPPackage>& rtp)
{
	const RTPHEADER& header = rtp->rtpHeader();

	if (header.v != RTP_VERSION)
	{
		return;
	}

	uint32_t sn = ntohs(header.seq);
	uint32_t timestmap = ntohl(header.ts);

	uint64_t nowtime = Time::getCurrentMilliSecond();
	if (internal->startStatisRecvTime == 0 || (internal->startStatisRecvTime != 0 && nowtime - internal->startStatisRecvTime >= internal->interval))
	{
		internal->recvstatistics.needCountPackage = 0;
		internal->recvstatistics.realRecvPackage = 0;
		internal->startStatisRecvTime = nowtime;
	}

	if (internal->recvstatistics.rtpSsrc == 0)
	{
		internal->recvstatistics.rtpSsrc = ntohl(header.ssrc);
	}
	if (sn < internal->prevRecvSn)
	{
		internal->recvstatistics.wrapAroundCount++;
	}

	{
		uint32_t needRecvPktsize = 0;
		
		if (internal->recvstatistics.prevSeq == 0 && internal->recvstatistics.realRecvPackage == 0)
		{
			internal->recvstatistics.needCountPackage = 1;
			internal->recvstatistics.prevSeq = sn;
		}

		else
		{
			if (sn >= internal->recvstatistics.prevSeq)
			{
				needRecvPktsize += (sn - internal->recvstatistics.prevSeq);
			}
			else
			{
				needRecvPktsize += (sn + MAXRTPSNNUM - internal->recvstatistics.prevSeq);
			}

			if (needRecvPktsize > 1)
			{
				//int a = 0;
			}

			internal->recvstatistics.needCountPackage += needRecvPktsize;
		}
		internal->recvstatistics.realRecvPackage++;
	}


	internal->recvstatistics.prevSeq = sn;
	internal->recvstatistics.prevTimesmap = timestmap;
}
void RTSPStatistics::inputRecvPackage(const shared_ptr<RTCPPackage>& rtcppackage) {}

void RTSPStatistics::inputSendPackage(const shared_ptr<RTPPackage>& rtp) 
{
	const RTPHEADER& header = rtp->rtpHeader();
	if (header.v != RTP_VERSION)
	{
		return;
	}

	//uint32_t sn = ntohs(header->seq);
	uint32_t timestmap = ntohl(header.ts);


	internal->sendstatistics.prevTimesmap = timestmap;
	internal->sendstatistics.sendPacketCount++;
	internal->sendstatistics.sendOctetCount += rtp->rtpDataLen();
}
void RTSPStatistics::inputSendPackage(const shared_ptr<RTCPPackage>& rtcppackage) {}

void RTSPStatistics::getRecvStatistics(RecvStatistics& statistics)
{
	statistics = internal->recvstatistics;
}

void RTSPStatistics::getSendStatistics(SendStatustics& statistics)
{
	statistics = internal->sendstatistics;
}

void RTSPStatistics::onPoolTimerProc() {}

}
}