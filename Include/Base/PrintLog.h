//
//  Copyright (c)1998-2014, Public Technology
//  All Rights Reserved.
//
//	Description:
//	$Id: PrintLog.h 3 2013-01-21 06:57:38Z  $
//

#ifndef __BASE_PRINT_LOG_H__
#define __BASE_PRINT_LOG_H__

#include <stdio.h>
#include "Defs.h"
#include "Function.h"
#include "IntTypes.h"
#include <string>
#include "Base/Time.h"
#include "Base/Shared_ptr.h"
#include "String.h"

namespace Public
{
namespace Base
{

enum LOG_Level
{
	LOG_Level_NONE = 0,

	LOG_Level_FATAL, //致命
	LOG_Level_ERROR, //错误
	LOG_Level_WARN,  //警告
	LOG_Level_INFO,  //信息
	LOG_Level_TRACE, //跟踪
	LOG_Level_DEBUG, //调试
};

struct LogPrintInfo
{
	LOG_Level loglev;
	std::string loglevstr;

	std::string filename;
	std::string func;
	int line;

	Time time;

	std::string logstr;

	std::string logdetails;
};

/// 打印输出回调函数类型
/// 参数为要打印的字符串
typedef Function<void(const shared_ptr<LogPrintInfo> &)> LogPrinterProc;

///设置打印等级
void BASE_API setprintloglevel(LOG_Level level);

//打印内容
void BASE_API printer(LOG_Level level, const char *filename, const char *func, int line, const char *fmt, const std::vector<Value> &values);

/// 设置打印的输出回调函数
/// \param printer [in]  输出回调函数, 为空时设置打印输出到标准输出设备
/// \retval 0 成功
/// \retval -1 失败
int BASE_API setlogprinter(void *userflag, const LogPrinterProc &printer);

/// 清除打印的输出回调函数
int BASE_API cleanlogprinter(void *userflag);

#define logprinter(level, filename, func, line, fmt, ...)  \
	{                                                      \
		std::vector<Value> values;                         \
		processValues(values, fmt, ##__VA_ARGS__);         \
		printer(level, filename, func, line, fmt, values); \
	}

/// 记录调试信息，级别6，不直接调用，用于实现debgf宏
/// \return 返回打印的字节数
#define logdebug(fmt, ...) logprinter(LOG_Level_DEBUG, __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)

/// 记录跟踪信息，级别5，不直接调用，用于实现tracef宏
/// \return 返回打印的字节数
#define logtrace(fmt, ...) logprinter(LOG_Level_TRACE, __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)

/// 记录调试信息，级别4，不直接调用，用于实现infof宏
/// \return 返回打印的字节数
#define loginfo(fmt, ...) logprinter(LOG_Level_INFO, __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)

/// 记录警告信息，级别3，不直接调用，用于实现warnf宏
/// \return 返回打印的字节数
#define logwarn(fmt, ...) logprinter(LOG_Level_WARN, __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)

/// 记录错误信息，级别2，不直接调用，用于实现errorf宏
/// \return 返回打印的字节数
#define logerror(fmt, ...) logprinter(LOG_Level_ERROR, __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)

/// 记录致命错误信息，级别1，不直接调用，用于实现fatalf宏
/// \return 返回打印的字节数
#define logfatal(fmt, ...) logprinter(LOG_Level_FATAL, __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)

} // namespace Base
} // namespace Public

#endif //__BASE_PRINT_LOG_H__
