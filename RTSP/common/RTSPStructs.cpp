//
//  Copyright (c)1998-2012, Public Technology
//  All Rights Reserved.
//
//	Description:
//	$Id: File.h 252 2013-12-18 04:40:28Z  $
//

#include "RTSP/RTSPStructs.h"
#include "RTSP/RTSPStatistics.h"
#include "RTSP/RTPFrame.h"

namespace Public {
namespace RTSP {

 SDPMediaInfo::SDPMediaInfo() :payLoad(0), sampRate(0), bandwidth(0)/*,nTrackID(0)*/,
	frametype(FrameType_Unknown), codeId(CodeID_Unknown), profile_level_id(0) {}

 void SDPMediaInfo::parseMediaCodeInfo()
{
	if (String::strcasecmp(mediaName.c_str(), "video") == 0) frametype = FrameType_Video;
	else if (String::strcasecmp(mediaName.c_str(), "audio") == 0 || String::strcasecmp(mediaName.c_str(), "talkback") == 0) frametype = FrameType_Audio;

	if (String::indexOfByCase(codecName, "jpeg") != (size_t)-1) codeId = CodeID_Video_JPEG;
	else if (String::indexOfByCase(codecName, "mpeg4") != (size_t)-1) codeId = CodeID_Video_MPEG4;
	else if (String::indexOfByCase(codecName, "h264") != (size_t)-1) codeId = CodeID_Video_H264;
	else if (String::indexOfByCase(codecName, "h265") != (size_t)-1) codeId = CodeID_Video_H265;
	else if (String::indexOfByCase(codecName, "h263") != (size_t)-1) codeId = CodeID_Video_H263;
	else if (String::indexOfByCase(codecName, "aac") != (size_t)-1) codeId = CodeID_Audio_AAC;
	else if (String::indexOfByCase(codecName, "pcmu") != (size_t)-1) codeId = CodeID_Audio_G711Mu;
	else if (String::indexOfByCase(codecName, "pcma") != (size_t)-1) codeId = CodeID_Audio_G711A;
	else if (String::indexOfByCase(codecName, "pcm") != (size_t)-1) codeId = CodeID_Audio_PCM;
	else if (String::indexOfByCase(codecName, "adpcm") != (size_t)-1) codeId = CodeID_Audio_ADPCM;
	else if (String::indexOfByCase(codecName, "g711a") != (size_t)-1) codeId = CodeID_Audio_G711A;
	else if (String::indexOfByCase(codecName, "g711mu") != (size_t)-1) codeId = CodeID_Audio_G711Mu;
	else if (String::indexOfByCase(codecName, "g722") != (size_t)-1) codeId = CodeID_Audio_G722;
	else if (String::indexOfByCase(codecName, "g726") != (size_t)-1) codeId = CodeID_Audio_G726;
	else if (String::indexOfByCase(codecName, "mp3") != (size_t)-1) codeId = CodeID_Audio_MP3;

	if (codeId >= CodeID_Audio_AAC)
	{
		std::vector<std::string> namearray = String::split(codecName, "-");
		codecName = namearray[0];
		if (namearray.size() >= 2) audio.sampleBits = Value(namearray[1]);
		if (namearray.size() >= 3) audio.channels = Value(namearray[2]);
	}

    if (codeId == CodeID_Unknown)
    {
        if (payLoad == 26)
        {
            codeId = CodeID_Video_JPEG;
        }
    }
}
 void SDPMediaInfo::buildMediaCodeInfo()
{
	if (mediaName.length() <= 0)
	{
		if (frametype == FrameType_Video)
		{
			mediaName = "video";
		}
		else if (frametype == FrameType_Audio)
		{
			mediaName = "audio";
		}
	}

	if (codecName.length() <= 0)
	{
		if (codeId == CodeID_Video_H263) codecName = "H263";
		else if (codeId == CodeID_Video_H264) codecName = "H264";
		else if (codeId == CodeID_Video_H265) codecName = "H265";
		else if (codeId == CodeID_Video_MPEG4) codecName = "mpeg4";
		else if (codeId == CodeID_Video_JPEG) codecName = "JPEG";
		else if (codeId == CodeID_Audio_AAC) codecName = "AAC";
		else if (codeId == CodeID_Audio_PCM) codecName = "PCM";
		else if (codeId == CodeID_Audio_G711Mu) codecName = "PCMU";
		else if (codeId == CodeID_Audio_G711A) codecName = "PCMA";
		else if (codeId == CodeID_Audio_ADPCM) codecName = "adpcm";
		else if (codeId == CodeID_Audio_MP3) codecName = "mp3";
		else if (codeId == CodeID_Audio_G726) codecName = "g726";
		else if (codeId == CodeID_Audio_G722) codecName = "g722";

		/*if (codeId >= CodeID_Audio_AAC)
		{
			if (audio.sampleBits != 0)
			{
				codecName = codecName + "-" + Value(audio.sampleBits);
				if (audio.channels != 0)
				{
					codecName = codecName + "-" + Value(audio.channels);
				}
			}
		}*/
	}

	if (sampRate == 0)
	{
		if (codeId == CodeID_Video_H263) sampRate = 90000;
		else if (codeId == CodeID_Video_H264) sampRate = 90000;
		else if (codeId == CodeID_Video_H265) sampRate = 90000;
		else if (codeId == CodeID_Video_MPEG4) sampRate = 90000;
		else if (codeId == CodeID_Video_JPEG) sampRate = 90000;
		else if (codeId == CodeID_Audio_AAC) sampRate = 8000;
		else if (codeId == CodeID_Audio_PCM) sampRate = 8000;
		else if (codeId == CodeID_Audio_G711Mu) sampRate = 8000;
		else if (codeId == CodeID_Audio_G711A) sampRate = 8000;
		else if (codeId == CodeID_Audio_G726) sampRate = 8000;
		else if (codeId == CodeID_Audio_ADPCM) sampRate = 8000;
		else if (codeId == CodeID_Audio_MP3) sampRate = 8000;
	}

	if (payLoad == 0)
	{
		//payload = 0  == pcmu
		if (codeId == CodeID_Video_H263) payLoad = 34;
		else if (codeId == CodeID_Video_H264) payLoad = 96;
		else if (codeId == CodeID_Video_H265) payLoad = 97;
		else if (codeId == CodeID_Video_MPEG4) payLoad = 98;
		else if (codeId == CodeID_Video_JPEG) payLoad = 99;
		else if (codeId == CodeID_Audio_AAC) payLoad = 26;
		else if (codeId == CodeID_Audio_G711Mu) payLoad = 0;
		else if (codeId == CodeID_Audio_PCM) payLoad = 19;
		else if (codeId == CodeID_Audio_G711A) payLoad = 8;
		else if (codeId == CodeID_Audio_ADPCM) payLoad = 9;
		else if (codeId == CodeID_Audio_MP3) payLoad = 21;
		else if (codeId == CodeID_Audio_G726) payLoad = 24;
		else if (codeId == CodeID_Audio_G722) payLoad = 25;
	}
}

