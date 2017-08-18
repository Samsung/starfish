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
 * \brief CCThread class source file
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

#include <CCThread.cfg>

#include "CCThreadDefine.h"

#include <CCError.cfg>

#if (CONFIG_TLS_ERROR == 0)

#include "CCErrorManager.h"

extern CCErrorManager g_errorManager;

#endif




#if defined (_USE_RA)

#include "Common/RADefines.h"
#include "Common/RAConfiguration.h"
#include "Wrapper/RAEventReceiver.h"
#include "Common/RAHashTable.h"
#include "Wrapper/RACall.h"
#include "Agent/Message/RAClassMsg.h"
#include "Wrapper/RACallMessage.h"
#include "Wrapper/RACallNetwork.h"
#include "Wrapper/RACallDummy.h"
#include "Wrapper/RAVirtualFrame.h"
#include "Common/RAInformation.h"


extern RAInformation g_Information;

#endif




#if defined (RA_CONFIG_TLS)

#if defined (_WIN32)

extern __declspec(thread) void *t_Call[];

#endif

#if defined (_LINUX)

extern __thread void *t_Call[];

#endif

#endif




#if defined (_WIN32)

static DWORD WINAPI StartThread(LPVOID param)
{
	CTThreadArg* threadArg = (CTThreadArg*) param;
	CTThreadMember* m = threadArg->m;

	m->sync[0].Give();
	
#if defined (RA_CONFIG_TLS)

	int i;

	for (i = 0; i < RAConfiguration::CATEGORY_MAX; i++)
	{
		t_Call[i] = NULL;
	}

#endif

	(*threadArg->start)(threadArg->arg);

#if defined (RA_CONFIG_TLS)

	int commType;

	for (i = 0; i < RAConfiguration::CATEGORY_MAX; i++)
	{
		commType = g_Information.CommuncationType(i);

		if (commType == RAInformation::TYPE_MESSAGE)
		{
			if (t_Call[i] != NULL)
			{
				((RACallMessage *) t_Call[i])->Destroy();
				delete (RACallMessage*) t_Call[i];
			}
		}
		else if (commType == RAInformation::TYPE_NETWORK)
		{
			if (t_Call[i] != NULL)
			{
				((RACallNetwork *) t_Call[i])->Destroy();
				delete (RACallNetwork*) t_Call[i];
			}
		}
		else
		{
			if (t_Call[i] != NULL)
			{
				((RACallDummy *) t_Call[i])->Destroy();
				delete (RACallDummy*) t_Call[i];
			}
		}
	}

#endif

	bool flagDestroy = false;

	threadArg->threadSync->sync.Lock();
	if (threadArg->threadSync->flag)
	{
		flagDestroy = true;
	}
	else
	{
		threadArg->threadSync->flag = true;
		if (m->event != NULL)
		{
			CCTask::SendEvent(m->event, false, m->priority);
		}
		m->sync[1].Give();
	}
	threadArg->threadSync->sync.Unlock();

	if (flagDestroy)
	{
		threadArg->threadSync->sync.Destroy();
		delete (CTThreadSync*) threadArg->threadSync;
	}

	delete (CTThreadArg*) threadArg;

	return 0;
}

#elif defined (_LINUX)

static void* StartThread(void* param)
{
	CTThreadArg* threadArg = (CTThreadArg*) param;
	CTThreadMember* m = threadArg->m;

	m->pid = getpid();
	m->pidSync.Give();

	m->sync[0].Give();

#if (CONFIG_TLS_ERROR == 0)
	g_errorManager.AddItem();
#endif

#if defined (RA_CONFIG_TLS)

	int i;

	for (i = 0; i < RAConfiguration::CATEGORY_MAX; i++)
	{
		t_Call[i] = NULL;
	}

#endif

	(*threadArg->start)(threadArg->arg);

#if defined (RA_CONFIG_TLS)

	for (i = 0; i < RAConfiguration::CATEGORY_MAX; i++)
	{
		if (t_Call[i] != NULL)
		{
			((RACallNetwork *) t_Call[i])->Destroy();
			delete (RACallNetwork*) t_Call[i];
		}
	}

#endif

#if (CONFIG_TLS_ERROR == 0)
	g_errorManager.DeleteItem();
#endif

	bool flagDestroy = false;

	threadArg->threadSync->sync.Lock();
	if (threadArg->threadSync->flag)
	{
		flagDestroy = true;
	}
	else
	{
		threadArg->threadSync->flag = true;
		if (m->event != NULL)
		{
			CCTask::SendEvent(m->event, false, m->priority);
		}
		m->sync[1].Give();
	}
	threadArg->threadSync->sync.Unlock();

	if (flagDestroy)
	{
		threadArg->threadSync->sync.Destroy();
		delete (CTThreadSync*) threadArg->threadSync;
	}

	delete (CTThreadArg*) threadArg;

	return NULL;
}

#endif




