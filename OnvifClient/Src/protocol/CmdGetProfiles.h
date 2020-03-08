#ifndef __ONVIFPROTOCOL_PROFILES_H__
#define __ONVIFPROTOCOL_PROFILES_H__
#include "CmdObject.h"


class CMDGetProfiles :public CmdObject
{
public:
	CMDGetProfiles():CmdObject(URL_ONVIF_MEDIA)
	{
	}
	virtual ~CMDGetProfiles() {}

	std::string token;
	virtual std::string build(const URL& URL)
	{
		XML::Child& getprofiles = body().addChild("GetProfiles");

		getprofiles.addAttribute("xmlns", "http://www.onvif.org/ver10/media/wsdl");

		return CmdObject::build(URL);
	}
	OnvifClientDefs::Profiles profileInfo;
	virtual bool parse(const XML::Child& body)
	{
		XML::Child resp = body.getChild("GetProfilesResponse");
		if (!resp) return false;
		
		for(XML::ChildIterator iter = resp.child("Profiles");iter;iter++)
		{
			const XML::Child& p_profiles = *iter;
			OnvifClientDefs::ProfileInfo profile;

			profile.fixed = p_profiles.getAttribute("fixed").readBool();
			profile.token = p_profiles.getAttribute("token").readString();

			if (parseProfileInfo(p_profiles, profile))
			{
				profileInfo.infos.push_back(profile);
			}
		}

		return true;
	}
	virtual bool parseProfileInfo(const XML::Child& p_profiles,OnvifClientDefs::ProfileInfo& info)
	{
		OnvifClientDefs::ProfileInfo* profileInfo = &info;

		const XML::Child& pname = p_profiles.getChild("Name");
		if (pname)
		{
			profileInfo->name = pname.data().readString();
		}
		else
		{
			return false;
		}

		const XML::Child& videosrc = p_profiles.getChild("VideoSourceConfiguration");
		if (videosrc)
		{
			if (!parseVideoSource(videosrc,info))
			{
				profileInfo->videoSource = NULL;
			}
		}

		const XML::Child& videoenc = p_profiles.getChild("VideoEncoderConfiguration");
		if (videoenc)
		{
			if (!parseVideoEncoder(videoenc, info))
			{
				profileInfo->videoEncoder = NULL;
			}
		}

		const XML::Child& audiosrc = p_profiles.getChild("AudioSourceConfiguration");
		if (audiosrc)
		{
			if (!parseAudioSource(audiosrc, info))
			{
				profileInfo->videoEncoder = NULL;
			}
		}

		const XML::Child& audioenc = p_profiles.getChild("AudioEncoderConfiguration");
		if (audioenc)
		{
			if (!parseAudioEncoder(audioenc, info))
			{
				profileInfo->videoEncoder = NULL;
			}
		}
		const XML::Child& ptzcfg = p_profiles.getChild("PTZConfiguration");
		if (ptzcfg)
		{
			if (!parsePTZCfg(ptzcfg, info))
			{
				profileInfo->ptzConfig = NULL;
			}
		}

		return true;
	}
private:
	bool parseVideoSource(const XML::Child& videosrc, OnvifClientDefs::ProfileInfo& info)
	{
		info.videoSource = make_shared<OnvifClientDefs::_VideoSource>();

		info.videoSource->token = videosrc.getAttribute("token").readString();
		info.videoSource->stream_name = videosrc.getChild("Name").data().readString();
		info.videoSource->use_count = videosrc.getChild("UseCount").data().readInt();
		info.videoSource->source_token = videosrc.getChild("SourceToken").data().readString();

		if (info.videoSource->stream_name == "" || info.videoSource->source_token == "") return false;


		const XML::Child& bounds = videosrc.getChild("Bounds");
		if (!bounds) return false;

		info.videoSource->height = bounds.getAttribute("height").readInt();
		info.videoSource->width = bounds.getAttribute("width").readInt();
		info.videoSource->x = bounds.getAttribute("x").readInt();
		info.videoSource->y = bounds.getAttribute("y").readInt();

		return true;
	}