 RTSP_Media_Infos::RTSP_Media_Infos() :startRange(0), stopRange(0) {}

 RTSP_Media_Infos RTSP_Media_Infos::cloneStreamInfo() const
{
	RTSP_Media_Infos info;

	info.ssrc = ssrc;

	for (std::list<shared_ptr<STREAM_TRANS_INFO> >::const_iterator iter = infos.begin(); iter != infos.end(); iter++)
	{
		shared_ptr<STREAM_TRANS_INFO> streaminfo = make_shared<STREAM_TRANS_INFO>();

		streaminfo->streaminfo = (*iter)->streaminfo;
		streaminfo->transportinfo.ssrc = (*iter)->transportinfo.ssrc;

		info.infos.push_back(streaminfo);
	}

	return info;
}

 const shared_ptr<STREAM_TRANS_INFO> RTSP_Media_Infos::streamInfo(FrameType frametype)const
{
	for (std::list<shared_ptr<STREAM_TRANS_INFO> >::const_iterator iter = infos.begin(); iter != infos.end(); iter++)
	{
		if ((*iter)->streaminfo.frametype == FrameType_Audio && frametype == FrameType_Audio)
		{
			return *iter;
		}
		else if ((*iter)->streaminfo.frametype == FrameType_Video && frametype != FrameType_Audio)
		{
			return *iter;
		}
	}

	return NULL;
}