bool CCThread::Create(void (*threadMain)(void*), void* arg, unsigned long stackSize)
{
	ASSERT(FlagCreate() == false);

	if (stackSize == 0)
		stackSize = CONFIG_THREAD_STACK_SIZE;

	CTThreadSync *threadSync = NULL;
	CTThreadArg	*threadArg = NULL;

	try
	{
		threadSync = new CTThreadSync;
		if (threadSync == NULL) throw 0;
		threadSync->flag = false;
		if (threadSync->sync.Create() == false) throw 0;

		m = new CTThreadMember;
		if (m == NULL) throw 0;
		if (m->sync[0].Create(0) == false) throw 0;
		if (m->sync[1].Create(0) == false) throw 0;
#if defined (_LINUX)
		if (m->pidSync.Create(0) == false) throw 0;
		m->pid = -1;
#endif
		m->flagDetach = false;
		m->event = NULL;
		m->threadSync = threadSync;

		threadArg = new CTThreadArg;
		if (threadArg == NULL) throw 0;
		threadArg->start = threadMain;
		threadArg->arg = arg;
		threadArg->threadSync = threadSync;
		threadArg->m = m;

#if defined (_WIN32)
		m->handle = (int)CreateThread(NULL, (unsigned int)stackSize, StartThread, (LPVOID)threadArg, 0, (DWORD*)&m->id);
		if (m->handle == 0) throw 0;
#endif

#if defined (_LINUX)
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		pthread_attr_setstacksize(&attr, stackSize);

		if (pthread_create((pthread_t*)&m->id, &attr, StartThread, threadArg) != 0)
		{
			pthread_attr_destroy(&attr);
			throw 0;
		}
#endif
	}
	catch (...)
	{
		if (threadArg != NULL)
		{
			delete threadArg;
		}

		if (m != NULL)
		{
#if defined (_LINUX)
			if (m->pidSync.FlagCreate())
				m->pidSync.Destroy();
#endif
			if (m->sync[0].FlagCreate())
				m->sync[0].Destroy();
			if (m->sync[1].FlagCreate())
				m->sync[1].Destroy();
			delete m;
		}

		if (threadSync != NULL)
		{
			if (threadSync->sync.FlagCreate())
				threadSync->sync.Destroy();

			delete threadSync;
		}

		return false;
	}

	m->sync[0].Take();

	THREADSAFE_CHECK_INITIALIZE

	return true;
}




void CCThread::Destroy(void)
{
	THREADSAFE_CHECK

	ASSERT(FlagCreate() == true);
	ASSERT(CCThread::SelfId() != Id());
	ASSERT(m->event == NULL);

	bool flagDestroy = false;

	if (CCThread::SelfId() != Id())
	{
		if (m->flagDetach == false)
		{
			m->sync[1].Take();
		}

		m->threadSync->sync.Lock();
		if (m->threadSync->flag)
		{
			flagDestroy = true;
		}
		else
		{
			m->threadSync->flag = true;
		}
		m->threadSync->sync.Unlock();

		if (flagDestroy)
		{
			m->threadSync->sync.Destroy();
			delete (CTThreadSync*) m->threadSync;
		}

		m->sync[0].Destroy();
		m->sync[1].Destroy();
	}

#if defined (_LINUX)
	m->pidSync.Destroy();
#endif
#if defined (_WIN32)
	CloseHandle((HANDLE)m->handle);
#endif

	delete m;
	m = NULL;
}




bool CCThread::SubscribeEvent(int condType, const CTEvent* event, bool priority)
{
	THREADSAFE_CHECK

	ASSERT(FlagCreate() == true);
	ASSERT(event != NULL);
	ASSERT(condType == CCThread::COND_THREAD_EXIT_REPORT);
	ASSERT(m->event == NULL);

	try
	{
		m->event = new CTEvent;
		if (m->event == NULL) throw 0;
	}
	catch (...)
	{
		return false;
	}

	*m->event = *event;

	m->priority = priority;

	bool flagAlreadyDead = false;

	m->threadSync->sync.Lock();
	if (m->threadSync->flag)
	{
		flagAlreadyDead = true;
	}
	m->threadSync->sync.Unlock();

	if (flagAlreadyDead)
	{
		CCTask::SendEvent(m->event, false, m->priority);
	}

	return true;
}




bool CCThread::UnsubscribeEvent(int condType, const CTEvent* event)
{
	THREADSAFE_CHECK

	ASSERT(FlagCreate() == true);
	ASSERT(condType == CCThread::COND_THREAD_EXIT_REPORT);
	ASSERT(m->event != NULL)

	delete m->event;

	m->event = NULL;	

	return true;
}




bool CCThread::Detach(void)
{
	THREADSAFE_CHECK

    ASSERT(FlagCreate() == true);

	m->flagDetach = true;

	return true;
}




long CCThread::Id(void)
{
	THREADSAFE_CHECK

    ASSERT(FlagCreate() == true);

    return m->id;
}




