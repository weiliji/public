//
//  Copyright (c)1998-2012,  Public Technology
//  All Rights Reserved.
//
//	Description:
//	$Id: ReadWriteMutex.cpp 11 2013-01-22 08:42:03Z  $
//

#include <assert.h>
#include "Base/ReadWriteMutex.h"
#include "Base/PrintLog.h"

#ifdef WIN32
#include <Windows.h>
#include <stdio.h>
#elif defined(__linux__) || defined(__APPLE__)
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#endif

#ifdef __linux

#define RWMUXTEX_MAXENTER 0xffff

#endif /* __linux*/

namespace Public
{
namespace Base
{

struct ReadWriteMutex::ReadWriteMutexInternal
{
#ifdef WIN32
	SRWLOCK			 lock;
#elif defined(__linux__) || defined(__APPLE__)
	pthread_rwlock_t lock;
#endif
};

ReadWriteMutex::ReadWriteMutex()
{
	internal = new ReadWriteMutexInternal;
	assert(internal);

#if defined(__linux__) || defined(__APPLE__)
	int ret = pthread_rwlock_init(&internal->lock, NULL);
	BASE_Assert(ret == 0);
	(void)ret; // 消除变量未使用编译警告

#elif defined(WIN32)
	InitializeSRWLock(&internal->lock);
#endif
}

ReadWriteMutex::~ReadWriteMutex()
{
#if defined(__linux__) || defined(__APPLE__)
	int ret = pthread_rwlock_destroy(&internal->lock);
	BASE_Assert(ret == 0);
	(void)ret; // 消除变量未使用编译警告

#elif defined(WIN32)
	//MSDN: SRW locks do not need to be explicitly destroyed.
#endif

	delete internal;
}

bool ReadWriteMutex::enterRead()
{
#if defined(__linux__) || defined(__APPLE__)
	int ret = pthread_rwlock_rdlock(&internal->lock);
	if (ret != 0)
		return false;
	return true;
#elif defined(WIN32)
	AcquireSRWLockShared(&internal->lock);
	return true;
#endif
	return false;
}


bool ReadWriteMutex::leaveRead()
{
#if defined(__linux__) || defined(__APPLE__)
	int ret = pthread_rwlock_unlock(&internal->lock);
	if (ret != 0)
		return false;
	return true;

#elif defined(WIN32)
	ReleaseSRWLockShared(&internal->lock);
	return true;
#endif

	return false;
}

bool ReadWriteMutex::enterWrite()
{
#if defined(__linux__) || defined(__APPLE__)
	int ret = pthread_rwlock_wrlock(&internal->lock);
	if (ret != 0)
		return false;
	return true;

#elif defined(WIN32)
	AcquireSRWLockExclusive(&internal->lock);
	return true;
#endif
	return false;
}
bool ReadWriteMutex::leaveWrite()
{
#if defined(__linux__) || defined(__APPLE__)
	int ret = pthread_rwlock_unlock(&internal->lock);
	if (ret != 0)
		return false;
	return true;

#elif defined(WIN32)
	ReleaseSRWLockExclusive(&internal->lock);
	return true;
#endif

	return false;
}

} // namespace Base
} // namespace Public