 const shared_ptr<STREAM_TRANS_INFO> RTSP_Media_Infos::videoStreamInfo()const
{
	return streamInfo(FrameType_Video);
}
 const shared_ptr<STREAM_TRANS_INFO> RTSP_Media_Infos::audioStreamInfo()const
{
	return streamInfo(FrameType_Audio);
}
 const shared_ptr<STREAM_TRANS_INFO> RTSP_Media_Infos::talkbackStreamInfo() const
{
     for (std::list<shared_ptr<STREAM_TRANS_INFO> >::const_iterator iter = infos.begin(); iter != infos.end(); iter++)
     {
         if ((*iter)->streaminfo.mediaName == "talkback" && (*iter)->streaminfo.frametype == FrameType_Audio)
         {
             return *iter;
         }
     }

     return shared_ptr<STREAM_TRANS_INFO>();
}
 shared_ptr<STREAM_TRANS_INFO> RTSP_Media_Infos::addVideoStreamInfo()
{
	shared_ptr<STREAM_TRANS_INFO> videoinfo = addStreamInfo("video");

	{
		videoinfo->streaminfo.payLoad = 0;
		videoinfo->streaminfo.sampRate = 0;
		videoinfo->streaminfo.frametype = FrameType_Video;
	}

	return videoinfo;
}

 shared_ptr<STREAM_TRANS_INFO> RTSP_Media_Infos::addAudioStreamInfo()
{
	shared_ptr<STREAM_TRANS_INFO> audioinfo = addStreamInfo("audio");

	{
        audioinfo->streaminfo.payLoad = 0;
        audioinfo->streaminfo.sampRate = 0;
        audioinfo->streaminfo.frametype = FrameType_Audio;
	}

	return audioinfo;
}

 shared_ptr<STREAM_TRANS_INFO> RTSP_Media_Infos::addTalkbackStreamInfo(CodeID codeid, int sampRate, int sampBit, int channels)
 {
     shared_ptr<STREAM_TRANS_INFO> talbackinfo = addStreamInfo("talkback");
     
     {
         talbackinfo->streaminfo.audio.channels = channels;
         talbackinfo->streaminfo.audio.sampleBits = sampBit;
         talbackinfo->streaminfo.payLoad = 0;
         talbackinfo->streaminfo.sampRate = 0;
         talbackinfo->streaminfo.codeId = codeid;
         talbackinfo->streaminfo.frametype = FrameType_Audio;
         talbackinfo->streaminfo.buildMediaCodeInfo();
     }

     return talbackinfo;
 }

 shared_ptr<STREAM_TRANS_INFO> RTSP_Media_Infos::addStreamInfo(const std::string& flag)
{
	shared_ptr<STREAM_TRANS_INFO> info = make_shared<STREAM_TRANS_INFO>();

	info->streaminfo.trackID = "trackID=" + Value(infos.size()).readString();
	info->streaminfo.mediaName = flag;

	infos.push_back(info);



	return info;
}
//是否有视频流
 bool RTSP_Media_Infos::hasVideo() const
{
	return streamInfo(FrameType_Video) != NULL;
}
//是否有音频流
 bool RTSP_Media_Infos::hasAudio() const
{
	return streamInfo(FrameType_Audio) != NULL;
}

//清除非音视频的流信息
 void RTSP_Media_Infos::cleanExStreamInfo()
{
	for (std::list<shared_ptr<STREAM_TRANS_INFO> >::const_iterator iter = infos.begin(); iter != infos.end(); )
	{
		if (String::strcasecmp((*iter)->streaminfo.mediaName.c_str(), "video") == 0 || String::strcasecmp((*iter)->streaminfo.mediaName.c_str(), "audio") == 0
            || String::strcasecmp((*iter)->streaminfo.mediaName.c_str(), "talkback") == 0 )
		{
			iter++;
		}
		else
		{
			infos.erase(iter++);
		}
	}
}
 PlayParam PlayInfo::speed() const
 {
	 std::map<PlayCmd, PlayParam>::const_iterator iter = params.find(PlayCmd_SPEED);
	 if (iter != params.end()) return iter->second;

	 return PlayParam();
 }
 PlayParam PlayInfo::range() const
 {
	 {
		 std::map<PlayCmd, PlayParam>::const_iterator iter = params.find(PlayCmd_RAND_CLOCK);
		 if (iter != params.end()) return iter->second;
	 }

	 {
		 std::map<PlayCmd, PlayParam>::const_iterator iter = params.find(PlayCmd_RAND_NPT);
		 if (iter != params.end()) return iter->second;
	 }
	
	 
	 return PlayParam();
 }

 bool PlayInfo::haveSpeed() const
 {
	 return params.find(PlayCmd_SPEED) != params.end();
 }
 bool PlayInfo::haveRang() const
 {
	 return haveRangNpt() || haveRangClock();
 }

