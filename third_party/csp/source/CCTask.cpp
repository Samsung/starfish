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
 * \brief CCTask class source file
 * \author Young-il Choi <duddlf.choi@samsung.com>
 * \date 2006-07-28
 */




#include <CSP.h>

#include <CCTask.cfg>

#include "CCTaskDefine.h"




extern CCList g_taskList;
extern CCMutex g_taskMutex;




void CCTask::m_ExecuteEventLoop(void* arg)
{
	CCTask* task = (CCTask*)arg;
	ASSERT(task->FlagCreate() == true);

	task->t_Create();

	while (task->ExecuteEvent() == true)
	{
	}

	task->t_Destroy();
}




void CCTask::t_Create(void)
{
    ASSERT(FlagCreate() == true);

    return;
}




void CCTask::t_Idle(void)
{
    ASSERT(FlagCreate() == true);

    return;
}




void CCTask::t_Destroy(void)
{
    ASSERT(FlagCreate() == true);

    return;
}




bool CCTask::t_OnExit(const CTEvent* event)
{
    ASSERT(FlagCreate() == true);
    ASSERT(event != NULL);

    return true;
}




long CCTask::Id(void)
{
	THREADSAFE_CHECK

    ASSERT(FlagCreate() == true);

    return CCThread::Id();
}




const char* CCTask::Name(void)
{
	THREADSAFE_CHECK

    ASSERT(FlagCreate() == true);

    return m->name;
}




void CCTask::SetIdleTime(unsigned long msec)
{
	THREADSAFE_CHECK

    ASSERT(FlagCreate() == true);

    m->msec = msec;

    return;
}




bool CCTask::t_OnEvent(const CTEvent* event)
{
	ASSERT(FlagCreate() == true);
	ASSERT(event != NULL);

	bool    ret;
	CTEvent eventPing;

	switch (event->type)
	{
#if defined (_USE_COMPAT_11)
	case EVENT_EXIT:
		return OnExit(event);
	case EVENT_PING:
		return OnPing(event);
	case EVENT_USER:
		return OnUser(event);
#else
	case EVENT_EXIT:
		ret = t_OnExit(event);
		m->flagExit = true;
		return ret;
	case EVENT_PING:
		if (event->ret == 0 || (unsigned long) (event->ret) > CCTime::Tick())
		{
			eventPing.type = event->param.l[0];
			eventPing.receiver = (CCHandler*) (event->param.l[1]);
			return SendEvent(&eventPing, false, false);
		}
		else
		{
			return true;
		}
#endif
	} 

	return false;
}




bool CCTask::SendEvent(CTEvent* event, bool wait, bool priority)
{
	ASSERT(event != NULL);
	ASSERT(event->receiver != NULL);

	if ((wait == false) || (event->receiver->Task()->Id() != CCThread::SelfId()))
	{
		return event->receiver->Task()->Put(event, wait, priority);
	}

	if (priority == true)
	{
		event->receiver->OnEvent(event);

		return true;
	}

	ASSERT(!"Invalid Parameter");

	return false;
}




bool CCTask::Create(const char* name, unsigned long msec, unsigned long stackSize)
{
	ASSERT(FlagCreate() == false);
	ASSERT(name != NULL);

	if (msec == 0)
		msec = CONFIG_TASK_IDLE_TIME;
	if (stackSize == 0)
		stackSize = CONFIG_TASK_STACK_SIZE;

	try
	{
		m = new CTTaskMember;
		if (m == NULL) throw 0;
	}
	catch(...)
	{
		return false;
	}

	if (m->saveLock.Create() == false)
	{
		delete m;
		m = NULL;
		return false;
	}

	if (CCQueue::Create() == false)
	{
		m->saveLock.Destroy();
		delete m;
		m = NULL;
		return false;
	}

	if (CCHandler::t_Create(this) == false)
	{
		CCQueue::Destroy();

		m->saveLock.Destroy();
		delete m;
		m = NULL;
		return false;
	}

	CCMath::Random(0);

	m->msec     = msec;
	m->flagExit = false;

	CCString::Copy(m->name, name, CONFIG_TASK_NAME_SIZE - 1);
	m->name[CONFIG_TASK_NAME_SIZE - 1] = 0;

#if defined (_USE_COMPAT_11)
	m->before[CALLBACK_EXIT] = NULL;
	m->before[CALLBACK_USER] = NULL;
	m->before[CALLBACK_INPUT] = NULL;
	m->after[CALLBACK_EXIT]  = NULL;
	m->after[CALLBACK_USER]  = NULL;
	m->after[CALLBACK_INPUT]  = NULL;
#endif

	if (CCThread::Create(m_ExecuteEventLoop, (void*)this, stackSize) == false)
	{
		CCHandler::t_Destroy();
		CCQueue::Destroy();

		m->saveLock.Destroy();
		delete m;
		m = NULL;
		return false;
	}

	g_taskMutex.Lock();
	g_taskList.SetPosition(CCList::POS_END, 0);
	g_taskList.Add(this);
	g_taskMutex.Unlock();

	THREADSAFE_CHECK_INITIALIZE

	return true;
}




void CCTask::Destroy(void)
{
	THREADSAFE_CHECK

	ASSERT(FlagCreate() == true);

	CCThread::Destroy();

	g_taskMutex.Lock();

/*
	if (g_taskList.Find(this))
		g_taskList.Remove();
*/

	if (g_taskList.SetPosition(CCList::POS_BEGIN, 0))
	{
		do
		{
			if (g_taskList.Data() == this)
			{
				g_taskList.Remove();
				break;
			}
		} while (g_taskList.SetPosition(CCList::POS_CURRENT, 1));
	}

	g_taskMutex.Unlock();

	CCHandler::t_Destroy();
	CCQueue::Destroy();
	
    m->saveLock.Destroy();
	delete m;
	m = NULL;
}




