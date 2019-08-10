#include "RTSP/RTPAnalyzer.h"

using namespace Public::RTSP;

typedef struct
{
	unsigned char csrc_len:4;
	unsigned char extension:1;
	unsigned char padding:1;
	unsigned char version:2;
	unsigned char payload:7;
	unsigned char marker:1;
	unsigned short seq_no;
	unsigned  long timestamp;
	unsigned long ssrc;
} RTP_FIXED_HEADER, *LPRTP_FIXED_HEADER;

typedef struct {
	unsigned char TYPE:5;
	unsigned char NRI:2;
	unsigned char F:1;
} NALU_HEADER, *LPNALU_HEADER;

typedef struct {
	unsigned char TYPE:5;
	unsigned char NRI:2;
	unsigned char F:1;
} FU_INDICATOR, *LPFU_INDICATOR;

typedef struct {
	unsigned char TYPE:5;
	unsigned char R:1;
	unsigned char E:1;
	unsigned char S:1;
} FU_HEADER, *LPFU_HEADER;

static int FindStartCode2 (unsigned char *Buf)
{
	if(Buf[0]!=0 || Buf[1]!=0 || Buf[2] !=1) return 0; //判断是否为0x000001,如果是返回1
	else return 3;
}

static int FindStartCode3 (unsigned char *Buf)
{
	if(Buf[0]!=0 || Buf[1]!=0 || Buf[2] !=0 || Buf[3] !=1) return 0;//判断是否为0x00000001,如果是返回1
	else return 4;
}

struct RTPBuilder::RTPBuilderInternal
{
	RTPBuilder::RTPPackageCallback rtpcallback;
	int video_seq_num;
	int nFrameRate;
	unsigned long curtimestamp;

	int audio_seq_num;
	int nAudiobufSize;
	int nSamplingRate;

	shared_ptr<MEDIA_INFO> rtspmedia;

	RTPBuilderInternal() :audio_seq_num(0)
		, video_seq_num(0)
		, nFrameRate(25)
		, curtimestamp(0)
		, nAudiobufSize(0)
		, nSamplingRate(8000)
	{

	}
	~RTPBuilderInternal()
	{
	}
};


RTPBuilder::RTPBuilder(const shared_ptr<MEDIA_INFO>& rtspmedia,const RTPPackageCallback &callback)
{
	internal = new RTPBuilderInternal();
	internal->rtpcallback = callback;
	internal->rtspmedia = rtspmedia;
}


RTPBuilder::~RTPBuilder()
{
	SAFE_DELETE(internal);
}

void RTPBuilder::setFrameRate(int framerate)
{
	internal->nFrameRate = framerate;
}

int RTPBuilder::getFrameRate()
{
	return internal->nFrameRate;
}

void RTPBuilder::setSamplingRate(int samplingRate)
{
	internal->nSamplingRate = samplingRate;
}

int RTPBuilder::getSamplingRate()
{
	return internal->nSamplingRate;
}

