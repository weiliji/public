#include "RTSP/RTPAnalyzer.h"

using namespace Public::RTSP;

typedef struct {
	unsigned char TYPE : 5;
	unsigned char NRI : 2;
	unsigned char F : 1;
} NALU_HEADER, *LPNALU_HEADER;

typedef struct {
	unsigned char TYPE : 5;
	unsigned char NRI : 2;
	unsigned char F : 1;
} FU_INDICATOR, *LPFU_INDICATOR;

typedef struct {
	unsigned char TYPE : 5;
	unsigned char R : 1;
	unsigned char E : 1;
	unsigned char S : 1;
} FU_HEADER, *LPFU_HEADER;

static int FindStartCode2(const char *Buf)
{
	if (Buf[0] != 0 || Buf[1] != 0 || Buf[2] != 1) return 0; //判断是否为0x000001,如果是返回1
	else return 3;
}

static int FindStartCode3(const char *Buf)
{
	if (Buf[0] != 0 || Buf[1] != 0 || Buf[2] != 0 || Buf[3] != 1) return 0;//判断是否为0x00000001,如果是返回1
	else return 4;
}

struct RTPBuilder::RTPBuilderInternal
{
	int video_seq_num;
	unsigned int curtimestamp;
	unsigned int currSendNums;

	int audio_seq_num;
	int nAudiobufSize;

	bool enableAutoTimeStamp = true;

	shared_ptr<STREAM_TRANS_INFO> transinfo;

	RTPBuilderInternal() :video_seq_num(0)
		, curtimestamp(0)
		, currSendNums(0)
		, audio_seq_num(0)
		, nAudiobufSize(0)
	{

	}
	~RTPBuilderInternal()
	{
	}

