#pragma once
#pragma once
#include "Defs.h"
#include "Base/Base.h"
#include "RTSPStructs.h"
using namespace Public::Base;

namespace Public {
namespace RTSP {

enum RTCPType
{
	RTCPType_SR = 200,
	RTCPType_RR = 201,
	RTCPType_SDES = 202,
	RTCPType_BYE = 203,
	RTCPType_APP = 204,
};

struct RTCPSenderReport
{
	uint32_t ssrc = 0;
	uint32_t ntptime_msw = 0;
	uint32_t ntptime_lsw = 0;
	uint32_t rtptimestamp = 0;
	uint32_t packetcount = 0;
	uint32_t octetcount = 0;
};

struct RTCPReceiverReport
{
	uint32_t rssrc = 0;	//report ssrc
	uint32_t ssrc = 0; // Identifies about which SSRC's data this report is...
	uint8_t fractionlost = 0;
	uint8_t packetslost[3] = {0};
	uint32_t exthighseqnr = 0;
	uint32_t jitter = 0;
	uint32_t lsr = 0;
	uint32_t dlsr = 0;
};

enum RTCP_SDES_Id
{
	RTCP_SDES_ID_CNAME = 1,
	RTCP_SDES_ID_NAME = 2,
	RTCP_SDES_ID_EMAIL = 3,
	RTCP_SDES_ID_PHONE = 4,
	RTCP_SDES_ID_LOCATION = 5,
	RTCP_SDES_ID_TOOL = 6,
	RTCP_SDES_ID_NOTE = 7,
	RTCP_SDES_ID_PRIVATE = 8,
};


struct RTCPSourceDescribe
{
	uint32_t		ssrc = 0;
	RTCP_SDES_Id	id = RTCP_SDES_ID_CNAME;
	Value			text;
};

struct RTSP_API RTCPItem
{
	RTCPItem(){}
	virtual ~RTCPItem() {}

	virtual bool parse(const char* buffer, int len) = 0;
	virtual bool copy(void* dst, uint32_t dstsize) = 0;
	virtual std::string toString() = 0;
	virtual RTCPType type() = 0;
};

class RTSP_API RTCPPackage
{
public:
	RTCPPackage();
	RTCPPackage(const RTCPPackage& pkt);
	virtual ~RTCPPackage();

	//解析RTCP包
	ErrorInfo parse(const char* buffer, uint32_t len);
	//获取RTCP包中的数据类型
	std::vector<shared_ptr<RTCPItem>> items() const;

	//根据类型来获取相应的数据
	ErrorInfo getSenderReport(const shared_ptr<RTCPItem>& item,RTCPSenderReport& sr) const;
	ErrorInfo getReciverReport(const shared_ptr<RTCPItem>& item, RTCPReceiverReport& rr) const;
	ErrorInfo getSourceDesribe(const shared_ptr<RTCPItem>& item, RTCPSourceDescribe& sdes) const;


	//build rtcp
	void addSenderReport(const shared_ptr<STREAM_TRANS_INFO>& transinfo);
	void addReciverReport(const shared_ptr<STREAM_TRANS_INFO>& transinfo, RTCPSenderReport* sr = NULL);
	void addSourceDesribe(const shared_ptr<STREAM_TRANS_INFO>& transinfo, RTCP_SDES_Id id, const Value& val);

	std::string toString() const;
private:
	struct RTCPPackageInternal;
	RTCPPackageInternal* internal;
};

}
}