bool RTPBuilder::inputVideoData(unsigned char* data, int size)
{
	shared_ptr<STREAM_TRANS_INFO> videotraninfo = internal->rtspmedia->videoStreamInfo();

	if (size <= 4 || videotraninfo == NULL || data == NULL)
	{
		return false;
	}

	int naulstartPos = FindStartCode2(data);
	if (naulstartPos == 0)
	{
		naulstartPos = FindStartCode3(data);
		if (naulstartPos == 0)
		{
			return false;
		}
	}

	int maxSize = size + sizeof(RTP_FIXED_HEADER) + sizeof(FU_INDICATOR) + sizeof(FU_HEADER);
	
	internal->curtimestamp += 90000 / internal->nFrameRate;

		

	int dataSize = size - naulstartPos;
	char nalu = data[naulstartPos];
	
	if (dataSize <= MAXRTPPACKETLEN)			// 分片
	{
		String videobuffer;
		char* pVideoBuffer = videobuffer.alloc(maxSize);
		memset(pVideoBuffer, 0, sizeof(RTP_FIXED_HEADER) + sizeof(FU_INDICATOR) + sizeof(FU_HEADER));
		LPRTP_FIXED_HEADER lpRtp_hdr = (LPRTP_FIXED_HEADER)pVideoBuffer;
		{
			lpRtp_hdr->payload = videotraninfo->streaminfo.nPayLoad;		// H264
			lpRtp_hdr->version = RTP_VERSION;
			lpRtp_hdr->ssrc = htonl(videotraninfo->transportinfo.ssrc);
			lpRtp_hdr->timestamp = htonl(internal->curtimestamp);
			lpRtp_hdr->seq_no = htons(internal->video_seq_num++);
			lpRtp_hdr->marker = 1;
		}
		int packSize = sizeof(RTP_FIXED_HEADER);		

		LPNALU_HEADER lpNalu_hdr = (LPNALU_HEADER)(pVideoBuffer + sizeof(RTP_FIXED_HEADER));
		packSize += sizeof(LPNALU_HEADER);
		lpNalu_hdr->F = nalu >> 7 & 1;
		lpNalu_hdr->NRI = nalu >> 5 & 3;
		lpNalu_hdr->TYPE = nalu & 0x1F;

		char *payload = (char*)(lpNalu_hdr + 1);
		int leng = naulstartPos + 1;
		packSize += size - leng;
		memcpy(payload, data + leng, size - leng);
		
		internal->rtpcallback(RTPPackage(videobuffer,0,packSize));
		
		return true;
	}

	int k = dataSize / MAXRTPPACKETLEN;
	int l = dataSize % MAXRTPPACKETLEN;
	int n = 0;
	while (k >= n)
	{
		String videobuffer;
		char* pVideoBuffer = videobuffer.alloc(maxSize);
		memset(pVideoBuffer, 0, sizeof(RTP_FIXED_HEADER) + sizeof(FU_INDICATOR) + sizeof(FU_HEADER));
		LPRTP_FIXED_HEADER lpRtp_hdr = (LPRTP_FIXED_HEADER)pVideoBuffer;
		{
			lpRtp_hdr->payload = videotraninfo->streaminfo.nPayLoad;		// H264
			lpRtp_hdr->version = RTP_VERSION;
			lpRtp_hdr->ssrc = htonl(videotraninfo->transportinfo.ssrc);
			lpRtp_hdr->timestamp = htonl(internal->curtimestamp);
			lpRtp_hdr->marker = 0;
			lpRtp_hdr->seq_no = htons(internal->video_seq_num++);
		}
		int packSize = sizeof(RTP_FIXED_HEADER);		

		FU_INDICATOR *lpFu_ind = (FU_INDICATOR*)(pVideoBuffer + sizeof(RTP_FIXED_HEADER));
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
			lpRtp_hdr->marker = 1;
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


		internal->rtpcallback(RTPPackage(videobuffer, 0, packSize));

		n++;
	}
	return true;
}

bool RTPBuilder::inputAudioData(unsigned char* data, int size)
{
	shared_ptr<STREAM_TRANS_INFO> audiotraninfo = internal->rtspmedia->audioStreamInfo();
	
	if (audiotraninfo == NULL || data == NULL || size <= 0) return false;

	int maxSize = size + sizeof(RTP_FIXED_HEADER) + sizeof(FU_INDICATOR) + sizeof(FU_HEADER);
	String audiobuffer;
	char* pAudioBuffer = audiobuffer.alloc(maxSize);
	memset(pAudioBuffer, 0, sizeof(RTP_FIXED_HEADER) + sizeof(FU_INDICATOR) + sizeof(FU_HEADER));
	
	int packSize = 0;
	LPRTP_FIXED_HEADER lpRtp_hdr = (LPRTP_FIXED_HEADER)pAudioBuffer;
	packSize += sizeof(RTP_FIXED_HEADER);
	lpRtp_hdr->payload = audiotraninfo->streaminfo.nPayLoad;		// G711A
	lpRtp_hdr->version = RTP_VERSION;
	lpRtp_hdr->marker = 1;
	lpRtp_hdr->ssrc = htonl(audiotraninfo->transportinfo.ssrc);
	int time = (int)(((float)internal->curtimestamp / 3600 / internal->nFrameRate) * internal->nSamplingRate);
	lpRtp_hdr->timestamp = htonl(time);
	lpRtp_hdr->seq_no = htons(internal->audio_seq_num++);

	char *payload = (char*)(lpRtp_hdr + 1);
	packSize += size;
	memcpy(payload, data, size);
	
	internal->rtpcallback(RTPPackage(audiobuffer, 0, packSize));

	return true;

}