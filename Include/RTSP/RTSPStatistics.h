#pragma once

#include "Base/Base.h"
#include "RTSP/Defs.h"
#include "RTSP/RTSPStructs.h"
#include "RTSP/RTCPPackage.h"
using namespace Public::Base;

namespace Public {
namespace RTSP {

class RTPPackage;

//RTSP数据收发统计
class RTSP_API RTSPStatistics
{
public:
	struct RecvStatistics
	{
		uint64_t		needCountPackage = 0;		//该接受的数据总数
		uint64_t		realRecvPackage = 0;		//实际接受的数据总数

		uint32_t		prevTimesmap = 0;			//上一帧事件搓
		uint32_t		prevSeq = 0;				//上一帧的包序号

		uint32_t		wrapAroundCount = 0;		//序号反转次数

		uint32_t		rtpSsrc = 0;				//recv rtp ssrc
	};
	struct SendStatustics
	{
		uint32_t		sendPacketCount = 0;		//发送包数
		uint32_t		sendOctetCount = 0;			//数据大小，不含RTP头

		uint32_t		prevTimesmap = 0;			//上一帧事件搓
	};
public:
	//interval为统计的时间间隔
	RTSPStatistics(uint32_t interval = 10*1000);
	virtual ~RTSPStatistics();

	void inputRecvPackage(const shared_ptr<RTPPackage>& rtp);
	void inputRecvPackage(const shared_ptr<RTCPPackage>& rtcppackage);

	void inputSendPackage(const shared_ptr<RTPPackage>& rtp);
	void inputSendPackage(const shared_ptr<RTCPPackage>& rtcppackage);

	void getRecvStatistics(RecvStatistics& statistics);
	void getSendStatistics(SendStatustics& statistics);

	void onPoolTimerProc();
private:
	struct RTSPStatisticsInternal;
	RTSPStatisticsInternal* internal;
};


}
}