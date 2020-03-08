#pragma  once
#include "MSPlayer/iPlayer.h"
#include "RTSP/RTSP.h"
using namespace Public::RTSP;

namespace Milesight {
namespace Player {

class MSPLAYER_API Source_RTSP :public ISource
{
public:
	Source_RTSP(const std::string& rtspaddr);
	virtual ~Source_RTSP();

	virtual ErrorInfo startTime(Time& st);
	virtual ErrorInfo endTime(Time& et);
	virtual ErrorInfo registerStatusCallback(const StatusCallback& statuscallback);
	virtual ErrorInfo start();
	virtual ErrorInfo stop();

	virtual ErrorInfo pause();
	virtual ErrorInfo resume();

	virtual Play_Speed playSpeed();
	virtual ErrorInfo setPlaySpeed(Play_Speed speed);

	virtual std::set<StreamType> streamTypes();

	/*virtual ErrorInfo seek(const Time& time);*/
	virtual ErrorInfo seek(uint64_t seekTime);

	virtual ErrorInfo seekUTCTime(uint64_t utctime);

	virtual PlayStatus status();
	virtual void setDirection(PlayDirect direction);
	virtual PlayDirect getDirection();
	virtual ErrorInfo read(FrameType& frametype, CodeID& codeid, shared_ptr<RTPFrame>& frame, uint64_t& timestmap, uint64_t &utctime, uint32_t timeout_ms);
	virtual ErrorInfo write(FrameType frametype, CodeID codeid, std::shared_ptr<RTPFrame>& frame, uint64_t timestamp, uint32_t timeout_ms);
	virtual AudioInfo getAuidoInfo();
	virtual ErrorInfo getVideoCode(CodeID &code);
	virtual ErrorInfo getAudioCode(CodeID &code);
	virtual ErrorInfo getResolution(uint32_t &width, uint32_t &height);
	virtual ErrorInfo getVideoBitRate(uint32_t &bitRate);
	virtual ErrorInfo getAudioBitRate(uint32_t &bitRate);
	virtual ErrorInfo getBandWidth(uint32_t &bitRate);
	virtual ErrorInfo getRecviceFrameRate(uint32_t &frameRate);
	virtual int32_t getVideoDelayTime();
	virtual ErrorInfo clean();

	virtual SourceType type();

	virtual uint32_t getCacheSize();

	virtual ErrorInfo getTotalTime(uint32_t &totalTime) { return { Error_Code_Fail }; };
private:
	struct Source_RTSPInternal;
	Source_RTSPInternal*internal;
};

}
}