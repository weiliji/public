#pragma once
#include "Base/Base.h"
#include "Defs.h"
#include "RTSP/RTSP.h"

using namespace Public::Base;
using namespace Public::RTSP;

#ifndef WIN32
typedef void* HWND;
#endif


namespace Milesight {
namespace Player {

struct FrameInfo
{
	CodeID codeid;					// 编码类型
	FrameType frameType;			// 帧类型
	uint32_t frameNo;				// 帧序号
	uint64_t timestamp;				// 时间戳
	uint64_t utctime;				// utc时间
	String data;					// 帧数据

	int channels;					// 声道
	int samplerate;					// 采样率
	int samplebits;					// 采样位数
};

struct DataInfo
{
	const char*		data = NULL;
	uint32_t		size = 0;
};

#define MAXVIDEODATASIZE	5

typedef struct {
	int				width = 0;				//宽
	int				height = 0;				//高
	int				frameRate;				//帧率
	union
	{
		struct{
			uint8_t *pY;
			uint8_t *pU;
			uint8_t *pV;

			uint32_t linesizeY;			// Y 分量的跨度
			uint32_t linesizeUV;		// UV 分量的跨度
		} yuv;

		struct {
			uint8_t *pRgb;
			uint32_t linesize;			// rgb每行的跨度
		} rgb;

		struct {
			uint8_t* lpSurface;			// 解码表面
		} dxva;

	};
	FrameDataType	dataType;						//帧数据类型
}FrameVideoData;

typedef struct {
	int				sampleRate = 0;		// 采样率
	int				sampleBits = 0;		// 采样位数
	int				channels = 0;		// 声道
	DataInfo		frameData;
	FrameDataType	dataType;		//数据类型
}FrameAudioData;

typedef enum
{
	ImageType_Jpeg = 0,		//JPEG
	ImageType_Bmp,			//BMP
} ImageType;

//该对象不存在数据拷贝
class MSPLAYER_API Frame
{
public:
	Frame() {}
	virtual ~Frame() {}

	virtual FrameType frametype()const = 0;
	virtual uint64_t timestamp() const = 0;
	virtual uint64_t utctime() const = 0;
	virtual shared_ptr<FrameVideoData> videodata() const = 0;
	virtual shared_ptr<FrameAudioData> audiodata() const = 0;
	virtual shared_ptr<Frame> converTo(FrameDataType type) { return shared_ptr<Frame>(); }
	virtual ErrorInfo snap(const std::string& filename, ImageType imgType) { return ErrorInfo(Error_Code_NotSupport); }
};

//解码器对象
class MSPLAYER_API IDecoder
{
public:
	struct AudioInfo
	{
		int channels;
		int sampleRate;
		int sampleBits;
	};

	IDecoder() {}
	virtual ~IDecoder() {}

	virtual ErrorInfo init(CodeID code, const set<FrameDataType>& supportlist, const AudioInfo &info) = 0;		//初始化
	virtual ErrorInfo uninit() = 0;					//反初始化

	virtual CodeID codeid() const = 0;				//获取编码类型

	virtual std::vector<shared_ptr<Frame> > decoder(const std::shared_ptr<FrameInfo> &frameinfo) = 0;	//解码
	
	virtual ErrorInfo clean() = 0;				//清空解码缓冲区
};

//界面渲染
class MSPLAYER_API IRender
{
public:
	IRender(){}
	virtual ~IRender() {}

	virtual ErrorInfo init() = 0;				//初始化

	virtual ErrorInfo uninit() = 0;				//反初始化

	virtual ErrorInfo setAlpha(float alpha) { return {}; };
	//渲染数据/音频/视频
	virtual ErrorInfo display(const shared_ptr<Frame>& frame) = 0;

	virtual ErrorInfo setViewSize(int w, int h) { return {}; };

	virtual ErrorInfo setViewPort(int x, int y, int w, int h) { return {}; };

	virtual ErrorInfo draw(int left, int top, int right, int bottom) { return {}; };

	virtual ErrorInfo drawRectangle(int left, int top, int right, int bottom, uint32_t color, bool full) { return {}; };

	virtual ErrorInfo refresh() { return {}; };

	virtual FrameType type() = 0;		//获取类型

	virtual ErrorInfo clean() = 0;		//清空渲染缓冲区

	virtual set<FrameDataType> supportDataType() = 0;

	//------------以下为音频render专有
	//0-100之间
	virtual uint32_t getVolume() { return 0; }		//获取音量
	virtual ErrorInfo setVolume(uint32_t volume) {	return ErrorInfo(Error_Code_NotSupport);}	//设置音量

	virtual bool mute() {return false;}	//是否静音
	virtual ErrorInfo setMute(bool mute) { return ErrorInfo(Error_Code_NotSupport); }	//设置静音

