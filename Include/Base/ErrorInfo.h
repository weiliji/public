#pragma once
#include "Base/IntTypes.h"
#include "Base/Defs.h"
#include "Base/String.h"
namespace Public
{
namespace Base
{

typedef enum
{
	//错误码定义
	Error_Code_Success = 0,
	Error_Code_Fail = -1,
	Error_Code_NotSupport = -2,
	Error_Code_Param = -3,

	//100类，权限相关的错误码
	Error_Code_Password = -100,
	Error_Code_OldPassword = -101,
	Error_Code_Username = -102,
	Error_Code_Authen = -103,

	//200类，网络相关的错误码
	Error_Code_Network = -200,
	Error_Code_NetworkTimeout = -201,
	Error_Code_ConnectTimeout = -202,
	Error_Code_CommunicateTimeout = -203,
	Error_Code_Unreachable = -204,

	//300类，数据解析相关错误码
	Error_Code_ParseParam = -300,
	Error_Code_ParseObject = -301,
	Error_Code_ParseFile = -302,
	Error_Code_ParseUrl = -303,
	Error_Code_Request = -304,
	Error_Code_Response = -305,

	//400类，设备服务相关错误码
	Error_Code_InitFail = -400,
	Error_Code_ResourceUninit = -401,
	Error_Code_ResourceCreateFail = -402,
	Error_Code_ResourceNotExist = -403,
	Error_Code_ResourceExisted = -404,
	Error_Code_DeviceNotExist = -405,
	Error_Code_DeviceExisted = -406,
	Error_Code_DeviceOffOnline = -407,
	Error_Code_ServiceNotExist = -408,
	Error_Code_ServiceExisted = -409,
	Error_Code_CameraNotExist = -410,
	Error_Code_CameraExisted = -411,
	Error_Code_CameraOffOnline = -412,
	Error_Code_FileExisted = -413,
	Error_Code_FileNotExist = -414,
	Error_Code_Busy = -415,

	//500类，录像相关
	Error_Code_InvalidRecord = -500,
	Error_Code_InvalidService = -501,
	Error_Code_InvalidDevice = -502,

	//600类，文件数据相关
	Error_Code_Send = -600,
	Error_Code_Recive = -601,
	Error_Code_OpenFile = -602,
	Error_Code_WriteFile = -603,
	Error_Code_ReadFile = -604,

	//700类，磁盘相关操作
	Error_Code_DiskErr = -700,
	Error_Code_DiskFull = -701,

	//800类，流相关操作
	Error_Code_StreamType = -800,
	Error_Code_StreamCmd = -801,

	//900类，数据库相关
	Error_Code_OpenDatabase = -900,
	Error_Code_CreateDatabase = -901,
	Error_Code_ExecDatabase = -902,
} Error_Code;

class BASE_API ErrorInfo
{
	std::string getErrorMsg(Error_Code code);

public:
	ErrorInfo(const ErrorInfo &ret);
	//系统错误码
	ErrorInfo(Error_Code _errorCode = Error_Code_Success, const std::string &_errmsg = "");
	//如果result == false，将会从getlasterror获取

	~ErrorInfo();

	ErrorInfo &operator=(const ErrorInfo &ret);

	bool operator==(const ErrorInfo &ret) const;
	bool operator!=(const ErrorInfo &ret) const;

	bool error() const;

	////是否失败
	operator bool() const;

	//是否正常
	bool operator!() const;

	std::string errorMessage() const;
	int errorCode() const;

public:
	Error_Code errcode;
	std::string errmsgstr;
};

} // namespace Base
} // namespace Public