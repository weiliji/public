//
//  Copyright (c)1998-2012, Public Technology
//  All Rights Reserved.
//
//	Description:
//	$Id: Md5.h 3 2013-01-21 06:57:38Z  $
//

#ifndef __BASE_MEDIACODE_H__
#define __BASE_MEDIACODE_H__

#include <stddef.h>
#include "Base/IntTypes.h"
#include "Defs.h"

//在这定义了视频相关的流媒体信息

namespace Public {
namespace Base {


//帧数据类型,值需要小于255
typedef enum
{
	FrameDataType_Unknown = 0,
	FrameDataType_YUV420 = 1,	//YUV 420
	FrameDataType_RGB24,
	FrameDataType_RGB32,
	FrameDataType_YV12 ,		//YV12
	FrameDataType_NV12,			//NV12
	FrameDataType_DXVA2_VLD,	//DAVA2
	FrameDataType_CUDA,
	FrameDataType_PCM,			//PCM
}FrameDataType;

//帧类型/I/P/B/VUDIO/AUDIO,值需要小于255
typedef enum
{
	FrameType_Unknown = 0,
	FrameType_Video_IFrame = 1,		// 包含SPS、PPS、SEI
	FrameType_Video_PFrame,
	FrameType_Video_BFrame,
	FrameType_Video_SPS,
	FrameType_Video_PPS,
	FrameType_Video_SEI,
	FrameType_Video_IDR,
	FrameType_Video,
	FrameType_Audio,
}FrameType;

//音视频编码方式,值需要小于255
typedef enum {
	CodeID_Unknown = 0,
	CodeID_Video_H264 = 1,			//H264类型
	CodeID_Video_H265,
	CodeID_Video_MPEG4,
	CodeID_Video_JPEG,
	CodeID_Video_H263,

	CodeID_Audio_AAC = 100,
	CodeID_Audio_G726,
	CodeID_Audio_G722,
	CodeID_Audio_G711A,
	CodeID_Audio_G711Mu,
	CodeID_Audio_MP3,
	CodeID_Audio_ADPCM,
	CodeID_Audio_PCM,
}CodeID; //编码类型

}
}

#endif