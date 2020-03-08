//
//  Copyright (c)1998-2012,  Public Technology
//  All Rights Reserved.
//
//	Description:
//	$Id: Semaphore.cpp 33 2013-02-05 09:43:35Z  $
//
#if defined(__linux__)
#include <semaphore.h>
#include <time.h>
#endif

#include <assert.h>
#include "Base/Semaphore.h"
#include "Base/PrintLog.h"
#include "Base/Thread.h"
#ifdef WIN32
#include <Windows.h>
#define INCREMENT_AMOUNT 1
#define LONG_MAX 2147483647L /* maximum (signed) long value */
#elif defined(__linux__) || defined(__APPLE__)
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#else
#error "Unknown Platform"
#endif

namespace Public
{
namespace Base
{

struct Semaphore::SemaphoreInternal
{
#ifdef WIN32
	void *handle;
#elif defined(__linux__) || defined(__APPLE__)
	sem_t *sem;
	char semname[64];
#endif
};

Semaphore::Semaphore(int initialCount)
{
	internal = new SemaphoreInternal;

#ifdef WIN32
	internal->handle = CreateSemaphore(NULL, initialCount, LONG_MAX, NULL);
	BASE_Assert(internal->handle);

#elif defined(__linux__)
	internal->sem = (sem_t *)malloc(sizeof(sem_t));
	BASE_Assert(internal->sem);
	int ret = sem_init(internal->sem, 0, initialCount);
	BASE_Assert(ret != -1);
	(void)ret; // 消除变量未使用编译警告
#elif defined(__APPLE__)
	snprintf_x(internal->semname, 64, "%llu_%llu", this, Time::getCurrentMilliSecond());
	internal->sem = sem_open(internal->semname, O_CREAT | O_EXCL, S_IRWXU, 0);
	BASE_Assert(internal->sem != NULL);
#endif
}

Semaphore::~Semaphore()
{
#ifdef WIN32
	CloseHandle(internal->handle);

#elif defined(__linux__)
	BASE_Assert(internal->sem);
	int ret = sem_destroy(internal->sem);
	BASE_Assert(ret == 0);
	(void)ret; // 消除变量未使用编译警告
	free(internal->sem);
#elif defined(__APPLE__)
	BASE_Assert(internal->sem != NULL);
	sem_unlink(internal->semname);
	internal->sem = NULL;
#endif
	delete internal;
}

int Semaphore::pend()
{
#ifdef WIN32
	BASE_Assert(internal->handle);
	return WaitForSingleObject(internal->handle, INFINITE);

#elif defined(__linux__) || defined(__APPLE__)
	BASE_Assert(internal->sem);
	int ret = 0;
	while ((ret = sem_wait(internal->sem)) != 0 && errno == EINTR)
		;

	if (ret != 0)
	{
		logerror("Semaphore::pend() errno %d\r\n", errno);

		perror("sem_wait");
	}

	return ret;

#endif
}

/// 减少信号量计数，如果是从0累加，会唤醒其等待队列的第一个线程
/// \timeout [in] 超时时间,单位毫秒
/// \return >=0 当前信号量计数
///         <0  超时

int Semaphore::pend(uint32_t timeout)
{

#ifdef WIN32
	BASE_Assert(internal->handle);
	DWORD ret = WaitForSingleObject(internal->handle, timeout);
	if (ret == WAIT_OBJECT_0)
		return 0;
	else
		return -1;
#elif defined(__linux__)
	BASE_Assert(internal->sem);
	int ret = 0;
	struct timespec timeSpec;
	clock_gettime(CLOCK_REALTIME, &timeSpec);
	timeSpec.tv_sec += (timeout < 1000) ? 1 : (timeout / 1000);
	while ((ret = sem_timedwait(internal->sem, &timeSpec)) != 0 && errno == EINTR)
		;

	return ret;
#elif defined(__APPLE__)
	BASE_Assert(internal->sem);
	int ret = 0;

	uint64_t starttime = Time::getCurrentMilliSecond();
	do
	{
		{
			ret = sem_trywait(internal->sem);
			if (ret == 0)
				break;
		}

		Thread::sleep(10);

		{
			ret = sem_trywait(internal->sem);
			if (ret == 0)
				break;
		}

	} while (Time::getCurrentMilliSecond() - starttime > timeout);

	return ret;
#endif
}

int Semaphore::post()
{
#ifdef WIN32
	LONG cnt = 0; // 下面的参数是用来获得该信号量当前的被使用的数量
	return ReleaseSemaphore(internal->handle, INCREMENT_AMOUNT, &cnt);

#elif defined(__linux__) || defined(__APPLE__)
	assert(internal->sem);
	return sem_post(internal->sem);

#endif
}

} // namespace Base
} // namespace Public
