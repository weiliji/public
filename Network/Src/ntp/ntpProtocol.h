#include "Base/Base.h"

using namespace Public::Base;

namespace Public
{
namespace Network
{
namespace NTP
{

#define VERSION_3 3
#define VERSION_4 4

#define MODE_CLIENT 3
#define MODE_SERVER 4

#define NTP_LI 0
#define NTP_VN VERSION_3
#define NTP_MODE MODE_CLIENT
#define NTP_STRATUM 0
#define NTP_POLL 4
#define NTP_PRECISION -6

#define NTP_HLEN 48

#define TIMEOUT 10

#define BUFSIZE 1500

#define JAN_1970 0x83aa7e80

#define NTP_CONV_FRAC32(x) (uint64_t)((x) * ((uint64_t)1 << 32))
#define NTP_REVE_FRAC32(x) ((double)((double)(x) / ((uint64_t)1 << 32)))

#define NTP_CONV_FRAC16(x) (uint32_t)((x) * ((uint32_t)1 << 16))
#define NTP_REVE_FRAC16(x) ((double)((double)(x) / ((uint32_t)1 << 16)))

#define USEC2FRAC(x) ((uint32_t)NTP_CONV_FRAC32((x) / 1000000.0))
#define FRAC2USEC(x) ((uint32_t)NTP_REVE_FRAC32((x)*1000000.0))

#define NTP_LFIXED2DOUBLE(x) ((double)(ntohl(((struct l_fixedpt *)(x))->intpart) - JAN_1970 + FRAC2USEC(ntohl(((struct l_fixedpt *)(x))->fracpart)) / 1000000.0))

struct s_fixedpt
{
	uint16_t intpart;
	uint16_t fracpart;
};

struct l_fixedpt
{
	uint32_t intpart;
	uint32_t fracpart;
};

#ifndef __LITTLE_ENDIAN
#define __LITTLE_ENDIAN
#endif

struct ntphdr
{
#ifdef __BID_ENDIAN
	unsigned int ntp_li : 2;
	unsigned int ntp_vn : 3;
	unsigned int ntp_mode : 3;
#endif
#ifdef __LITTLE_ENDIAN
	unsigned int ntp_mode : 3;
	unsigned int ntp_vn : 3;
	unsigned int ntp_li : 2;
#endif
	uint8_t ntp_stratum;
	uint8_t ntp_poll;
	int8_t ntp_precision;
	struct s_fixedpt ntp_rtdelay;
	struct s_fixedpt ntp_rtdispersion;
	uint32_t ntp_refid;
	struct l_fixedpt ntp_refts;
	struct l_fixedpt ntp_orits;
	struct l_fixedpt ntp_recvts;
	struct l_fixedpt ntp_transts;
};

class NTPProtocol
{
public:
	static void buildRequest(char buffer[NTP_HLEN], const Time &time)
	{
		u_long timesec = (u_long)time.makeTime();

		ntphdr *ntp = (ntphdr *)buffer;

		memset(ntp, 0, sizeof(ntphdr));

		ntp->ntp_li = NTP_LI;
		ntp->ntp_vn = NTP_VN;
		ntp->ntp_mode = NTP_MODE;
		ntp->ntp_stratum = NTP_STRATUM;
		ntp->ntp_poll = NTP_POLL;
		ntp->ntp_precision = NTP_PRECISION;

		ntp->ntp_transts.intpart = htonl(timesec + JAN_1970);
		ntp->ntp_transts.fracpart = htonl(USEC2FRAC(time.millisecond * 1000));
	}

	static bool parseAndBuildResponse(const char *recvbuf, uint32_t len, char buffer[NTP_HLEN], const Time &time)
	{
		if (len != NTP_HLEN)
			return false;

		ntphdr *recvntp = (ntphdr *)recvbuf;
		ntphdr *sendntp = (ntphdr *)buffer;

		*sendntp = *recvntp;

		sendntp->ntp_orits = recvntp->ntp_transts;

		{
			u_long timesec = (u_long)time.makeTime();
			sendntp->ntp_recvts.intpart = htonl(timesec + JAN_1970);
			sendntp->ntp_recvts.fracpart = htonl(USEC2FRAC(time.millisecond * 1000));
		}

		sendntp->ntp_transts = sendntp->ntp_recvts;

		return true;
	}

	static bool parseResponse(const char *buffer, uint32_t len, const Time &time, Time &recvtime)
	{
		if (len != NTP_HLEN)
			return false;

		u_long timesec = (u_long)time.makeTime();

		ntphdr *ntp = (ntphdr *)buffer;

		double t1 = NTP_LFIXED2DOUBLE(&ntp->ntp_orits);
		double t2 = NTP_LFIXED2DOUBLE(&ntp->ntp_recvts);
		double t3 = NTP_LFIXED2DOUBLE(&ntp->ntp_transts);
		double t4 = (double)(timesec + time.millisecond);

		double offset = ((t2 - t1) + (t3 - t4)) / 2;

		recvtime.breakTime(timesec + (int)offset + 28800);
		recvtime.millisecond = (int)(time.millisecond + offset - (int)offset);

		return true;
	}
};

} // namespace NTP
} // namespace Network
} // namespace Public