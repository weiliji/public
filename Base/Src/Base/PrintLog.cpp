//
//  Copyright (c)1998-2012,  Public Technology
//  All Rights Reserved.
//
//	Description:
//	$Id: PrintLog.cpp 11 2013-01-22 08:42:03Z  $
//

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef WIN32
#include <Windows.h>
#endif
#include <time.h>
#include "Base/Defs.h"
#include "Base/PrintLog.h"
#include "Base/Time.h"
#include "Base/Mutex.h"
#include "Base/Guard.h"
#include <list>

using namespace std;

////////////////////////////////////////////////////////////////////////////////

namespace Public
{
namespace Base
{

#if defined(WIN32) && !defined(_WIN32_WCE)

enum
{
	COLOR_FATAL = FOREGROUND_RED | FOREGROUND_BLUE,
	COLOR_ERROR = FOREGROUND_RED,
	COLOR_WARN = FOREGROUND_RED | FOREGROUND_GREEN,
	COLOR_INFO = FOREGROUND_GREEN,
	COLOR_TRACE = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
	COLOR_DEBUG = FOREGROUND_GREEN | FOREGROUND_BLUE,
};

static HANDLE stdout_handle = 0;
static WORD old_color_attrs = 0;

inline void set_console_color(int c)
{
	if (c == 0)
		return;

	if (stdout_handle == 0)
	{
		stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
	}

	// Gets the current text color.
	CONSOLE_SCREEN_BUFFER_INFO buffer_info;
	GetConsoleScreenBufferInfo(stdout_handle, &buffer_info);
	old_color_attrs = buffer_info.wAttributes;

	// We need to flush the stream buffers into the console before each
	// SetConsoleTextAttribute call lest it affect the text that is already
	// printed but has not yet reached the console.
	fflush(stdout);
	SetConsoleTextAttribute(stdout_handle, c);
}

inline void reset_console_color()
{
	if (stdout_handle == 0)
	{
		stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
	}

	fflush(stdout);
	// Restores the text color.
	SetConsoleTextAttribute(stdout_handle, old_color_attrs);
}

#else // defined(WIN32) && !defined(_WIN32_WCE)
enum
{
	COLOR_FATAL = 35,
	COLOR_ERROR = 31,
	COLOR_WARN = 33,
	COLOR_INFO = 32,
	COLOR_TRACE = 37,
	COLOR_DEBUG = 36
};

inline void set_console_color(int c)
{
	fprintf(stdout, "\033[%d;40m", c);
}

inline void reset_console_color()
{
	fprintf(stdout, "\033[0m");
}

#endif // defined(WIN32) && !defined(_WIN32_WCE)

////////////////////////////////////////////////////////////////////////////////

struct LogPrintInfoInternal
{
	LOG_Level s_printLogLevel;

	std::map<void *, Public::Base::LogPrinterProc> s_printList;
	Mutex logMutex;