bool CCThread::SetPriority(int priority)
{
	THREADSAFE_CHECK

	ASSERT(FlagCreate() == true);

#if defined (_LINUX)

	if (m->pid == -1)
	{
		m->pidSync.Take();
	}

	switch (priority)
	{
		case PRIORITY_HIGH:
			setpriority(PRIO_PROCESS, m->pid, -10);
			break;
		case PRIORITY_NORMAL:
			setpriority(PRIO_PROCESS, m->pid, 0);
			break;
		case PRIORITY_LOW:
			setpriority(PRIO_PROCESS, m->pid, 10);
			break;
		default:
			return false;
	}

#elif defined (_WIN32)

	switch (priority)
	{
		case PRIORITY_HIGH:
			return SetThreadPriority((HANDLE)m->handle, THREAD_PRIORITY_ABOVE_NORMAL) != 0;
		case PRIORITY_NORMAL:
			return SetThreadPriority((HANDLE)m->handle, THREAD_PRIORITY_NORMAL) != 0;
		case PRIORITY_LOW:
			return SetThreadPriority((HANDLE)m->handle, THREAD_PRIORITY_BELOW_NORMAL) != 0;
		default:
			return false;
	}

#endif

	return true;
}




long CCThread::SelfId(void)
{
#if defined (_WIN32)

	return (int)GetCurrentThreadId();

#elif defined (_LINUX)

    return (int)pthread_self();

#endif
}




bool CCThread::SetSelfPriority(int priority)
{
#if defined (_LINUX)

	switch (priority)
	{
		case CCThread::PRIORITY_HIGH:
			setpriority(PRIO_PROCESS, getpid(), -10);
			break;
		case CCThread::PRIORITY_NORMAL:
			setpriority(PRIO_PROCESS, getpid(), 0);
			break;
		case CCThread::PRIORITY_LOW:
			setpriority(PRIO_PROCESS, getpid(), 10);
			break;
		default:
			return false;
	}

#elif defined (_WIN32)

	switch (priority)
	{
		case CCThread::PRIORITY_HIGH:
			return SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL) != 0;
		case CCThread::PRIORITY_NORMAL:
			return SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL) != 0;
		case CCThread::PRIORITY_LOW:
			return SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL) != 0;
		default:
			return false;
	}

#endif

	return true;
}




#if defined (_USE_COMPAT_11)

void CallMain(void* thread)
{
	((CCThread*)thread)->t_Main();
}




bool CCThread::Create(unsigned long stackSize)
{
	ASSERT(FlagCreate() == false);

	if (stackSize == 0)
		stackSize = CONFIG_THREAD_STACK_SIZE;

	CTThreadSync *threadSync = NULL;
	CTThreadArg	*threadArg = NULL;

	try
	{
		threadSync = new CTThreadSync;
		if (threadSync == NULL) throw 0;
		threadSync->flag = false;
		if (threadSync->sync.Create() == false) throw 0;

		m = new CTThreadMember;
		if (m == NULL) throw 0;
		if (m->sync[0].Create(0) == false) throw 0;
		if (m->sync[1].Create(0) == false) throw 0;
#if defined (_LINUX)
		if (m->pidSync.Create(0) == false) throw 0;
		m->pid = -1;
#endif
		m->flagDetach = false;
		m->event = NULL;
		m->threadSync = threadSync;

		threadArg = new CTThreadArg;
		if (threadArg == NULL) throw 0;
		threadArg->start = CallMain;
		threadArg->arg = this;
		threadArg->threadSync = threadSync;
		threadArg->m = M;

#if defined (_WIN32)
		m->handle = (int)CreateThread(NULL, (unsigned int)stackSize, StartThread, (LPVOID)threadArg, 0, (DWORD*)&m->id);
		if (m->handle == 0) throw 0;
#endif

#if defined (_LINUX)
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		pthread_attr_setstacksize(&attr, stackSize);

		if (pthread_create((pthread_t*)&m->id, &attr, StartThread, threadArg) != 0)
		{
			pthread_attr_destroy(&attr);
			throw 0;
		}
#endif
	}
	catch (...)
	{
		if (threadArg != NULL)
		{
			delete threadArg;
		}

		if (m != NULL)
		{
#if defined (_LINUX)
			if (m->pidSync.FlagCreate())
				m->pidSync.Destroy();
#endif
			if (m->sync[0].FlagCreate())
				m->sync[0].Destroy();
			if (m->sync[1].FlagCreate())
				m->sync[1].Destroy();
			delete m;
		}

		if (threadSync != NULL)
		{
			if (threadSync->sync.FlagCreate())
				threadSync->sync.Destroy();

			delete threadSync;
		}

		return false;
	}

	m->sync[0].Take();

	THREADSAFE_CHECK_INITIALIZE

	return true;
}




void CCThread::t_Main(void)
{
	return;
}

#endif