void CCTask::Exit(void)
{
	THREADSAFE_CHECK

	ASSERT(FlagCreate() == true);

	CTEvent event;
	event.receiver = this;
	event.type = CCTask::EVENT_EXIT;

	CCTask::SendEvent(&event);
}




bool CCTask::ExecuteEvent(void)
{
	ASSERT(FlagCreate() == true);

	CTEvent event;
	if (Get(&event, m->msec) == false)
	{
		t_Idle();

		return true;
	}

	m->saveLock.Lock();
	m->saveEvent = event;
	m->saveLock.Unlock();

	event.receiver->OnEvent(&event);

	if (event.reserved != NULL)
	{
		((CCSemaphore*)(event.reserved))->Give();
	}
	
	if (m->flagExit == true)
	{
		return false;
	}

	return true;
}




void CCTask::GetExecutionEvent(CTEvent* event)
{
	THREADSAFE_CHECK

	m->saveLock.Lock();
	*event = m->saveEvent;
	m->saveLock.Unlock();
}




#if defined (_USE_COMPAT_11)

bool CCTask::t_OnUser(const CTEvent* event)
{
    ASSERT(FlagCreate() == true);
    ASSERT(event != NULL);

    return true;
}




bool CCTask::SetEventCallback(int type, CTEventCallback before, CTEventCallback after, const void* param)
{
	THREADSAFE_CHECK

	ASSERT(FlagCreate() == true);

	if (type != CALLBACK_EXIT &&
		type != CALLBACK_USER &&
		type != CALLBACK_INPUT)
	{
		CCError::Set(CCError::ERROR_INVALID);
		return false;
	}

	m->before[type] = before;
	m->after[type]  = after;
	m->param[type]  = param;

	return true;
}




bool CCTask::GetEventCallback(int type, CTEventCallback * before, CTEventCallback * after, const void **param)
{
	THREADSAFE_CHECK

	ASSERT(FlagCreate() == true);

	if (type != CALLBACK_EXIT &&
		type != CALLBACK_USER &&
		type != CALLBACK_INPUT)
	{
		CCError::Set(CCError::ERROR_INVALID);
		return false;
	}

	if (before != NULL)
		*before = m->before[type];
	if (after != NULL)
		*after = m->after[type];
	if (param != NULL)
		*param = m->param[type];

	return true;
}




bool CCTask::OnExit(const CTEvent* event)
{
	THREADSAFE_CHECK

	ASSERT(FlagCreate() == true);
	ASSERT(event != NULL);

	bool ret = false;

#if defined (_USE_COMPAT_11)
	if (m->before[CALLBACK_EXIT] != NULL)
	{
		ret = (m->before[CALLBACK_EXIT])(event, m->param[CALLBACK_EXIT]);
	}
#endif

	if (ret == false)
	{
		ret |= t_OnExit(event);
	}

#if defined (_USE_COMPAT_11)
	if (m->after[CALLBACK_EXIT] != NULL)
	{
		ret |= (m->after[CALLBACK_EXIT])(event, m->param[CALLBACK_EXIT]);
	}
#endif

	m->flagExit = true;

	return ret;
}




bool CCTask::OnUser(const CTEvent* event)
{
	THREADSAFE_CHECK

	ASSERT(FlagCreate() == true);
	ASSERT(event != NULL);

	bool ret = false;

#if defined (_USE_COMPAT_11)
	if (m->before[CALLBACK_USER] != NULL)
	{
		ret = (m->before[CALLBACK_USER])(event, m->param[CALLBACK_USER]);
	}
#endif

	if (ret == false)
	{
		ret |= t_OnUser(event);
	}

#if defined (_USE_COMPAT_11)
	if (m->after[CALLBACK_USER] != NULL)
	{
		ret |= (m->after[CALLBACK_USER])(event, m->param[CALLBACK_USER]);
	}
#endif

	return ret;
}




bool CCTask::OnPing(const CTEvent* event)
{
	THREADSAFE_CHECK

	ASSERT(FlagCreate() == true);
	ASSERT(event != NULL);

	CCSemaphore* sem = (CCSemaphore* )(event->param.l[0]);
	sem[0].Give();

	bool ret = sem[1].Try(1000);
	ASSERT(ret);

	sem[0].Destroy();
	sem[1].Destroy();
	delete [] sem;

	return true;
}




bool CCTask::Ping(unsigned long msec)
{
	THREADSAFE_CHECK

	// A task cannot ping itself.
	if (CCThread::SelfId() == Id())
	{
		return false;
	}

	// sem is destroyed and destructed by the pinged task.
	try
	{
		CCSemaphore* sem = new CCSemaphore[2];
		if (sem == NULL) throw 0;
	}
	catch(...)
	{
		return false;
	}

	if (sem[0].Create(0) == false)
	{
		delete [] sem;
		return false;
	}
	if (sem[1].Create(0) == false)
	{
		sem[0].Destroy();
		delete [] sem;
		return false;
	}

	CTEvent e;
	e.receiver = this;
	e.type = EVENT_PING;
	e.param.l[0] = (long)sem;

	if (SendEvent(&e, false, true) == false)
	{
		sem[0].Destroy();
		sem[1].Destroy();
		delete [] sem;
		return false;
	}

	bool ret = sem[0].Try(msec);
	sem[1].Give();

	return ret;
}




void CCTask::t_Main(void)
{
	return;
}

#endif