	virtual bool enable(bool en) { return false; }
};

typedef enum
{
	Play_Speed_Normal = 0,		//正常速度,1倍速
	Play_Speed_Fast_2x,			//2倍速，快放
	Play_Speed_Fast_4x,			//4倍速，快放
	Play_Speed_Fast_8x,			//8倍速，快放
	Play_Speed_Fast_16x,		//16倍速，快放
	Play_Speed_Fast_32x,		//32倍速，快放

	Play_Speed_Slow_2x,			//1/2倍速，慢放
	Play_Speed_Slow_4x,			//1/4倍速，慢放
	Play_Speed_Slow_8x,			//1/8倍速，慢放
	Play_Speed_Slow_16x,		//1/16倍速，慢放
	Play_Speed_Slow_32x,		//1/32倍速，慢放
} Play_Speed;

typedef enum
{
	PlayStatus_Init = 0,		//初始化 
	PlayStatus_ConnectSuccess,	//链接成功
	PlayStatus_Play,			//播放状态
	PlayStatus_Pause,			//暂停 
	PlayStatus_FastPlay,		//快放
	PlayStatus_SlowPlay,		//慢放
	PlayStatus_ForwardPlay,		//正放
	PlayStatus_BackPlay,		//倒放
	PlayStatus_Error,			//错误
	PlayStatus_EOF,				//结束

	PlayStatus_Decode_Begin,		// 视频每帧解码开始
	PlayStatus_Decode_End,			// 视频每帧解码结束
	PlayStatus_Rendering_Begin,		// 视频每帧渲染开始
	PlayStatus_Rendering_End,		// 视频每帧渲染结束

	PlayStatus_Resolution,			// 分辨率变化
} PlayStatus;

typedef enum
{
	direction_forward = 0,			// 正向
	direction_backward,				// 反向
} PlayDirect;

typedef enum 
{
	SourceType_Live = 0,		//实时播放
	SourceType_NVRPlayback,		//NVR回放
	SourceType_VOD,				//集中存储回放
	SourceType_File,			//本地回放
	SourceType_Talkback,		//对讲
}SourceType;

typedef enum
{
	StreamType_Video = 0,		// 视频
	StreamType_Audio,			// 音频
} StreamType;

//播放状态回调
typedef Function<void(PlayStatus status, const ErrorInfo& errinfo)> StatusCallback;

//视频源定义,主动获取方式
class MSPLAYER_API ISource
{
public:
	struct AudioInfo
	{
		int nChannels;
		int nSampleRate;
		int nSampleBit;
	};

	struct VideoInfo
	{

	};
public:
	ISource(){}
	virtual ~ISource() {}

	virtual ErrorInfo startTime(Time& st) = 0;
	virtual ErrorInfo endTime(Time& et) = 0;
	virtual ErrorInfo start() = 0;		//开始
	virtual ErrorInfo stop() = 0;		//停止
	virtual ErrorInfo registerStatusCallback(const StatusCallback& statuscallback) = 0;

	virtual ErrorInfo pause() = 0;		//暂停
	virtual ErrorInfo resume() = 0;		//恢复

	virtual Play_Speed playSpeed() = 0;	//获取播放速度
	virtual ErrorInfo setPlaySpeed(Play_Speed speed) = 0;	//设置播放速度

	virtual std::set<StreamType> streamTypes() = 0;

	virtual ErrorInfo seek(uint64_t seekTime) = 0;				// 相对时间
	virtual ErrorInfo seekUTCTime(uint64_t utctime) { return {}; };		// utc时间戳

	virtual PlayStatus status() = 0;	//获取播放状态

	virtual PlayDirect getDirection() = 0;

	virtual void setDirection(PlayDirect direction) = 0;

	virtual ErrorInfo read(FrameType& frametype, CodeID& codeid, shared_ptr<RTPFrame>& frame, uint64_t& timestmap, uint64_t &utctime, uint32_t timeout_ms) = 0; //读取数据

	virtual ErrorInfo write(FrameType frametype, CodeID codeid, std::shared_ptr<RTPFrame>& frame, uint64_t timestamp, uint32_t timeout_ms) = 0; // 写入数据

	virtual AudioInfo getAuidoInfo() = 0;

	virtual ErrorInfo getVideoCode(CodeID &code) = 0;	// 获取视频编码

	virtual ErrorInfo getAudioCode(CodeID &code) = 0;	// 获取音频编码

	virtual ErrorInfo getResolution(uint32_t &width, uint32_t &height) = 0;	// 获取视频分辨率

	virtual int32_t getVideoDelayTime() = 0;		// 获取视频延迟时间

	// 获取码率
	virtual ErrorInfo getVideoBitRate(uint32_t &bitRate) = 0;

	// 获取码率
	virtual ErrorInfo getAudioBitRate(uint32_t &bitRate) = 0;

	// 获取带宽
	virtual ErrorInfo getBandWidth(uint32_t &bitRate) = 0;

	// 获取接收到的帧率
	virtual ErrorInfo getRecviceFrameRate(uint32_t &frameRate) = 0;

	//清除缓存
	virtual ErrorInfo clean() = 0;

	virtual SourceType type() = 0;

	virtual uint32_t getCacheSize() = 0;

	virtual ErrorInfo getTotalTime(uint32_t &totalTime) = 0;
};

}
}