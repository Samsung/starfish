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
 * \brief CCTimer class source file
 * \author Young-il Choi <duddlf.choi@samsung.com>
 * \date 2006-07-28
 */




#include <CSP.h>

#include <CCTimer.cfg>

#include "CCTimerDefine.h"




void TimerMain(void* arg)
{
	CTTimerMember* m = (CTTimerMember*) arg;

	m->sync.Lock();

	unsigned long progress = 0;
	bool          ret;

	while(true)
	{
		if ((progress < m->count) || (m->count == (unsigned long) INFINITY))
		{
			unsigned long interval = m->interval;

			m->sync.Unlock();

			ret = m->wait.Try(interval);

			m->sync.Lock();

			// Try에서 fail한 후 Lock 직전에 Reset 또는 Destroy가 일어난
			// 경우가 있을 수 있으므로 Lock을 획득한 후 한 번 더 check
			if (ret == false)
				ret = m->wait.Try(0);

			// Destroy가 호출된 경우
			if ((ret == true) && (m->destroy == true))
			{
				return;
			}

			// Reset이 호출된 경우
			if (ret == true)
			{
				progress = 0;

//				if (m->event != NULL)
//				{
//					m->event->param.l[0] = m->count;
//				}
			}
			// Alarm time-out이 발생하여 Alarm 이벤트를 보내야 하는 경우
			else
			{
				if (m->event != NULL)
				{
//					m->event->param.l[1] = progress;

					CCTask::SendEvent(m->event, false, m->priority);
				}
				progress++;
			}
		}
		else
		{
			m->sync.Unlock();

			m->wait.Take();

			if (m->destroy == true)
			{
				return;
			}

			progress = 0;

			m->sync.Lock();
		}
	}
}




bool CCTimer::Create(void)
{
	ASSERT(FlagCreate() == false);

	try
	{
		m = new CTTimerMember;
		if (m == NULL) throw 0;
	}
	catch (...)
	{
		return false;
	}

	if (m->sync.Create() == false)
	{
		delete m;
		m = NULL;
		return false;
	}

	if (m->wait.Create(0) == false)
	{
		m->sync.Destroy();
		delete m;
		m = NULL;
		return false;
	}

	m->interval = INFINITY;
	m->count    = INFINITY;
	m->state    = TIMER_STATE_STOP;
	m->destroy  = false;
	m->event    = NULL;

	if (m->thread.Create(TimerMain, m, CONFIG_TIMER_STACK_SIZE) == false)
	{
		m->wait.Destroy();
		m->sync.Destroy();
		delete m;
		m = NULL;
		return false;
	}

	THREADSAFE_CHECK_INITIALIZE

	return true;
}




void CCTimer::Destroy(void)
{
	THREADSAFE_CHECK

	ASSERT(FlagCreate() == true);
	ASSERT(m->event == NULL);

	m->sync.Lock();

	m->destroy = true;
	m->wait.Give();

	m->sync.Unlock();

	m->thread.Destroy();
	m->wait.Destroy();
	m->sync.Destroy();

	delete m;
	m = NULL;
}




void CCTimer::Start(unsigned long interval, unsigned long count)
{
	THREADSAFE_CHECK

	ASSERT(FlagCreate() == true);
	ASSERT(m->state == TIMER_STATE_STOP);

	m->sync.Lock();

	m->interval = (interval < CONFIG_TIMER_MIN_INTERVAL) ? CONFIG_TIMER_MIN_INTERVAL : interval;
	m->count    = count;
	m->state    = TIMER_STATE_START;

	m->wait.Give();
	m->sync.Unlock();
}



void CCTimer::Stop(void)
{
	THREADSAFE_CHECK

	ASSERT(FlagCreate() == true);
	ASSERT(m->state == TIMER_STATE_START);

	m->sync.Lock();

	m->interval = INFINITY;
	m->count    = INFINITY;
	m->state    = TIMER_STATE_STOP;

	m->wait.Give();
	m->sync.Unlock();
}




int CCTimer::State(void)
{
	THREADSAFE_CHECK

	ASSERT(FlagCreate() == true);

	return m->state;
}




bool CCTimer::SubscribeEvent(int condType, const CTEvent* event, bool priority)
{
	THREADSAFE_CHECK

	ASSERT(FlagCreate() == true);
	ASSERT(event != NULL);
	ASSERT(condType == CCTimer::COND_TIMER_REPORT)
	ASSERT(m->event == NULL);
	ASSERT(m->state == TIMER_STATE_STOP);

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

	return true;
}




bool CCTimer::UnsubscribeEvent(int condType, const CTEvent* event)
{
	THREADSAFE_CHECK

	ASSERT(FlagCreate() == true);
	ASSERT(condType == CCTimer::COND_TIMER_REPORT)
	ASSERT(m->event != NULL);
	ASSERT(m->state == TIMER_STATE_STOP);

	delete m->event;
	m->event = NULL;

	return true;
}




#if 0

void CCTimer::Reset(unsigned long interval, unsigned long count)
{
//	THREADSAFE_CHECK

#if defined(_USE_THREADSAFE_CHECK)
	ASSERT(m->__check_id == CCThread::SelfId());
#endif

	ASSERT(FlagCreate() == true);
	ASSERT(interval >= CONFIG_TIMER_MIN_INTERVAL);

	m->sync.Lock();

	m->interval = (interval < CONFIG_TIMER_MIN_INTERVAL) ? CONFIG_TIMER_MIN_INTERVAL : interval;
	m->count    = count;

	m->wait.Give();
	m->sync.Unlock();
}

#endif


