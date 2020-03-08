#include "RTSP/RTSP.h"
#include "Base/Base.h"

#if 0

#include <windows.h>
#include <fcntl.h>

#define MAXFILESIZE		1024*1024*1024

#define OFFSET			64*1024

void makefile(const std::string& filename)
{
	FILE* fd = fopen(filename.c_str(), "wb+");
	if (fd == NULL) return ;

	int ret = fseek(fd, MAXFILESIZE - 1, SEEK_SET);
	if (ret != 0)
	{
		assert(0);
	}

	char emtpy = 0;
	int writelen = fwrite(&emtpy, 1, 1, fd);
	if (writelen != 1)
	{
		assert(0);
	}

	fclose(fd);
}


int openfile(const std::string& filename)
{
	int mask;
	mask = _umask(0);
	_umask(mask);
	int fd = _open(filename.c_str(), O_RDWR | O_CREAT | O_BINARY | _O_NOINHERIT, (_S_IREAD | _S_IWRITE) &~mask);
	
	return fd;
}


int fd = 0;
std::string filename;
char* buf = NULL;

int readfile(uint32_t offset, void* buf, uint32_t count)
{
	DWORD read_bytes = 0;

	OVERLAPPED overlapped;
	memset(&overlapped, 0, sizeof(OVERLAPPED));

	overlapped.OffsetHigh = (uint32_t)((offset & 0xFFFFFFFF00000000LL) >> 32);
	overlapped.Offset = (uint32_t)(offset & 0xFFFFFFFFLL);

	HANDLE file = (HANDLE)_get_osfhandle(fd);
	SetLastError(0);
	BOOL RF = ReadFile(file, buf, count, &read_bytes, &overlapped);

	// For some reason it errors when it hits end of file so we don't want to check that
	if ((RF == FALSE) && GetLastError() != ERROR_HANDLE_EOF)
	{
		//logerror("Error reading file : %d\n", GetLastError());
		//assert(0);
	}

	return read_bytes;
}

void runwritemmapthread(Thread* thread,void* param)
{
	uint64_t t1 = Time::getCurrentMilliSecond();

	HANDLE filefd = (HANDLE)_get_osfhandle(fd);
	HANDLE hMapFile = CreateFileMappingA(filefd, NULL,                    // default secURLty 
		PAGE_READWRITE | SEC_COMMIT,          // read/write access
		0,                       // max. object size 
		MAXFILESIZE,                // buffer size  
		NULL); // name of mapping object

	if (hMapFile == INVALID_HANDLE_VALUE)
	{
		long err = GetLastError();
		assert(0);
		return ;
	}
	uint64_t t2 = Time::getCurrentMilliSecond();
	buf = (char*)MapViewOfFileEx(hMapFile, // handle to map object
		FILE_MAP_ALL_ACCESS,  // read/write permission
		0,
		MAXFILESIZE - OFFSET,
		OFFSET, NULL);

	if (buf == NULL)
	{
		long err = GetLastError();
		assert(0);
		return;
	}
	uint64_t t3 = Time::getCurrentMilliSecond();

	logdebug("MapViewOfFileEx  t3 - t1 = %llu t2-t1=%llu", t3 - t1, t2 - t1);

	int times = 0;
	while (thread->looping())
	{
		uint32_t val = times++;

		uint64_t t1 = Time::getCurrentMilliSecond();
		memcpy(buf, &val, sizeof(val));
		uint64_t t2 = Time::getCurrentMilliSecond();
		
		logdebug("mmap: write %d usedtime = %llums", val,t2-t1);

		Thread::sleep(1000);
	}

	if (buf != NULL)
	{
		UnmapViewOfFile((LPCTSTR)buf);
		buf = NULL;
	}
	if (hMapFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hMapFile);
		hMapFile = INVALID_HANDLE_VALUE;
	}
}

void readfilefd(Thread* thread, void* param)
{
	while (thread->looping())
	{
		uint32_t index = 0;

		int readlen = readfile(MAXFILESIZE - OFFSET, &index, sizeof(index));

		logdebug("fd: readlen %d readval %d", readlen, index);

		Thread::sleep(1000);
	}
}

void readfilemmmap(Thread* thread, void* param)
{
	while (thread->looping())
	{
		uint32_t index = 0;

		memcpy(&index, buf, sizeof(index));
	
		logdebug("mmap: readval %d",index);
	
		Thread::sleep(1000);
	}
}

void readfilebyfile(Thread* thread, void* param)
{
	while (thread->looping())
	{
		int readlen = 0;
		uint32_t readval = 0;

		FILE* fd = fopen(filename.c_str(), "rb");
		if (fd != NULL)
		{
			int ret = fseek(fd, MAXFILESIZE - OFFSET, SEEK_SET);
			if (ret == 0)
			{
				readlen = fread(&readval, 1, sizeof(readval), fd);
			}

			fclose(fd);
		}

		logdebug("file: readlen %d readval %d", readlen, readval);

		Thread::sleep(1000);
	}
}

int runtestfile()
{
	filename = "e:\\test.dat";

	{
		uint64_t t1 = Time::getCurrentMilliSecond();
		makefile(filename);
		uint64_t t2 = Time::getCurrentMilliSecond();

		fd = openfile(filename);
		uint64_t t3 = Time::getCurrentMilliSecond();


		logdebug("runtestfile  t3 - t1 = %llu t2-t1=%llu", t3 - t1, t2 - t1);
	}
	


	shared_ptr<Thread> t1 = ThreadEx::creatThreadEx("runwritemmapthread", runwritemmapthread, NULL);
	t1->createThread();

	shared_ptr<Thread> t2 = ThreadEx::creatThreadEx("readfilefd", readfilefd, NULL);
	t2->createThread();

	shared_ptr<Thread> t3 = ThreadEx::creatThreadEx("readfilebyfile", readfilebyfile, NULL);
	t3->createThread();

	shared_ptr<Thread> t4 = ThreadEx::creatThreadEx("readfilemmmap", readfilemmmap, NULL);
	t4->createThread();


	while (1) Thread::sleep(1000);

	return 0;
}
#endif