	bool parseVideoEncoder(const XML::Child& videoenc, OnvifClientDefs::ProfileInfo& info)
	{
		info.videoEncoder = make_shared<OnvifClientDefs::_VideoEncoder>();

		info.videoEncoder->token = videoenc.getAttribute("token").readString();
		info.videoEncoder->name = videoenc.getChild("token");
		
		if (info.videoEncoder->name == "" || info.videoEncoder->token == "") return false;

		info.videoEncoder->use_count = videoenc.getChild("UseCount").data().readInt();
		info.videoEncoder->encoding = onvif_parse_encoding(videoenc.getChild("Encoding").data());

		info.videoEncoder->width = videoenc.getChild("Resolution").getChild("Width").data().readInt();
		info.videoEncoder->height = videoenc.getChild("Resolution").getChild("Height").data().readInt();
		info.videoEncoder->quality = videoenc.getChild("Quality").data().readFloat();
		
		info.videoEncoder->framerate_limit = videoenc.getChild("RateControl").getChild("FrameRateLimit").data().readInt();
		info.videoEncoder->encoding_interval = videoenc.getChild("RateControl").getChild("EncodingInterval").data().readInt();
		info.videoEncoder->bitrate_limit = videoenc.getChild("RateControl").getChild("BitrateLimit").data().readInt();


		if (info.videoEncoder->encoding == CodeID_Video_H264)
		{
			info.videoEncoder->gov_len = videoenc.getChild("H264").getChild("GovLength").data().readInt();
			info.videoEncoder->h264_profile = onvif_parse_h264_profile(videoenc.getChild("H264").getChild("H264Profile").data());
		}

		info.videoEncoder->session_timeout = videoenc.getChild("SessionTimeout").data().readInt();

		return true;
	}

	bool parseAudioSource(const XML::Child& audiosrc, OnvifClientDefs::ProfileInfo& info)
	{
		info.audioSource = make_shared<OnvifClientDefs::_AudioSource>();

		info.audioSource->token = audiosrc.getAttribute("token").readString();
		info.audioSource->stream_name = audiosrc.getChild("Name").data().readString();
		info.audioSource->use_count = audiosrc.getChild("UseCount").data().readInt();
		info.audioSource->source_token = audiosrc.getChild("SourceToken").data().readString();

		if (info.audioSource->stream_name == "" || info.audioSource->source_token == "") return false;

		return true;
	}

	bool parseAudioEncoder(const XML::Child& audioenc, OnvifClientDefs::ProfileInfo& info)
	{
		info.audioEncoder = make_shared<OnvifClientDefs::_AudioEncoder>();

		info.audioEncoder->token = audioenc.getAttribute("token").readString();
		info.audioEncoder->name = audioenc.getChild("token");

		if (info.audioEncoder->name == "" || info.audioEncoder->token == "") return false;

		info.audioEncoder->use_count = audioenc.getChild("UseCount").data().readInt();
		info.audioEncoder->encoding = onvif_parse_encoding(audioenc.getChild("Encoding").data());

		info.audioEncoder->bitrate = audioenc.getChild("Bitrate").data().readInt();
		info.audioEncoder->sample_rate = audioenc.getChild("SampleRate").data().readInt();
	
		info.audioEncoder->session_timeout = audioenc.getChild("SessionTimeout").data().readInt();

		return true;
	}

	bool parsePTZCfg(const XML::Child& ptzcfg, OnvifClientDefs::ProfileInfo& info)
	{
		info.ptzConfig = make_shared<OnvifClientDefs::PTZConfig>();

		info.ptzConfig->token = ptzcfg.getAttribute("token").readString();;
		info.ptzConfig->name = ptzcfg.getChild("Name").data().readString();

		if (info.ptzConfig->token == "" || info.ptzConfig->name == "") return false;

		info.ptzConfig->use_count = ptzcfg.getChild("UseCount").data().readInt();
		info.ptzConfig->nodeToken = ptzcfg.getChild("NodeToken").data().readString();

		info.ptzConfig->def_speed.pan_tilt_x = ptzcfg.getChild("DefaultPTZSpeed").getChild("PanTilt").getAttribute("x").readInt();
		info.ptzConfig->def_speed.pan_tilt_y = ptzcfg.getChild("DefaultPTZSpeed").getChild("PanTilt").getAttribute("y").readInt();
		
		info.ptzConfig->def_speed.zoom = ptzcfg.getChild("DefaultPTZSpeed").getChild("Zoom").getAttribute("x").readInt();

		info.ptzConfig->def_timeout = ptzcfg.getChild("DefaultPTZTimeout").data().readInt();

		return true;
	}

};



#endif //__ONVIFPROTOCOL_H__
