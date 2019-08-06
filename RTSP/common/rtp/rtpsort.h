#pragma  once
#include "rtpSession.h"

//RTP包排序算法
class RTPSort
{
	struct RTPPackgeInfo
	{
		String			framedata;
		StringBuffer	rtpbuffer;
		uint32_t		sn;
		uint32_t		tiemstmap;
		bool			mark;

		bool operator < (const RTPPackgeInfo& info) const
		{
			return sn < info.sn;
		}
	};
public:
	RTPSort(const shared_ptr<STREAM_TRANS_INFO>& _trans,const RTPSession::MediaDataCallback& _datacallback):transportinfo(_trans),datacalblack(_datacallback), prevframesn(0){}
	~RTPSort() {}

	void inputRtpData(const String& buffer)
	{
		if (buffer.length() <= sizeof(RTPHEADER)) return;

		RTPHEADER* header = (RTPHEADER*)buffer.c_str();

		if (header->v != RTP_VERSION)
		{
			return;
		}
		RTPPackgeInfo info;
		info.framedata = buffer;
		info.mark = header->m;
		info.sn = ntohs(header->seq);
		info.tiemstmap = ntohl(header->ts);
		info.rtpbuffer = StringBuffer(buffer.c_str() + sizeof(RTPHEADER), buffer.length() - sizeof(RTPHEADER));


		//第一包或者包连续
		if (prevframesn == 0 || (uint16_t)(prevframesn + 1) == info.sn)
		{
			datacalblack(transportinfo, *header, info.rtpbuffer);

			prevframesn = info.sn;
		}



	}
private:
	shared_ptr<STREAM_TRANS_INFO>	transportinfo;
	RTPSession::MediaDataCallback	datacalblack;

	std::vector<RTPSort>	rtplist;

	uint16_t				 prevframesn;
};
