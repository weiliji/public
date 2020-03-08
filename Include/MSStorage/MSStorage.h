#pragma  once
#include "Base/Base.h"
#include "MSMQ/MSMQ.h"
#include "MSProtocol/MSProtocol.h"
using namespace Public::Base;
using namespace Milesight::MQ;
using namespace Milesight::Protocol;

namespace Milesight {
namespace Storage {

#ifdef _MSC_VER

#ifdef MSSTORAGE_DLL_BUILD
#define  MSSTORAGE_API _declspec(dllexport)
#else
#define  MSSTORAGE_API _declspec(dllimport)
#endif
#else
#define MSSTORAGE_API
#endif

struct MSVolumeConfig
{
	bool		enable = false;	//是否启用，启用后才能写
	uint32_t	reserved = 10;	//百分比,预留保留空间
};

struct MSVolumeInfo :public Host::DiskInfo,public MSVolumeConfig
{
	ErrorInfo  status;		//volume状态
};

//可以直接复制，无拷贝
struct MSFrameInfo
{
	CodeID					codeid;			//编码类型
	FrameType				frametype;		//帧类型
	uint32_t				timestmap;		//帧时间戳
	String 					data;
};
struct MSBookMarkInfo
{
	DeviceId				devid;
	uint64_t				startTime;
	uint32_t				timeLong;

	std::string				bookmarkName;
	std::string				bookmarkDescrition;
	std::string				creator;
};

struct MSBookRecordInfo
{
	uint64_t					startTime;
	uint64_t					stopTime;
	uint32_t					mediaSize;
	uint32_t					fileid;
	uint32_t					indexpos;
	MSProtoStreamType			streamtype;
	std::string					volume;
};

//触发方式
enum MSTrigger
{
	MSTrigger_Time = 1 << 1,		//定时
	MSTrigger_Motion = 1 << 2,		//移动侦测报警
	MSTrigger_AlarmIn = 1 << 3,		//报警输入触发报警
};


struct MSCameraConfig
{
	DeviceId		devid;
	uint32_t		duetime;	//过期时间，天
};


enum ReadType {
	ReadType_Forward = 0,	//正向读取，正常播放
	ReadType_Backward = 1,		//反向读取，倒放
};

class FS_Manager;

//录像流对象，用于录像播放，下载等
class MSSTORAGE_API MSRecordStream
{
public:
	MSRecordStream() {}
	virtual ~MSRecordStream() {}

	virtual	ErrorInfo open(uint64_t starttime, uint64_t stoptime) = 0;
	virtual ErrorInfo close() = 0;

	//定位
	virtual ErrorInfo seek(uint64_t time) = 0;
	
	//读取帧数据
	virtual std::vector<shared_ptr<MSFrameInfo>> readGop(ReadType type) = 0;

	//是否已经结束
	virtual bool isEndOfFile()const = 0;

	//录像起始结束时间
	virtual uint64_t startTime() const = 0;
	virtual uint64_t stopTime() const = 0;
};

enum MSStorageEventType
{
	MSStorageEventType_Disk_Ready = 10000,			//准备就绪
	MSStorageEventType_Disk_Offline,				//不在线
	MSStorageEventType_Disk_AccessDenied,			//拒绝访问
	MSStorageEventType_Disk_Full,					//磁盘满

	MSStorageEventType_File_CreateError = 20000,	//创建文件失败
	MSStorageEventType_File_OpenError,				//打开文件失败
	MSStorageEventType_File_WriteError,				//写入文件失败
	MSStorageEventType_File_ReadError,				//读取文件失败

	
};

//这里定义的是存储相关的事件通知
class MSSTORAGE_API MSMSStorageEvent
{
public:
	MSMSStorageEvent() {}
	virtual ~MSMSStorageEvent() {}

	//磁盘文件事件
	virtual void onEvent(MSStorageEventType event,const std::string& name, const std::string& filename,const ErrorInfo& err) = 0;
	
	//存储事件
	virtual void onStorageEvent(const DeviceId& devid, MSStorageEventType event, const ErrorInfo& err) = 0;


};

//存储对象，存储录像，查询等
class MSSTORAGE_API MSStorage:public enable_shared_from_this<MSStorage>
{
public:
	MSStorage(const shared_ptr<MSMSStorageEvent>& event);
	virtual ~MSStorage();

	ErrorInfo start(uint32_t threadnum);
	ErrorInfo stop();

	//获取磁盘卷信息
	ErrorInfo getVoumeList(std::map<std::string, MSVolumeInfo>& volumeList);
	
	//添加删除nas卷
	ErrorInfo addNasVolume(const Host::DiskInfo& diskinfo);
	ErrorInfo delNasVolume(const std::string& name);

	//设置卷的使能状态
	ErrorInfo setVolumeConfig(const std::map<std::string, MSVolumeConfig>& config);
	ErrorInfo setVolumeConfig(const std::string& name, const MSVolumeConfig& config);

	//设置摄像机配置
	ErrorInfo setCameraConfig(const std::vector<MSCameraConfig>& config);
	

	//启动/停止 设备预录
	ErrorInfo startPrerecord(const DeviceId& devid,MSProtoStreamType streamtype, uint32_t startprercordtime, uint32_t stopprevrecordtime);
	ErrorInfo stopPrerecord(const DeviceId& devid, MSProtoStreamType streamtype);

	//放入数据进行预录和录像
	//trigger为触发类型，可以使用 MSTrigger 相或，当中途有变化，直接inputStream传入
	ErrorInfo inputStream(const DeviceId& devid, MSProtoStreamType streamtype, const shared_ptr<MSFrameInfo>& framinfo,uint32_t trigger);

	//启动设备录像，预录像时长,录像类型
	//trigger为触发类型，可以使用 MSTrigger 相或，当中途有变化，直接inputStream传入
	ErrorInfo startRecord(const DeviceId& devid, MSProtoStreamType streamtype, uint32_t trigger);
	//停止设备录像，停止录像后预,录像时长
	ErrorInfo stopRecord(const DeviceId& devid, MSProtoStreamType streamtype);
	//判断设备录像是否已经完成
	bool checkRecordIsFinish(const DeviceId& devid, MSProtoStreamType streamtype);

	ErrorInfo searchRecord(const DeviceId& devid, uint64_t starttime, uint64_t stoptime, std::list<MSBookRecordInfo>& recordlist);

	//创建播放流对象
	shared_ptr<MSRecordStream> createRecordStream(const DeviceId& devid, MSProtoStreamType streamtype);
private:
	struct MSStorageInternal;
	MSStorageInternal* internal;
};

}
}