	LogPrintInfoInternal() : s_printLogLevel(LOG_Level_DEBUG) {}
};

static LogPrintInfoInternal logmanager;

std::string getLogTimeAndLevelString(LOG_Level lev)
{
	const char *levestring = "";

	if (lev == LOG_Level_FATAL)
		levestring = "FATAL";
	else if (lev == LOG_Level_ERROR)
		levestring = "ERROR";
	else if (lev == LOG_Level_WARN)
		levestring = "WARN";
	else if (lev == LOG_Level_INFO)
		levestring = "INFO";
	else if (lev == LOG_Level_TRACE)
		levestring = "TRACE";
	else if (lev == LOG_Level_DEBUG)
		levestring = "DEBUG";

	return levestring;
}

inline int getLevlevColor(LOG_Level lev)
{
	int color = 0;
	if (lev == LOG_Level_FATAL)
		color = COLOR_FATAL;
	else if (lev == LOG_Level_ERROR)
		color = COLOR_ERROR;
	else if (lev == LOG_Level_WARN)
		color = COLOR_WARN;
	else if (lev == LOG_Level_INFO)
		color = COLOR_INFO;
	else if (lev == LOG_Level_TRACE)
		color = COLOR_TRACE;
	else if (lev == LOG_Level_DEBUG)
		color = COLOR_DEBUG;

	return color;
}

int BASE_API setlogprinter(void *userflag, const LogPrinterProc &printer)
{
	Guard locker(logmanager.logMutex);

	logmanager.s_printList[userflag] = printer;

	return 0;
}

int BASE_API cleanlogprinter(void *userflag)
{
	Guard locker(logmanager.logMutex);
	logmanager.s_printList.erase(userflag);

	return 0;
}

void BASE_API setprintloglevel(LOG_Level level)
{
	logmanager.s_printLogLevel = level;
}

#ifndef DEBUG
#define DEBUGPrintLoeveCheck(lev) /*{if(lev == LOG_Level_DEBUG) return -1;}*/
#else
#define DEBUGPrintLoeveCheck(lev)
#endif

const char* _getFileName(const char* filename)
{
	size_t pos = strlen(filename);

	while (pos > 0)
	{
		if (filename[pos - 1] == '\\' || filename[pos - 1] == '/') return filename + pos;
		pos--;
	}

	return filename;
}

void printer(LOG_Level level, const char *filename, const char *func, int line, const char *fmt, const std::vector<Value> &values)
{
	DEBUGPrintLoeveCheck(LOG_Level_DEBUG);

	if (logmanager.s_printLogLevel < level)
		return;

	shared_ptr<LogPrintInfo> info = make_shared<LogPrintInfo>();

	info->loglev = level;
	info->loglevstr = getLogTimeAndLevelString(level);

	//file name
	info->filename = _getFileName(filename);
	
	info->func = func;
	info->line = line;
	info->time = Time::getCurrentTime();

	//set logstr and logcontent
	{
		Guard locker(logmanager.logMutex);
#define MAXPRINTBUFFERLEN 10240
		static char buffer[MAXPRINTBUFFERLEN] = {0};

		{
			String::_snprintf_x(buffer, MAXPRINTBUFFERLEN - 1, fmt, values);

			size_t bufferlen = strlen(buffer);

			while (bufferlen > 0 && (buffer[bufferlen - 1] == '\r' || buffer[bufferlen - 1] == '\n'))
			{
				buffer[bufferlen - 1] = 0;
				bufferlen--;
			}

			info->logstr = std::string(buffer,bufferlen);
		}

		char fmttmp[64] = {0};
		{
			snprintf_x(fmttmp, 64, "%04d-%02d-%02d %02d:%02d:%02d.%03u", info->time.year, info->time.month, info->time.day, info->time.hour, info->time.minute, info->time.second, info->time.millisecond);
		}

		//snprintf_x(buffer, MAXPRINTBUFFERLEN - 1, "%s %s - %s", fmttmp, info->loglevstr.c_str(), info->logstr.c_str());
		//snprintf_x(buffer, MAXPRINTBUFFERLEN - 1, "%s %s - %s  [%s %s:%d]", fmttmp, info->loglevstr.c_str(), info->logstr.c_str(), _getFileName(info->filename.c_str()), info->func.c_str(), info->line);
		snprintf_x(buffer, MAXPRINTBUFFERLEN - 1, "%s %s - %s  [%s:%d]", fmttmp, info->loglevstr.c_str(), info->logstr.c_str(), _getFileName(info->filename.c_str()),info->line);

		info->logdetails = buffer;
	}

	//pritnf
	{
		Guard locker(logmanager.logMutex);

		set_console_color(getLevlevColor(level));

		size_t len = info->logdetails.length();
		const char *s = info->logdetails.c_str();

		bool ishaveenter = false;
		if (len > 1 && (s[len - 1] == '\n' || s[len - 1] == '\r'))
		{
			ishaveenter = true;
		}
		fputs(info->logdetails.c_str(), stdout);
		if (!ishaveenter)
		{
			fputs("\r\n", stdout);
		}

		reset_console_color();
	}

	//set callback
	{
		std::map<void *, Public::Base::LogPrinterProc>::iterator iter;
		for (iter = logmanager.s_printList.begin(); iter != logmanager.s_printList.end(); iter++)
		{
			Public::Base::LogPrinterProc s_print = iter->second;
			s_print(info);
		}
	}
}

} // namespace Base
} // namespace Public
