/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/*!
 * \file
 * \brief CCSemaphore class source file
 * \author Young-il Choi <duddlf.choi@samsung.com>
 * \date 2006-07-28
 */




#if defined (_WIN32)

#include <windows.h>

#endif

#if defined (_LINUX)

#include <pthread.h>
#include <errno.h>
#include <semaphore.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>

#endif




#include <CSP.h>

#include "CCSemaphoreDefine.h"




#if defined (_LINUX)

extern pthread_mutex_t TimeMutex;

#endif




bool CCSemaphore::Create(long count)
{
	ASSERT(FlagCreate() == false);

	try
	{
		m = new CTSemaphoreMember;
		if (m == NULL) throw 0;
	}
	catch (...)
	{
		return false;
	}

#if defined (_WIN32)

	m->addr = ::CreateSemaphore(NULL, (long)count, 0x7fffffff, NULL);
	if (m->addr == NULL)
	{
		return false;
	}

#elif defined (_LINUX)

	if (pthread_cond_init(&m->cond, NULL) != 0)
	{
		return false;
	}

	m->count = count;

#endif

	THREADSAFE_CHECK_INITIALIZE

	return true;
}




void CCSemaphore::Destroy(void)
{
	THREADSAFE_CHECK

	ASSERT(FlagCreate() == true);

#if defined (_WIN32)

	::CloseHandle(m->addr);
	delete m;

#elif defined (_LINUX)

	pthread_cond_destroy(&m->cond);
	delete m;
	
#endif

	m = NULL;
	return;
}




void threadsafe CCSemaphore::Take(void)
{
	ASSERT(FlagCreate() == true);

#if defined (_WIN32)

	::WaitForSingleObject(m->addr, INFINITE);

#elif defined (_LINUX)

	pthread_mutex_lock(&TimeMutex);

	while (m->count <= 0)
	{
		pthread_cond_wait(&m->cond, &TimeMutex);
	}

	m->count--;
	pthread_mutex_unlock(&TimeMutex);

#endif
}




bool threadsafe CCSemaphore::Try(unsigned long msec)
{
	ASSERT(FlagCreate() == true);

#if defined (_WIN32)

	return ::WaitForSingleObject(m->addr, msec) == WAIT_OBJECT_0;

#elif defined (_LINUX)

	if (msec == 0)
	{
		pthread_mutex_lock(&TimeMutex);

		if (m->count <= 0)
		{
			pthread_mutex_unlock(&TimeMutex);

			return false;
		}

		m->count--;
		pthread_mutex_unlock(&TimeMutex);

		return true;
	}

	struct timeval  now;
	struct timespec timeout;

	unsigned long base = CCTime::Tick();
	unsigned long elapse = 0;
	unsigned long aggregateMilliSec = 0;
	unsigned long overSec = 0;
	unsigned long underMilliSec = 0;

	pthread_mutex_lock(&TimeMutex);

	while (m->count <= 0) 
	{
		gettimeofday(&now, NULL);

		aggregateMilliSec = (msec - elapse) + now.tv_usec / 1000;

		// overflow 경우 - INFINITY 처리
		if ((long) aggregateMilliSec < now.tv_usec / 1000)
		{
			pthread_cond_wait(&m->cond, &TimeMutex);
		}
		else
		{
			overSec =  aggregateMilliSec / 1000;
			underMilliSec = aggregateMilliSec % 1000;

			timeout.tv_sec = now.tv_sec + overSec;
			timeout.tv_nsec = underMilliSec * 1000000;

			if (pthread_cond_timedwait(&m->cond, &TimeMutex, &timeout) == ETIMEDOUT)
			{
				pthread_mutex_unlock(&TimeMutex);
	
				return false;
			}
		}

		elapse += (CCTime::Tick() - base);

		if (elapse >= msec) 
		{
			pthread_mutex_unlock(&TimeMutex);
			return false;
		}
	}

	m->count--;
	pthread_mutex_unlock(&TimeMutex);

	return true;

#endif

}




/*

bool threadsafe CCSemaphore::Try(unsigned long msec)
{
	ASSERT(FlagCreate() == true);

#if defined (_WIN32)

	return ::WaitForSingleObject(m->addr, msec) == WAIT_OBJECT_0;

#elif defined (_LINUX)

	if (msec == 0)
	{
		pthread_mutex_lock(&TimeMutex);

		if (m->count <= 0)
		{
			pthread_mutex_unlock(&TimeMutex);

			return false;
		}

		m->count--;
		pthread_mutex_unlock(&TimeMutex);

		return true;
	}

	struct timeval  now;
	struct timespec timeout;
	unsigned long   duration = msec;
	unsigned long   end      = CCTime::Tick() + msec;

	pthread_mutex_lock(&TimeMutex);

	while (m->count <= 0) 
	{
		gettimeofday(&now, NULL);

		duration       += now.tv_usec / 1000;
		timeout.tv_sec  = now.tv_sec + duration / 1000;
		timeout.tv_nsec = (duration % 1000) * 1000000;

		if (pthread_cond_timedwait(&m->cond, &TimeMutex, &timeout) == ETIMEDOUT)
		{
			pthread_mutex_unlock(&TimeMutex);

			return false;
		}

		duration = end - CCTime::Tick();
	}

	m->count--;
	pthread_mutex_unlock(&TimeMutex);

	return true;

#endif

}

*/





void threadsafe CCSemaphore::Give(void)
{
	ASSERT(FlagCreate() == true);

#if defined (_WIN32)

	::ReleaseSemaphore(m->addr, 1, NULL);

#elif defined (_LINUX)

	pthread_mutex_lock(&TimeMutex);
	m->count++;
	pthread_cond_signal(&m->cond);
	pthread_mutex_unlock(&TimeMutex);

#endif

}
