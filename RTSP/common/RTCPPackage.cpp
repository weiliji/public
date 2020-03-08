#include "RTSP/RTCPPackage.h"
#include "RTSP/RTSPStatistics.h"

namespace Public {
namespace RTSP {

#define RTCP_VERSION							 2

#define RTP_NTPTIMEOFFSET						2208988800UL

#define RTCPITEMMINLEN							4



#define RTCP_SDES_NUMITEMS_NONPRIVATE			7
#define RTCP_SDES_MAXITEMLENGTH					255

#define RTCP_BYE_MAXREASONLENGTH				255
#define RTCP_DEFAULTMININTERVAL					5.0	
#define RTCP_DEFAULTBANDWIDTHFRACTION			0.05
#define RTCP_DEFAULTSENDERFRACTION				0.25
#define RTCP_DEFAULTHALFATSTARTUP				true
#define RTCP_DEFAULTIMMEDIATEBYE				true
#define RTCP_DEFAULTSRBYE						true

struct RTCPHEADER
{
	uint8_t count : 5; /* varies by packet type */
	uint8_t padding : 1;      /* padding flag */
	uint8_t version : 2; /* protocol version */

	uint8_t packettype;    /* RTCP packet type */
	uint16_t length; /* pkt len in words, w/o this word */
};

struct RTCPItem_SR:public RTCPItem
{
	virtual bool parse(const char* buffer, int len)
	{
		if (len != sizeof(sr)) return false;

		sr = *(RTCPSenderReport*)buffer;

		return true;
	}

	virtual bool copy(void* dst, uint32_t dstsize)
	{
		if (dstsize != sizeof(sr)) return false;

		*(RTCPSenderReport*)dst = sr;

		return true;
	}

	virtual std::string toString() { return string((const char*)&sr, sizeof(sr)); }
	virtual RTCPType type() {return RTCPType_SR;}

	RTCPSenderReport		sr;
};

struct RTCPItem_RR :public RTCPItem
{
	virtual bool parse(const char* buffer, int len)
	{
		if (len != sizeof(rr)) return false;

		rr = *(RTCPReceiverReport*)buffer;

		return true;
	}

	virtual bool copy(void* dst, uint32_t dstsize)
	{
		if (dstsize != sizeof(rr)) return false;

		*(RTCPReceiverReport*)dst = rr;

		return true;
	}

	virtual std::string toString() { return string((const char*)&rr, sizeof(rr)); }
	virtual RTCPType type() { return RTCPType_RR; }

	RTCPReceiverReport		rr;
};

struct RTCPItem_SDES :public RTCPItem
{
	virtual bool parse(const char* buffer, int len)
	{
		if (len < 6) return false;

		uint32_t offset = 0;

		sdes.ssrc = ntohl(*(uint32_t*)(buffer + offset));
		offset += sizeof(uint32_t);

		sdes.id = (RTCP_SDES_Id)buffer[offset];
		offset += 1;

		uint32_t textlen = buffer[offset];
		offset += 1;

		if (textlen + offset < (uint32_t)len) return false;
		sdes.text = Value(std::string(buffer + offset, textlen));


		return true;
	}

	virtual bool copy(void* dst, uint32_t dstsize)
	{
		if (dstsize != sizeof(sdes)) return false;

		*(RTCPSourceDescribe*)dst = sdes;

		return true;
	}
	virtual std::string toString() 
	{
		std::string sdstr;

		std::string valstr = sdes.text.readString();

		//1: add ssrc
		uint32_t nssrc = htonl(sdes.ssrc);
		sdstr = string((const char*)&nssrc, sizeof(uint32_t));

		//2: add sdes type
		char type = (char)sdes.id;
		sdstr += string(&type, 1);

		//3: add sdes len
		char len = (char)valstr.length();
		sdstr += string(&len, 1);

		//4: add sdes text
		sdstr += valstr;

		uint32_t specsize = sdstr.length() % RTCPITEMMINLEN;
		//不是4的倍数，填充
		if (specsize != 0)
		{
			for (uint32_t i = 0; i < 4 - specsize; i++)
			{
				char end = 0;
				sdstr += string(&end, 1);
			}
		}

		return sdstr;
	}
	virtual RTCPType type() { return RTCPType_SDES; }

