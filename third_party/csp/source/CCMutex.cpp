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
 * \brief CCMutex class source file
 * \author Young-il Choi <duddlf.choi@samsung.com>
 * \date 2006-07-28
 */




#if defined (_WIN32)

#include <windows.h>

#endif

#if defined (_LINUX)

#include <errno.h>
#include <semaphore.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>

#endif




#include <CSP.h>

#include "CCMutexDefine.h"




bool CCMutex::Create(void)
{
	ASSERT(FlagCreate() == false);
	
	try
	{
		m = new CTMutexMember;
		if (m == NULL) throw 0;
	}
	catch (...)
	{
		return false;
	}

#if defined (_WIN32)

	m->handle = ::CreateSemaphore(NULL, 1, 0x7fffffff, NULL);

#elif defined (_LINUX)

	pthread_mutex_init(&m->mutex, NULL);

#endif

	m->threadId = 0;

	THREADSAFE_CHECK_INITIALIZE

	return true;
}




void CCMutex::Destroy(void)
{
	ASSERT(FlagCreate() == true);

#if defined(_USE_THREADSAFE_CHECK)

	ASSERT(m->__check_id == CCThread::SelfId());

#endif

#if defined (_WIN32)

	::CloseHandle(m->handle);

#elif defined (_LINUX)

	pthread_mutex_destroy(&m->mutex);

#endif

	delete m;
	m = NULL;

	return;
}




void threadsafe CCMutex::Lock(void)
{
	ASSERT(FlagCreate() == true);

	int id;

    if (m->threadId == (id = CCThread::SelfId()))
    {
        m->count++;

        return;
    }

#if defined (_WIN32)

	::WaitForSingleObject(m->handle, INFINITE);

#elif defined (_LINUX)

	pthread_mutex_lock(&m->mutex);

#endif

	m->threadId = id;
	m->count    = 1;

	return;
}




void threadsafe CCMutex::Unlock(void)
{
	ASSERT(FlagCreate() == true);
	ASSERT(m->threadId == CCThread::SelfId());

	if (--(m->count) > 0)
	{
		return;
	}

	m->threadId = 0;

#if defined (_WIN32)

	::ReleaseSemaphore(m->handle, 1, NULL);

#elif defined (_LINUX)

	pthread_mutex_unlock(&m->mutex);

#endif

	return;
}




#if defined (_USE_COMPAT_11)

bool threadsafe CCMutex::Try(unsigned long msec)
{
	if (msec == (unsigned long) INFINITY)
	{
		Lock();

		return true;
	}

	return false;
}

#endif
