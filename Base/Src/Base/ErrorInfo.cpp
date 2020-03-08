#include "Base/ErrorInfo.h"
namespace Public
{
namespace Base
{

struct ErrorItemInfo
{
	Error_Code code;
	const char *errmsg;
};
std::string ErrorInfo::getErrorMsg(Error_Code code)
{
	static ErrorItemInfo errorItemInfosMap[] =
		{
			{Error_Code_Success, "成功"},
			{Error_Code_Fail, "失败"},
			{Error_Code_NotSupport, "不支持"},
			{Error_Code_Param, "参数错误"},

			//100类，权限相关的错误码
			{Error_Code_Password, "密码错误"},
			{Error_Code_OldPassword, "旧密码错误"},
			{Error_Code_Username, "用户名错误"},
			{Error_Code_Authen, "认证失败"},

			//200类，网络相关的错误码
			{Error_Code_Network, "网络错误"},
			{Error_Code_NetworkTimeout, "网络超时"},
			{Error_Code_ConnectTimeout, "连接超时"},
			{Error_Code_CommunicateTimeout, "通讯超时"},
			{Error_Code_Unreachable, "不能到达"},

			//300类，数据解析相关错误码
			{Error_Code_ParseParam, "解析参数错误"},
			{Error_Code_ParseObject, "解析对象错误"},
			{Error_Code_ParseFile, "解析文件错误"},
			{Error_Code_ParseUrl, "解析url错误"},
			{Error_Code_Request, "请求错误"},
			{Error_Code_Response, "回复错误"},

			//400类，设备服务相关错误码
			{Error_Code_InitFail, "初始化失败"},
			{Error_Code_ResourceUninit, "资源未初始化"},
			{Error_Code_ResourceCreateFail, "资源创建失败"},
			{Error_Code_ResourceNotExist, "资源不存在"},
			{Error_Code_ResourceExisted, "资源已存在"},
			{Error_Code_DeviceNotExist, "设备不存在"},
			{Error_Code_DeviceExisted, "设备已存在"},
			{Error_Code_DeviceOffOnline, "设备离线"},
			{Error_Code_ServiceNotExist, "服务不存在"},
			{Error_Code_ServiceExisted, "服务已存在"},
			{Error_Code_CameraNotExist, "通道不存在"},
			{Error_Code_CameraExisted, "通道已存在"},
			{Error_Code_CameraOffOnline, "通道离线"},
			{Error_Code_FileExisted, "文件已存在"},
			{Error_Code_FileNotExist, "文件不存在"},
			{Error_Code_Busy, "资源忙"},

			//500类，录像相关
			{Error_Code_InvalidRecord, "无效的录像"},
			{Error_Code_InvalidService, "无效的服务"},
			{Error_Code_InvalidDevice, "无效的设备"},

			//600类，文件数据相关
			{Error_Code_Send, "发送数据失败"},
			{Error_Code_Recive, "接收数据失败"},
			{Error_Code_OpenFile, "打开文件失败"},
			{Error_Code_WriteFile, "写入文件失败"},
			{Error_Code_ReadFile, "读取文件失败"},

			//700类，磁盘相关操作
			{Error_Code_DiskErr, "磁盘异常错误"},
			{Error_Code_DiskFull, "磁盘满"},

			//800类，流相关操作
			{Error_Code_StreamType, "流类型错误"},
			{Error_Code_StreamCmd, "播放控制命令错误"},

			//900类，数据库相关
			{Error_Code_OpenDatabase, "打开数据库失败"},
			{Error_Code_CreateDatabase, "创建数据库失败"},
			{Error_Code_ExecDatabase, "执行数据库命令失败"},
		};

	for (uint32_t i = 0; i < sizeof(errorItemInfosMap) / sizeof(ErrorItemInfo); i++)
	{
		if (errorItemInfosMap[i].code == code)
			return errorItemInfosMap[i].errmsg;
	}

	return "未知错误";
}

ErrorInfo::ErrorInfo(const ErrorInfo &ret)
{
	errcode = ret.errcode;
	errmsgstr = ret.errmsgstr;
}
//系统错误码
ErrorInfo::ErrorInfo(Error_Code _errorCode, const std::string &_errmsg)
{
	errcode = _errorCode;
	errmsgstr = _errmsg;

	if (errmsgstr.length() <= 0)
		errmsgstr = getErrorMsg(errcode);
}
//如果result == false，将会从getlasterror获取

ErrorInfo ::~ErrorInfo() {}

ErrorInfo &ErrorInfo::operator=(const ErrorInfo &ret)
{
	errcode = ret.errcode;
	errmsgstr = ret.errmsgstr;

	return *this;
}

bool ErrorInfo::operator==(const ErrorInfo &ret) const
{
	return errcode == ret.errcode && errmsgstr == ret.errmsgstr;
}

bool ErrorInfo::operator!=(const ErrorInfo &ret) const
{
	return errcode != ret.errcode || errmsgstr != ret.errmsgstr;
}

bool ErrorInfo::error() const
{
	return errcode != Error_Code_Success;
}

////是否失败
ErrorInfo::operator bool() const
{
	return errcode != Error_Code_Success;
}

//是否正常
bool ErrorInfo::operator!() const
{
	return errcode == Error_Code_Success;
}
std::string ErrorInfo::errorMessage() const
{
	return errmsgstr;
}
int ErrorInfo::errorCode() const
{
	return errcode;
}

} // namespace Base
} // namespace Public