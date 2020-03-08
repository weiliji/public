#pragma once
#include "Base/Base.h"
#include "Defs.h"
#include "iPlayer.h"
using namespace Public::Base;

#ifdef WIN32
#ifdef MSPLAYER_EXPORTS
#define MSPLAYER_API __declspec(dllexport)
#else
#define MSPLAYER_API __declspec(dllimport)
#endif
#else
#define MSPLAYER_API
#endif

namespace Milesight {
namespace Player {

class MSPLAYER_API MSPlayer
{
public:
	// url 可以是本地媒体文件；rtsp流
	// window 窗口句柄，如果为QTOPenglES环境则传入NULL
	MSPlayer(const std::string& url, uint64_t window);

	MSPlayer(const std::shared_ptr<ISource> &source, uint64_t window);

	virtual ~MSPlayer();

	// 初始化
	static ErrorInfo init();

	// 反初始化
	static ErrorInfo uninit();

	// 开始播放
	ErrorInfo play();

	// 停止播放
	ErrorInfo stop();

	ErrorInfo clear(int x, int y, int w, int h, uint32_t color);

	// 如果QT调用该库并且使用opengles渲染，则需要由QT线程调用该接口；非QT无需调用
	ErrorInfo refresh(int x = 0, int y = 0, int w = 0, int h = 0);

	// 定位播放，相对时间，单位秒
	ErrorInfo seek(uint64_t playTime);

	// 定位播放，绝对时间，单位秒
	ErrorInfo seekUTCTime(uint64_t playTime);

	// 暂停播放
	ErrorInfo pause();

	// 恢复播放，同时也恢复到1倍速
	ErrorInfo resume();

	// 单帧播放
	ErrorInfo step();

	// 抓图,保存当前图片
	ErrorInfo snap(const std::string& filename, ImageType imgType);

	// 获取每帧渲染的时间
	ErrorInfo getFrameRendeTime(uint32_t &frameTime);

	// 获取已播放时间
	ErrorInfo getPlayedTime(uint64_t &playedTime);

	// 设置播放速度
	ErrorInfo setPlaySpeed(Play_Speed speed);

	// 获取当前播放速度
	ErrorInfo getPlaySpeed(Play_Speed &speed);

	// 设置播放方向，倒放、正放
	ErrorInfo setPlayDirect(PlayDirect eDirect);

	// 获取当前播放方向
	ErrorInfo getPlayDirect(PlayDirect &eDirect);

	// 获取当前播放状态，多种状态可同时存在
	ErrorInfo getPlayStatus(std::set<PlayStatus> &status);

	// 注册播放状态回调
	ErrorInfo registerPlayStatusCallback(const StatusCallback &callback);

	// 反注册播放状态回调
	ErrorInfo unRegisterPlayStatusCallback(const StatusCallback &callback);

	// 启动本地录像
	ErrorInfo startLocalRecord(const std::string& filename);

	// 停止本地录像
	ErrorInfo stopLocalRecord();

	// 获取已录像的时间，单位秒
	ErrorInfo getRecordTime(uint32_t &recordTime);

	// 获取本地文件的总时间，单位秒
	ErrorInfo getTotalTime(uint32_t &time);

	// 获取音量大小，取值范围0 - 100
	ErrorInfo getVolume(uint32_t &volume);

	// 设置音量大小，取值范围0 - 100
	ErrorInfo setVolume(uint32_t volume);

	// 使能音频
	ErrorInfo enableVolume(bool enable);

	// 开始电子放大
	ErrorInfo startDigital();

	// 设置电子放大的缩放倍率
	// @param zoomRate 缩放倍率取值范围(0.1~1.0); 1.0为原始画面
	ErrorInfo setDigitalPTZZoom(float zoomRate);

	// 设置电子放大的中心原点
	// @param xPos 相对于窗口(视口)尺寸的横坐标
	// @param yPos 相对于窗口(视口)尺寸的纵坐标
	ErrorInfo setDigitalPTZOrigin(uint32_t xPos, uint32_t yPos);

	// 获取电子放大的缩放倍率，取值范围 0.1 - 1.0
	// @param zoomRate 缩放倍率获取
	ErrorInfo getDigitalPTZZoom(float &zoomRate);

	// 获取电子放大的中心原点
	// @param xPos 获取当前电子放大的横坐标
	// @param yPos 获取当前电子放大的纵坐标
	ErrorInfo getDigitalPTZOrigin(uint32_t &xPos, uint32_t &yPos);

	// 停止电子放大
	ErrorInfo stopDigital();

	// 添加图片水印
	// @param filename 图片文件名，支持PNG透明
	// @param x 相对于图像的横坐标
	// @param y 相对于图像的纵坐标
	ErrorInfo addPictureMark(const std::string& filename, int x, int y);

	// 添加文字水印
	// @param txt 叠加内容
	// @param fontFile 字体文件的名称，可以为空，默认使用微软雅黑
	// @param fontSize 字体大小
	// @param color 文字颜色，RGBA格式
	// @param x 相对于图像的横坐标
	// @param y 相对于图像的纵坐标
	ErrorInfo addTxtMark(const std::string& txt, const std::string& fontFile, uint32_t fontSize, uint32_t color, uint32_t x, uint32_t y);

	// 添加矩形框水印
	// @param x 相对于图像的横坐标
	// @param y 相对于图像的纵坐标
	// @param width 相对于图像的宽度
	// @param height 相对于图像的高度
	// @param borderSize -1 表示填充矩形
	// @param color 矩形框颜色，RGBA格式
	ErrorInfo addRectangleMark(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t borderSize, uint32_t color);

	// 获取视频帧率
	ErrorInfo getFrameRate(uint32_t &frameRate);

	// 获取当前码流类型集合
	ErrorInfo getStreamTypes(std::set<StreamType>& streamTypes);

	// 获取视频编码类型
	// @param codeid 视频编码类型
	ErrorInfo getVideoCode(CodeID &codeid);

	// 获取音频编码类型
	// @param codeid 音频编码类型
	ErrorInfo getAudioCode(CodeID &codeid);

	// 获取视频分辨率
	// @param width 宽度
	// @param height 高度
	ErrorInfo getResolution(uint32_t &width, uint32_t &height);

	// 获取视频码率
	// @param bitRate 视频码率大小
	ErrorInfo getVideoBitRate(uint32_t &bitRate);

	// 获取单位时间内的带宽大小，本地文件播放无意义
	// @param bitRate 获取接收到的网络带宽大小，单位kbps
	ErrorInfo getBandWidth(uint32_t& bitRate);

	// 获取单位时间内接收的视频帧数，本地文件播放无意义
	// @param frameRate 单位时间内接收到的帧数
	ErrorInfo getRecviceFrameRate(uint32_t &frameRate);
private:
	struct MSPlayerInternal;
	MSPlayerInternal *internal;

};
}
}