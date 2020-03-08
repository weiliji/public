//
//  Copyright (c)1998-2012, Chongqing Technology
//  All Rights Reserved.
//
//	Description:
//	$Id: PrintLog.cpp 11 2013-01-22 08:42:03Z $
//

#pragma  once

#include "Base/PrintLog.h"
#include "Base/Thread.h"
#include "Base/File.h"
#include "Base/Guard.h"
#include "Base/Directory.h"
#include "Base/Semaphore.h"

namespace Public{
namespace Base{

#define MAXLOGFILESIZE		1024*1024
#define ONEDAYTIME			(24*60*60)
#define ONEHOURE			(60*60)

#define MAXLOGSTACKSIZE		10000

class Log :public Thread
{
public:
	Log() :Thread("LogInternal"), currCheckLogDay(0){}
	~Log(){}

	//log level 打印等级  0:all 1:debug 2:info 3:warn 4:error 5:fatal 6:off
	bool init(const std::string& _appName,const std::string& _logpath, LOG_Level _logLevel)
	{
		appName = _appName;
		printlev = _logLevel;
#ifndef DEBUG
		printlev = LOG_Level_INFO;
#endif
		logPath = _logpath;
		if (logPath.length() == 0)
		{
			logPath = File::getExcutableFileFullPath();
		}

		logPath = logPath + PATH_SEPARATOR + "log";

		createThread();

		return true;
	}
	bool uninit()
	{
		cancelThread();
		logSem.post();
		destroyThread();

		{
			Guard locker(logMutex);
			logList.clear();
		}

		return true;
	}
	void print(const shared_ptr<LogPrintInfo>& info)
	{
		Guard locker(logMutex);

		/*if(logList.size() >= MAXLOGSTACKSIZE)
		{
		logList.front();
		logList.pop_front();
		}*/
	
		logList.push_back(info);
		logSem.post();
	}
private:
	void threadProc()
	{
		int logindex = 0;
		uint64_t logsize = 0;
		uint64_t curfilehour = 0;
		FILE* fd = NULL;
		std::string currlogfilename;
		while (looping())
		{
			checkAndDeleteLog();

			//检测文件是否改切换
			if (fd != NULL)
			{
				Time nowtime = Time::getCurrentTime();
				uint64_t nowtimestmap = nowtime.makeTime();
				uint64_t nowhour = (uint64_t)(nowtimestmap / ONEHOURE);

				if (nowhour != curfilehour)
				{
					fclose(fd);
					logindex = 0;
					fd = NULL;
					if (logsize == 0)
					{
						File::remove(currlogfilename);
					}
					logsize = 0;
				}
				else if (logsize >= MAXLOGFILESIZE)
				{
					fclose(fd);
					logindex++;
					logsize = 0;
					fd = NULL;
				}
			}

			logSem.pend(500);

			if (fd == NULL)
			{
				if (!File::access(logPath.c_str(), File::accessExist))
				{
					File::makeDirectory(logPath.c_str());
				}

				Time nowtime = Time::getCurrentTime();
				curfilehour = (uint64_t)(nowtime.makeTime() / ONEHOURE);

				char buffer[256] = { 0 };
				snprintf(buffer, 255, "%s/logFile_%04d-%02d-%02d_%02d%02d%02d_%02d_%d.log", logPath.c_str(),
					nowtime.year, nowtime.month, nowtime.day, nowtime.hour, nowtime.minute, nowtime.second, logindex, Process::getProcessId());
				logsize = 0;
				currlogfilename = buffer;
				fd = fopen(currlogfilename.c_str(), "wb+");
			}

			if (fd == NULL)
			{
				continue;
			}

			shared_ptr<LogPrintInfo> info;
			{
				Guard locker(logMutex);
				if (logList.size() <= 0)
				{
					continue;
				}
				info = logList.front();
				logList.pop_front();
			}



			if (info->loglev > printlev) continue;

			const char* logstr = info->logdetails.c_str();
			size_t logstrlen = info->logdetails.length();

			bool ishaveenter = false;
			if (logstrlen > 1 && (logstr[logstrlen - 1] == '\n' || logstr[logstrlen - 1] == '\r'))
			{
				ishaveenter = true;
			}

			size_t writelen = fwrite(logstr, 1, logstrlen, fd);
			if (writelen > 0) logsize += writelen;

			if (!ishaveenter)
			{
				writelen = fwrite("\r\n", 1, 2, fd);
				if (writelen > 0) logsize += writelen;
			}		
		
			fflush(fd);
		}

		if (fd != NULL)
		{
			fclose(fd);
			if (logsize == 0)
			{
				File::remove(currlogfilename);
			}
		}
	}


	void checkAndDeleteLog()
	{
#define MAXLOGSAVYDAYS		30

		Public::Base::Time currTime = Public::Base::Time::getCurrentTime();

		if (currTime.makeTime() / ONEDAYTIME == currCheckLogDay)
		{
			return;
		}

		uint64_t lastLogFileTime = currTime.makeTime() - MAXLOGSAVYDAYS * ONEDAYTIME;

		Directory browser(logPath);
		while (1)
		{
			Directory::Dirent dir;
			if (!browser.read(dir))
			{
				break;
			}
			if (dir.Type == Directory::Dirent::DirentType_File)
			{
				if (dir.FileSize == 0)
				{
					File::remove((dir.Path + PATH_SEPARATOR + dir.Name).c_str());
				}
				else if (dir.CreatTime < lastLogFileTime)
				{
					File::remove((dir.Path + PATH_SEPARATOR + dir.Name).c_str());
				}
			}
		}
		currCheckLogDay = currTime.makeTime() / ONEDAYTIME;
	}
private:
	Mutex									logMutex;
	std::string								logPath;
	std::string								appName;
	LOG_Level								printlev;
	std::list<shared_ptr<LogPrintInfo> > 	logList;
	Semaphore								logSem;
	uint64_t								currCheckLogDay;
};

};
};


