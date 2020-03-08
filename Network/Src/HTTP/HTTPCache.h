#pragma once
#include "Base/Base.h"
using namespace Public::Base;

namespace Public {
namespace Network{
namespace HTTP {

class HTTPCache
{
public:
	HTTPCache() {}
	virtual ~HTTPCache() {}
	virtual uint32_t write(const char* buffer, uint32_t len) = 0;
	virtual uint32_t read(uint32_t pos, char* buffer, uint32_t len) = 0;
	virtual uint32_t totalsize() = 0;
};

#define MAXBUFFERCACHESIZE	5*1024*1024

class HTTPCacheMem :public HTTPCache
{
	struct BufferItem
	{
		BufferItem() {}
		std::string		data;
	};
public:
	HTTPCacheMem() :cachetotalsize(0) {}
	virtual ~HTTPCacheMem() {}
	virtual uint32_t write(const char* buffer, uint32_t len)
	{
		Guard locker(mutex);
		//	if (cachetotalsize >= MAXBUFFERCACHESIZE) return false;

		BufferItem item;
		item.data = std::string(buffer, len);

		memcache.push_back(item);

		cachetotalsize += len;

		return len;
	}
	virtual uint32_t totalsize()
	{
		Guard locker(mutex);

		return cachetotalsize;
	}
	virtual uint32_t read(uint32_t pos, char* buffer, uint32_t len)
	{
		Guard locker(mutex);

		uint32_t havereadlen = 0;

		uint32_t offsize = 0;
		for (std::list<BufferItem>::iterator iter = memcache.begin(); iter != memcache.end() && havereadlen < len; iter++)
		{
			uint32_t readoffsize = 0;
			uint32_t canreadlen = 0;
			if (offsize >= pos)
			{
				//从0开始，读取datalen
				canreadlen = min((uint32_t)iter->data.length(), len - havereadlen);
			}
			else if (offsize + iter->data.length() <= pos)
			{
				//无数据可读
			}
			else
			{
				//只有这些数据可读
				readoffsize = pos - offsize;
				canreadlen = min((uint32_t)iter->data.length() - readoffsize, len - havereadlen);
			}

			if (canreadlen > 0)
			{
				memcpy(buffer + havereadlen, iter->data.c_str() + readoffsize, canreadlen);

				havereadlen += canreadlen;
			}

			offsize += (uint32_t)iter->data.length();
		}

		return havereadlen;
	}
private:
	uint32_t				cachetotalsize;
	Mutex					mutex;
	std::list<BufferItem>	memcache;
};

class HTTPCacheFile :public HTTPCache
{
public:
	HTTPCacheFile(const std::string& _filename, bool _deletefile, bool readmode)
		:filesize(0), filename(_filename), needdelete(_deletefile), writepos(0)
	{
		fd = fopen(filename.c_str(), readmode ? "rb" : "wb+");

		FileInfo info;
		if (File::stat(filename, info) && readmode) filesize = (int)info.size;
	}
	virtual ~HTTPCacheFile()
	{
		if (fd != NULL)
		{
			fclose(fd);
			fd = NULL;
		}
		if (needdelete)
		{
			int deletetimes = 0;
			while (deletetimes++ <= 10 && File::access(filename, File::accessExist))
			{
				File::remove(filename);
				if (!File::access(filename, File::accessExist)) break;

				Thread::sleep(100);
			}
		}
	}
	virtual uint32_t totalsize()
	{
		return filesize;
	}
	virtual uint32_t write(const char* buffer, uint32_t len)
	{
		if (fd == NULL) return false;

		fseek(fd, (int)writepos, SEEK_SET);
		size_t ret = fwrite(buffer, 1, len, fd);
		fflush(fd);
		if (ret > 0) writepos += ret;

		return (uint32_t)ret;
	}
	virtual uint32_t read(uint32_t pos, char* buffer, uint32_t len)
	{
		if (fd == NULL || (int)pos >= filesize) return 0;

		fseek(fd, (uint32_t)pos, SEEK_SET);
		uint32_t readlen = (uint32_t)fread(buffer, 1, len, fd);

		return readlen;
	}
private:
	int					filesize;
	std::string			filename;
	bool				needdelete;

	size_t				writepos;
public:
	FILE*				fd;
};


struct ServerCacheFilePath
{
public:
	ServerCacheFilePath()
	{
		cachepath = File::getExcutableFileFullPath() + PATH_SEPARATOR + ".httpcache";

		File::removeDirectory(cachepath);
	}
	~ServerCacheFilePath()
	{
		File::removeDirectory(cachepath);
	}
	static ServerCacheFilePath* instance()
	{
		static ServerCacheFilePath file;

		return &file;
	}

	std::string cachefilename()
	{
		if (!File::access(cachepath, File::accessExist)) File::makeDirectory(cachepath);


		char buffer[256];
		snprintf_x(buffer, 255, "%s%c%llu_%06x.md", cachepath.c_str(), PATH_SEPARATOR, Time::getCurrentMilliSecond(), buffer);

		return buffer;
	}
private:
	std::string cachepath;
};

}
}
}