 bool PlayInfo::haveRangNpt() const
 {
	 return params.find(PlayCmd_RAND_NPT) != params.end();
 }
 bool PlayInfo::haveRangClock() const
 {
	 return params.find(PlayCmd_RAND_CLOCK) != params.end();;
 }

 void PlayInfo::addSpeed(double speed)
 {
	 PlayParam param;
	 param.cmd = PlayCmd_SPEED;
	 param.data.speed = speed;

	 params[param.cmd] = param;
 }
 void PlayInfo::addRangNpt(uint64_t start, uint64_t stop)
 {
	 params.erase(PlayCmd_RAND_CLOCK);

	 PlayParam param;
	 param.cmd = PlayCmd_RAND_NPT;
	 param.data.from = start;
	 param.data.to = stop;

	 params[param.cmd] = param;

 }
 void PlayInfo::addRangClock(uint64_t start, uint64_t stop)
 {
	 params.erase(PlayCmd_RAND_NPT);

	 PlayParam param;
	 param.cmd = PlayCmd_RAND_CLOCK;
	 param.data.from = start;
	 param.data.to = stop;

	 params[param.cmd] = param;
 }

 bool PlayInfo::parseSpeed(const std::string& str)
 {
	 if (str.empty())
	 {
		 return false;
	 }

	 if (str.length() <= 0) return false;

	 PlayParam param;
	 param.cmd = PlayCmd_SPEED;
	 param.data.speed = Value(str);

	 params[param.cmd] = param;

	 return true;
 }

 std::string PlayInfo::buildSpeed() const
 {
	 std::map<PlayCmd, PlayParam>::const_iterator iter = params.find(PlayCmd_SPEED);
	 if (iter == params.end()) return "";

	 return Value(iter->second.data.speed).readString();
 }

 std::string PlayInfo::buildRang() const
 {
	 {
		 std::map<PlayCmd, PlayParam>::const_iterator iter = params.find(PlayCmd_RAND_NPT);
		 if (iter != params.end())
		 {
			 std::string playstr;
			 playstr = "npt=";

			 playstr += Value(iter->second.data.from).readString() + "-";

			 if (iter->second.data.to != 0)
			 {
				 playstr += Value(iter->second.data.to).readString();
			 }

			 return playstr;
		 }
	 }
	 {
		 std::map<PlayCmd, PlayParam>::const_iterator iter = params.find(PlayCmd_RAND_CLOCK);
		 if (iter != params.end())
		 {
			 std::string playstr;
			 playstr = "clock=";

			 playstr += Value(iter->second.data.from).readString() + "-";

			 if (iter->second.data.to != 0)
			 {
				 playstr += Value(iter->second.data.to).readString();
			 }

			 return playstr;
		 }
	 }
	
	 return "";
 }
 bool PlayInfo::parseRang(const std::string& str)
 {
	 std::vector<std::string> cmdarray = String::split(str, "=");
	 if (cmdarray.size() <= 0) return false;

	 PlayParam param;

	 if (String::iequals(cmdarray[0], "npt"))
		 param.cmd = PlayCmd_RAND_NPT;
	 else if (String::iequals(cmdarray[0], "clock"))
		 param.cmd = PlayCmd_RAND_CLOCK;
	 else
		 return false;

	 if (cmdarray.size() >= 2)
	 {
		 std::vector<std::string> varray = String::split(cmdarray[1], "-");
		 if (varray.size() >= 1) param.data.from = Value(varray[0]);
		 if (varray.size() >= 2) param.data.to = Value(varray[1]);
	 }

	 params[param.cmd] = param;

	 return true;
 }

 bool RtspParameter::addParameter(const std::string& key, const Value& value)
 {
	 parameterMap[key] = value.readString();

	 return true;
 }

 bool RtspParameter::parseParameter(const std::string body)
 {
	 std::vector<std::string> vec = String::split(body, ";");
	 for (uint32_t i = 0; i < vec.size(); i++)
	 {
		 std::vector<std::string> tmpVec = String::split(vec[i], "=");
		 if (tmpVec.size() != 2)
		 {
			 return false;
		 }

		 for (uint32_t j = 0; j < tmpVec.size(); j++)
		 {
			 parameterMap[tmpVec[0]] = tmpVec[1];
		 }
	 }

	 return true;
 }

 std::string RtspParameter::toString()
 {
	 std::string str;
	 for (std::map<std::string, std::string>::iterator iter = parameterMap.begin(); iter != parameterMap.end(); iter++)
	 {
		 str += iter->first + "=" + iter->second + ";";
	 }

	 return str;
 }

}
}