	RTCPSourceDescribe		sdes;
};


struct RTCPPackage::RTCPPackageInternal
{
	struct ItemsInfo
	{
		RTCPType type;
		std::vector<shared_ptr<RTCPItem> > items;
	};

	std::vector<ItemsInfo> items;

	void addItem(const shared_ptr<RTCPItem>& item)
	{
		for (size_t i = 0; i < items.size(); i++)
		{
			if (items[i].type == item->type())
			{
				items[i].items.push_back(item);

				return;
			}
		}

		ItemsInfo info;
		info.type = item->type();
		info.items.push_back(item);

		items.push_back(info);
	}
};


RTCPPackage::RTCPPackage()
{
	internal = new RTCPPackageInternal;
}

RTCPPackage::RTCPPackage(const RTCPPackage& pkt)
{
	internal = new RTCPPackageInternal;
	internal->items = pkt.internal->items;
}
RTCPPackage::~RTCPPackage()
{
	SAFE_DELETE(internal);
}

//解析RTCP包
ErrorInfo RTCPPackage::parse(const char* buffer, uint32_t bufferlen)
{
	while (bufferlen >= sizeof(RTCPHEADER))
	{
		RTCPHEADER* header = (RTCPHEADER*)buffer;
		if (header->version != RTCP_VERSION)
		{
			return ErrorInfo(Error_Code_ParseParam);
		}
		uint16_t len = ntohs(header->length);
		if (len*RTCPITEMMINLEN + sizeof(RTCPHEADER) > bufferlen)
		{
			return ErrorInfo(Error_Code_ParseParam);
		}
		
		shared_ptr<RTCPItem> item;
		switch (header->packettype)
		{
		case RTCPType_SR:
			item = make_shared<RTCPItem_SR>();
			break;
		case RTCPType_RR:
			item = make_shared<RTCPItem_RR>();
			break;
		case RTCPType_SDES:
			item = make_shared<RTCPItem_SDES>();
			break;
		default:
			break;
		}

		if (item)
		{
			if (!item->parse(buffer + sizeof(RTCPHEADER), len * RTCPITEMMINLEN))
			{
				return ErrorInfo(Error_Code_ParseParam);
			}

			internal->addItem(item);
		}

		bufferlen -= len * RTCPITEMMINLEN + sizeof(RTCPHEADER);
		buffer += len * RTCPITEMMINLEN + sizeof(RTCPHEADER);
	}

	return ErrorInfo();
}

std::vector<shared_ptr<RTCPItem>> RTCPPackage::items() const
{
	std::vector<shared_ptr<RTCPItem>> items;
	for (size_t i = 0; i < internal->items.size(); i++)
	{
		for (size_t j = 0; j < internal->items[i].items.size(); j++)
		{
			items.push_back(internal->items[i].items[j]);
		}
	}

	return items;
}
//根据类型来获取相应的数据
ErrorInfo RTCPPackage::getSenderReport(const shared_ptr<RTCPItem>& item,RTCPSenderReport& sr) const
{
	if (item && item->type() == RTCPType_SR && item->copy(&sr, sizeof(sr)))
	{
		return ErrorInfo();
	}

	return ErrorInfo(Error_Code_ParseParam);
}
ErrorInfo RTCPPackage::getReciverReport(const shared_ptr<RTCPItem>& item, RTCPReceiverReport& rr) const
{
	if (item && item->type() == RTCPType_RR && item->copy(&rr, sizeof(rr)))
	{
		return ErrorInfo();
	}

	return ErrorInfo(Error_Code_ParseParam);
}
ErrorInfo RTCPPackage::getSourceDesribe(const shared_ptr<RTCPItem>& item, RTCPSourceDescribe& sdes) const
{
	if (item && item->type() == RTCPType_SDES && item->copy(&sdes, sizeof(sdes)))
	{
		return ErrorInfo();
	}
	return ErrorInfo(Error_Code_ParseParam);
}


//build rtcp
void RTCPPackage::addSenderReport(const shared_ptr<STREAM_TRANS_INFO>& transport)
{
	shared_ptr<RTSPStatistics> statistics = transport->rtspstatistics.lock();
	if (statistics == NULL) return;

	RTSPStatistics::SendStatustics sendstatustics;
	statistics->getSendStatistics(sendstatustics);

	Time nowtime = Time::getCurrentTime();
	uint64_t micsrc = Time::getCurrentMicroSecond();

	double micsrctmp = micsrc * 1.0;
	double miclsw = micsrctmp / 1000000.0 * 65536.0 * 65536.0;

	shared_ptr<RTCPItem_SR> item = make_shared<RTCPItem_SR>();
	
	item->sr.ssrc = htonl(transport->transportinfo.ssrc);
	item->sr.rtptimestamp = htonl(sendstatustics.prevTimesmap);
	item->sr.packetcount = htonl(sendstatustics.sendPacketCount);
	item->sr.octetcount = htonl(sendstatustics.sendOctetCount);
	item->sr.ntptime_msw = htonl((uint32_t)(nowtime.makeTime() + RTP_NTPTIMEOFFSET));
	item->sr.ntptime_lsw = htonl((uint32_t)miclsw);

	internal->addItem(item);
}
void RTCPPackage::addReciverReport(const shared_ptr<STREAM_TRANS_INFO>& transport, RTCPSenderReport* sr)
{
	shared_ptr<RTSPStatistics> statistics = transport->rtspstatistics.lock();
	if (statistics == NULL) return;

	RTSPStatistics::RecvStatistics recvstatustics;
	statistics->getRecvStatistics(recvstatustics);

	shared_ptr<RTCPItem_RR> item = make_shared<RTCPItem_RR>();

	item->rr.rssrc = htonl(transport->transportinfo.ssrc);
	item->rr.ssrc = htonl(recvstatustics.rtpSsrc);
	item->rr.exthighseqnr = htonl(recvstatustics.prevSeq + 65536 * recvstatustics.wrapAroundCount);
	item->rr.lsr = htonl(recvstatustics.prevTimesmap);
	
	
	//以下jitter/dlsr/lost 等 暂不计算
	
	item->rr.fractionlost = 0;

	item->rr.packetslost[0] = 0xff;
	item->rr.packetslost[1] = 0xff;
	item->rr.packetslost[2] = 0xff;

	item->rr.jitter = htonl(300 + Time::getCurrentMilliSecond() % 100);
	item->rr.dlsr = htonl(150000 + Time::getCurrentMilliSecond() % 10000);// recvreport.dlsr);

	internal->addItem(item);
}
void RTCPPackage::addSourceDesribe(const shared_ptr<STREAM_TRANS_INFO>& transport,RTCP_SDES_Id id, const Value& val)
{
	shared_ptr<RTCPItem_SDES> item = make_shared<RTCPItem_SDES>();

	item->sdes.ssrc = htonl(transport->transportinfo.ssrc);
	item->sdes.id = id;
	item->sdes.text = val;

	internal->addItem(item);
}

std::string RTCPPackage::toString() const
{
	std::string values;

	for (size_t i = 0; i < internal->items.size(); i++)
	{
		std::string itemstr;
		for (size_t j = 0; j < internal->items[i].items.size(); j++)
		{
			itemstr += internal->items[i].items[j]->toString();
		}

		RTCPHEADER rtcpheader;
		//build rtcpheader
		{
			memset(&rtcpheader, 0, sizeof(rtcpheader));
			rtcpheader.version = RTCP_VERSION;
			rtcpheader.packettype = internal->items[i].type;
			rtcpheader.length = htons((u_short)(itemstr.length() / 4));
			rtcpheader.count = internal->items[i].items.size();
		}

		values += string((const char*)&rtcpheader, sizeof(rtcpheader)) + itemstr;
	}

	return values;
}


}
}