	void inputExtendData(std::vector<shared_ptr<RTPPackage>>&rtptmp, shared_ptr<STREAM_TRANS_INFO> videotraninfo, const char* data, int size, const String& extenddata, int& seq_num)
	{
		const char* externbuffer = extenddata.c_str();
		uint32_t externlen = (uint32_t)extenddata.length();

		if (externlen >= MAXRTPPACKETLEN) externlen = 0;

		if (externlen > 0)
		{
			String videobuffer;
			uint32_t packSize = externlen + sizeof(RTPHEADER);
			char* pVideoBuffer = videobuffer.alloc(packSize + 100);
			memset(pVideoBuffer, 0, packSize);
			RTPHEADER* lpRtp_hdr = (RTPHEADER*)pVideoBuffer;
			{
				lpRtp_hdr->pt = videotraninfo->streaminfo.payLoad;		// H264
				lpRtp_hdr->v = RTP_VERSION;
				lpRtp_hdr->ssrc = htonl(videotraninfo->transportinfo.ssrc);
				lpRtp_hdr->ts = htonl(curtimestamp);
				lpRtp_hdr->seq = htons(seq_num++);
				lpRtp_hdr->m = size == 0 ? 1 : 0;
				lpRtp_hdr->x = 1;
			}
			memcpy(pVideoBuffer + sizeof(RTPHEADER), externbuffer, externlen);

			rtptmp.push_back(make_shared<RTPPackage>(videobuffer, 0, packSize));
		}
	}
	std::vector<shared_ptr<RTPPackage>> inputVideoData(const char* data, int size,const String& extenddata)
	{
		std::vector<shared_ptr<RTPPackage>> rtptmp;

		shared_ptr<STREAM_TRANS_INFO> videotraninfo = transinfo;
		
		if(videotraninfo == NULL) 	return rtptmp;

		inputExtendData(rtptmp,videotraninfo, data, size, extenddata, video_seq_num);


		if (size <= 4 || videotraninfo == NULL || data == NULL)
		{
			return rtptmp;
		}

		int naulstartPos = FindStartCode2(data);
		if (naulstartPos == 0)
		{
			naulstartPos = FindStartCode3(data);
			if (naulstartPos == 0)
			{
				return rtptmp;
			}
		}

		int maxSize = size + sizeof(RTPHEADER) + sizeof(FU_INDICATOR) + sizeof(FU_HEADER);


		int dataSize = size - naulstartPos;
		char nalu = data[naulstartPos];

		if (dataSize <= MAXRTPPACKETLEN)			// 分片
		{
			String videobuffer;
			char* pVideoBuffer = videobuffer.alloc(maxSize + 100);
			memset(pVideoBuffer, 0, sizeof(RTPHEADER) + sizeof(FU_INDICATOR) + sizeof(FU_HEADER));
			RTPHEADER* lpRtp_hdr = (RTPHEADER*)pVideoBuffer;
			{
				lpRtp_hdr->pt = videotraninfo->streaminfo.payLoad;		// H264
				lpRtp_hdr->v = RTP_VERSION;
				lpRtp_hdr->ssrc = htonl(videotraninfo->transportinfo.ssrc);
				lpRtp_hdr->ts = htonl(curtimestamp);
				lpRtp_hdr->seq = htons(video_seq_num++);
				lpRtp_hdr->m = 1;
			}
			int packSize = sizeof(RTPHEADER);

			LPNALU_HEADER lpNalu_hdr = (LPNALU_HEADER)(pVideoBuffer + sizeof(RTPHEADER));
			packSize += sizeof(LPNALU_HEADER);
			lpNalu_hdr->F = nalu >> 7 & 1;
			lpNalu_hdr->NRI = nalu >> 5 & 3;
			lpNalu_hdr->TYPE = nalu & 0x1F;

			char *payload = (char*)(lpNalu_hdr + 1);
			int leng = naulstartPos + 1;
			packSize += size - leng;
			memcpy(payload, data + leng, size - leng);

			rtptmp.push_back(make_shared<RTPPackage>(videobuffer, 0, packSize));
		}
		else
		{
			int k = dataSize / MAXRTPPACKETLEN;
			int l = dataSize % MAXRTPPACKETLEN;
			int n = 0;
			while (k >= n)
			{
				String videobuffer;
				char* pVideoBuffer = videobuffer.alloc(maxSize + 100);
				memset(pVideoBuffer, 0, sizeof(RTPHEADER) + sizeof(FU_INDICATOR) + sizeof(FU_HEADER));
				RTPHEADER* lpRtp_hdr = (RTPHEADER*)pVideoBuffer;
				{
					lpRtp_hdr->pt = videotraninfo->streaminfo.payLoad;		// H264
					lpRtp_hdr->v = RTP_VERSION;
					lpRtp_hdr->ssrc = htonl(videotraninfo->transportinfo.ssrc);
					lpRtp_hdr->ts = htonl(curtimestamp);
					lpRtp_hdr->m = 0;
					lpRtp_hdr->seq = htons(video_seq_num++);
				}
				int packSize = sizeof(RTPHEADER);

				FU_INDICATOR *lpFu_ind = (FU_INDICATOR*)(pVideoBuffer + sizeof(RTPHEADER));
				packSize += sizeof(FU_INDICATOR);
				lpFu_ind->F = nalu >> 7 & 1;
				lpFu_ind->NRI = nalu >> 5 & 3;
				lpFu_ind->TYPE = 28;

				FU_HEADER *lpFu_hdr = (FU_HEADER*)(lpFu_ind + 1);
				packSize += sizeof(FU_HEADER);
				lpFu_hdr->R = 0;
				lpFu_hdr->E = 0;
				lpFu_hdr->S = 0;
				int payloadSize = MAXRTPPACKETLEN;
				if (n == 0)
				{
					lpFu_hdr->S = 1;
				}

				if (n == k)
				{
					lpRtp_hdr->m = 1;
					lpFu_hdr->E = 1;
					if (l == 0)
					{
						break;
					}
					payloadSize = l - 1;
				}
				packSize += payloadSize;
				lpFu_hdr->TYPE = nalu & 0x1F;

				char *plyload = (char*)(lpFu_hdr + 1);
				int leng = naulstartPos + 1;

				memcpy(plyload, data + leng + n * MAXRTPPACKETLEN, payloadSize);

				rtptmp.push_back(make_shared<RTPPackage>(videobuffer, 0, packSize));

				n++;
			}
		}


		return rtptmp;
	}

