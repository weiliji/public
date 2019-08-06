#pragma  once
#include "rtpSession.h"

#define MAXSNNUM	0xffff
#define MAXRTPFRAMESIZE		100

//RTP∞¸≈≈–ÚÀ„∑®
class RTPSort
{
	struct RTPPackgeInfo
	{
		String			framedata;
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
		
		if (prevframesn != 0 && info.sn < prevframesn && prevframesn - info.sn > 5000)
		{
			info.sn += (MAXSNNUM + 1);
		}

		rtplist.push_back(info);
		rtplist.sort();

		while (rtplist.size() > 0)
		{
			RTPPackgeInfo info = rtplist.front();

			if (prevframesn == 0 || (uint16_t)(prevframesn + 1) == (uint16_t)info.sn || rtplist.size() > MAXRTPFRAMESIZE)
			{
				rtplist.pop_front();

				datacalblack(transportinfo, *header, StringBuffer(info.framedata.c_str() + sizeof(RTPHEADER), info.framedata.length() - sizeof(RTPHEADER)));

				if (prevframesn != 0 && (uint16_t)(prevframesn + 1) != (uint16_t)info.sn)
				{
					logwarn("RTP start sn %d to sn :%d loss", prevframesn, info.sn);
				}

				prevframesn = info.sn;
			}
			else
			{
				break;
			}
		}
		
		if (rtplist.size() == 0 && prevframesn >= MAXSNNUM)
		{
			prevframesn -= (MAXSNNUM + 1);
		}
	}
private:
	shared_ptr<STREAM_TRANS_INFO>	transportinfo;
	RTPSession::MediaDataCallback	datacalblack;

	std::list<RTPPackgeInfo>		rtplist;

	uint32_t						prevframesn;
};
