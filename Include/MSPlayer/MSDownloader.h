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
	
class MSPLAYER_API MSDownloader
{
public:
	// url 可以是本地媒体文件、rtsp地址
	// saveFileName 保存的本地文件
	MSDownloader(const std::string& url, const std::string& saveFileName);

	MSDownloader(const std::shared_ptr<ISource> &source, const std::string& saveFileName);

	virtual ~MSDownloader();

	// 初始化
	static ErrorInfo init();

	// 反初始化
	static ErrorInfo uninit();

	// 开始下载
	ErrorInfo start();

	// 停止下载
	ErrorInfo stop();

	// 获取已经下载录像的相对时间
	ErrorInfo getDownloadTime(uint32_t &downloadTime);

	// 设置下载速度
	ErrorInfo setDownloadSpeed(Play_Speed speed);

	// 获取当前下载速度
	ErrorInfo getDownloadSpeed(Play_Speed &speed);

	// 是否使能视频
	ErrorInfo enableVideo(bool enable);

	// 是否使能音频
	ErrorInfo enableAudio(bool enable);

	// 设置下载状态回调
	ErrorInfo setDownloadStatusCallback(const StatusCallback &callback);

	// 获取视频编码类型
	// @param codeid 视频编码类型
	ErrorInfo getVideoCode(CodeID &codeid);

	// 获取音频编码类型
	// @param codeid 音频编码类型
	ErrorInfo getAudioCode(CodeID &codeid);

	// 获取单位时间内的带宽大小，本地文件无意义
	// @param bitRate 获取接收到的网络带宽大小，单位kbps
	ErrorInfo getBandWidth(uint32_t& bitRate);

	// 获取单位时间内接收的视频帧数，本地文件无意义
	// @param frameRate 单位时间内接收到的帧数
	ErrorInfo getRecviceFrameRate(uint32_t &frameRate);

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

	// 添加时间水印
	ErrorInfo addTimeMark(const std::string& format, const std::string& fontFile, uint32_t fontSize, uint32_t color, uint32_t x, uint32_t y);
private:
	struct MSDownloadInternal;
	MSDownloadInternal *internal;
};
}
}