	std::vector<shared_ptr<RTPPackage>> inputAudioData(const char* data, int size, const String& extenddata)
	{
		std::vector<shared_ptr<RTPPackage>> rtptmp;

		shared_ptr<STREAM_TRANS_INFO> audiotraninfo = transinfo;
		if(audiotraninfo == NULL) 	return rtptmp;

		inputExtendData(rtptmp, audiotraninfo, data, size, extenddata, audio_seq_num);

		if (audiotraninfo == NULL || data == NULL || size <= 0) return rtptmp;

		int maxSize = size + sizeof(RTPHEADER) + sizeof(FU_INDICATOR) + sizeof(FU_HEADER);
		String audiobuffer;
		char* pAudioBuffer = audiobuffer.alloc(maxSize + 100);
		memset(pAudioBuffer, 0, sizeof(RTPHEADER) + sizeof(FU_INDICATOR) + sizeof(FU_HEADER));

		int packSize = 0;
		RTPHEADER* lpRtp_hdr = (RTPHEADER*)pAudioBuffer;
		packSize += sizeof(RTPHEADER);
		lpRtp_hdr->pt = audiotraninfo->streaminfo.payLoad;		// G711A
		lpRtp_hdr->v = RTP_VERSION;
		lpRtp_hdr->m = 1;
		lpRtp_hdr->ssrc = htonl(audiotraninfo->transportinfo.ssrc);
		lpRtp_hdr->ts = htonl(curtimestamp);
		lpRtp_hdr->seq = htons(audio_seq_num++);

		char *payload = (char*)(lpRtp_hdr + 1);
		packSize += size;
		memcpy(payload, data, size);

		rtptmp.push_back(make_shared<RTPPackage>(audiobuffer, 0, packSize));

		return rtptmp;

	}
};


RTPBuilder::RTPBuilder(const shared_ptr<STREAM_TRANS_INFO>& transinfo)
{
	internal = new RTPBuilderInternal();
	internal->transinfo = transinfo;
}


RTPBuilder::~RTPBuilder()
{
	SAFE_DELETE(internal);
}

std::vector<shared_ptr<RTPPackage>> RTPBuilder::inputFrame(const shared_ptr<RTPFrame>& frame)
{
	std::vector<shared_ptr<RTPPackage>> rtplist = frame->toRtpPackage();
	if (rtplist.size() > 0)
	{
		return rtplist;
	}
	else
	{
		String data = frame->framedata();
		
		if (frame->frameType() == FrameType_Audio)
		{
			if (internal->enableAutoTimeStamp)
			{
				// (int)(((float)curtimestamp / 3600 / nFrameRate) * nSamplingRate);
				{
					if (internal->currSendNums == 0)
					{
						internal->curtimestamp = 0;
					}
					else
					{
						if (internal->transinfo->streaminfo.sampRate == 0) internal->transinfo->streaminfo.sampRate = 8000;
						internal->curtimestamp += (1000000 / internal->transinfo->streaminfo.sampRate);
					}
					internal->currSendNums++;
				}
			}
			else
			{
				internal->curtimestamp = frame->timestmap();
			}
			
			return internal->inputAudioData(data.c_str(), (int)data.length(),frame->extendData());
		}
		else
		{
			if (internal->enableAutoTimeStamp)
			{
				{
					if (internal->currSendNums == 0)
					{
						internal->curtimestamp = 0;
					}
					else
					{
						if (internal->transinfo->streaminfo.video.frameRate == 0) internal->transinfo->streaminfo.video.frameRate = 25;
						if (internal->transinfo->streaminfo.sampRate == 0) internal->transinfo->streaminfo.sampRate = 90000;

						internal->curtimestamp += internal->transinfo->streaminfo.sampRate / internal->transinfo->streaminfo.video.frameRate;
					}

					internal->currSendNums++;
				}
			}
			else
			{
				internal->curtimestamp = frame->timestmap();
			}

			return internal->inputVideoData(data.c_str(), (int)data.length(), frame->extendData());
		}
	}

	

	return std::vector<shared_ptr<RTPPackage>>();
}

void RTPBuilder::setAutoTimeStampEnable(bool enable)
{
	internal->enableAutoTimeStamp = enable;
}

