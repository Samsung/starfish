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
 * \brief CCQueue class source file
 * \author Young-il Choi <duddlf.choi@samsung.com>
 * \date 2006-07-28
 */




#include <CSP.h>
#include <CCQueue.cfg>

#include "CCQueueDefine.h"




bool CCQueue::Create(void)
{
	ASSERT(FlagCreate() == false);

	try
	{
		m = new CTQueueMember;
		if (m == NULL) throw 0;
	}
	catch (...)
	{
		return false;
	}

	if (m->mutex.Create() == false)
	{
		delete m;
		m = NULL;
		return false;
	}

	if (m->sem.Create(0) == false)
	{
		m->mutex.Destroy();

		delete m;
		m = NULL;
		return false;
	}

	m->in   = 0;
	m->out  = 0;
	m->size = 0;

	THREADSAFE_CHECK_INITIALIZE

	return true;
}




void CCQueue::Destroy(void)
{
	THREADSAFE_CHECK

	ASSERT(FlagCreate() == true);

	m->mutex.Lock();
	m->mutex.Destroy();
	m->sem.Destroy();

	delete m;
	m = NULL;
	return;
}




unsigned long CCQueue::Count(void)
{
    ASSERT(FlagCreate() == true);

    return m->size;
}




unsigned long CCQueue::Count(const CCHandler* receiver)
{
	ASSERT(FlagCreate() == true);

	unsigned long found = 0;

	m->mutex.Lock();

	unsigned long index = m->out;

	while (index != m->in)
	{
		if (m->event[index].receiver == receiver)
		{
			found++;
		}

		index = (index + 1) % CONFIG_QUEUE_SIZE;
	}

	m->mutex.Unlock();

	return found;
}




unsigned long CCQueue::Count(const CCHandler* receiver, unsigned long type)
{
	ASSERT(FlagCreate() == true);

	unsigned long found = 0;

	m->mutex.Lock();

	unsigned long index = m->out;

	while (index != m->in)
	{
		if ((m->event[index].receiver == receiver) && (m->event[index].type == type))
		{
			found++;
		}

		index = (index + 1) % CONFIG_QUEUE_SIZE;
	}

	m->mutex.Unlock();

	return found;
}




bool CCQueue::Put(CTEvent* event, bool wait, bool priority)
{
	ASSERT(FlagCreate() == true);

	m->mutex.Lock();

	if (((priority == false) ? (m->size + CONFIG_EMEGENCY_QUEUE_SIZE) : m->size) >= CONFIG_QUEUE_SIZE)
	{
		m->mutex.Unlock();

		PRINT("[CCQueue::Put] Event Queue Full [%s:%x]",
			event->receiver->Task()->Name(), event->receiver->Task()->Id());

		return false;
	}

	m->sem.Give();

	if (wait == false)
	{
		event->reserved = NULL;

		if (priority == true)
		{
			m->out           = (m->out + CONFIG_QUEUE_SIZE - 1) % CONFIG_QUEUE_SIZE;
			m->event[m->out] = *event;
		}
		else
		{
			m->event[m->in] = *event;
			m->in           = (m->in + 1) % CONFIG_QUEUE_SIZE;
		}

		m->size++;
		m->mutex.Unlock();
	}
	else
	{
		CCSemaphore waitSem;
		waitSem.Create(0);
		event->reserved = (void*)&waitSem;

		if (priority == true)
		{
			m->out           = (m->out + CONFIG_QUEUE_SIZE - 1) % CONFIG_QUEUE_SIZE;
			m->event[m->out] = *event;
		}
		else
		{
			m->event[m->in] = *event;
			m->in           = (m->in + 1) % CONFIG_QUEUE_SIZE;
		}

		m->size++;
		m->mutex.Unlock();

		waitSem.Take();
		waitSem.Destroy();
	}

	return true;
}




bool CCQueue::Get(CTEvent* event, unsigned long msec)
{
	ASSERT(FlagCreate() == true);

	unsigned long base = CCTime::Tick();
	unsigned long elapse = 0;

	while (true)
	{
		if (m->sem.Try(msec - elapse) == false)
		{
			return false;
		}

		m->mutex.Lock();

		if (m->size == 0)
		{
			m->mutex.Unlock();

			elapse += (CCTime::Tick() - base);

			if (elapse >= msec) return false;

			continue;
		}

		*event = m->event[m->out];
		m->out = (m->out + 1) % CONFIG_QUEUE_SIZE;
		m->size--;

		m->mutex.Unlock();

		return true;
	}
}




/*

bool CCQueue::Get(CTEvent* event, unsigned long msec)
{
	ASSERT(FlagCreate() == true);

	unsigned long end = CCTime::Tick() + msec;

	while (true)
	{
		if (m->sem.Try(end - CCTime::Tick()) == false)
		{
			return false;
		}

		m->mutex.Lock();

		if (m->size == 0)
		{
			m->mutex.Unlock();

			continue;
		}

		*event = m->event[m->out];
		m->out = (m->out + 1) % CONFIG_QUEUE_SIZE;
		m->size--;

		m->mutex.Unlock();

		return true;
	}
}

*/




bool CCQueue::Peek(CTEvent* event, unsigned long msec)
{
	ASSERT(FlagCreate() == true);

	unsigned long base = CCTime::Tick();
	unsigned long elapse = 0;

	while (true)
	{
		if (m->sem.Try(msec - elapse) == false)
		{
			return false;
		}

		m->mutex.Lock();

		if (m->size == 0)
		{
			m->mutex.Unlock();

			elapse += (CCTime::Tick() - base);

			if (elapse >= msec) return false;

			continue;
		}

		m->sem.Give();

		*event = m->event[m->out];

		m->mutex.Unlock();

		return true;
	}
}




/*
 
bool CCQueue::Peek(CTEvent* event, unsigned long msec)
{
	ASSERT(FlagCreate() == true);

	while (true)
	{
		if (m->sem.Try(msec) == false)
		{
			return false;
		}

		m->mutex.Lock();

		if (m->size == 0)
		{
			m->mutex.Unlock();

			continue;
		}

		m->sem.Give();

		*event = m->event[m->out];

		m->mutex.Unlock();

		return true;
	}
}

*/




void CCQueue::Remove(unsigned long* count)
{
	ASSERT(FlagCreate() == true);

	unsigned long found = 0;

	m->mutex.Lock();

	unsigned long size = m->size;

	m->size = 0;

	while (size-- != 0)
	{
		if (m->event[m->out].reserved != NULL)
		{
			((CCSemaphore*)(m->event[m->out].reserved))->Give();
		}
		found++;
		m->sem.Try();

		m->out = (m->out + 1) % CONFIG_QUEUE_SIZE;
	}

	m->mutex.Unlock();

	if (count != NULL)
		*count = found;
}




void CCQueue::Remove(const CCHandler* receiver, unsigned long* count)
{
	ASSERT(FlagCreate() == true);

	unsigned long found = 0;

	m->mutex.Lock();

	unsigned long size = m->size;

	m->size = 0;

	while (size-- != 0)
	{
		if (m->event[m->out].receiver == receiver)
		{
			if (m->event[m->out].reserved != NULL)
			{
				((CCSemaphore*)(m->event[m->out].reserved))->Give();
			}
			found++;
			m->sem.Try();
		}
		else
		{
			m->event[m->in] = m->event[m->out];
			m->in           = (m->in + 1) % CONFIG_QUEUE_SIZE;
			m->size++;
		}

		m->out = (m->out + 1) % CONFIG_QUEUE_SIZE;
	}

	m->mutex.Unlock();

	if (count != NULL)
		*count = found;
}




void CCQueue::Remove(const CCHandler* receiver, unsigned long type, unsigned long* count)
{
	ASSERT(FlagCreate() == true);
	ASSERT(receiver != NULL);

	unsigned long found = 0;

	m->mutex.Lock();

	unsigned long size = m->size;

	m->size = 0;

	while (size-- != 0)
	{
		if ((m->event[m->out].receiver == receiver) && (m->event[m->out].type == type))
		{
			if (m->event[m->out].reserved != NULL)
			{
				((CCSemaphore*)(m->event[m->out].reserved))->Give();
			}
			found++;
			m->sem.Try();
		}
		else
		{
			m->event[m->in] = m->event[m->out];
			m->in           = (m->in + 1) % CONFIG_QUEUE_SIZE;
			m->size++;
		}

		m->out = (m->out + 1) % CONFIG_QUEUE_SIZE;
	}

	m->mutex.Unlock();

	if (count != NULL)
		*count = found;
}




#if defined (_USE_COMPAT_11)

bool CCQueue::Find(const CCHandler* receiver, unsigned long* count)
{
	ASSERT(FlagCreate() == true);

	unsigned long found  = 0;

	m->mutex.Lock();

	unsigned long index = m->out;

	while (index != m->in)
	{
		if (m->event[index].receiver == receiver)
		{
			found++;
		}

		index = (index + 1) % CONFIG_QUEUE_SIZE;
	}

	m->mutex.Unlock();

	if (count != NULL)
	{
		*count = found;
	}

	return found != 0;
}




bool CCQueue::Find(const CCHandler* receiver, unsigned long type, unsigned long* count)
{
	ASSERT(FlagCreate() == true);

	unsigned long found  = 0;

	m->mutex.Lock();

	unsigned long index = m->out;

	while (index != m->in)
	{
		if ((m->event[index].receiver == receiver) && (m->event[index].type == type))
		{
			found++;
		}

		index = (index + 1) % CONFIG_QUEUE_SIZE;
	}

	m->mutex.Unlock();

	if (count != NULL)
	{
		*count = found;
	}

	return found != 0;
}




bool CCQueue::Delete(const CCHandler* receiver, unsigned long* count)
{
	ASSERT(FlagCreate() == true);

	unsigned long found = 0;

	m->mutex.Lock();

	unsigned long size = m->size;

	m->size = 0;

	while (size-- != 0)
	{
		if (m->event[m->out].receiver == receiver)
		{
			if (m->event[m->out].reserved != NULL)
			{
				((CCSemaphore*)(m->event[m->out].reserved))->Give();
			}
			found++;
			m->sem.Try();
		}
		else
		{
			m->event[m->in] = m->event[m->out];
			m->in           = (m->in + 1) % CONFIG_QUEUE_SIZE;
			m->size++;
		}

		m->out = (m->out + 1) % CONFIG_QUEUE_SIZE;
	}

	m->mutex.Unlock();

	if (count != NULL)
	{
		*count = found;
	}

	return found != 0;
}




bool CCQueue::Delete(const CCHandler* receiver, unsigned long type, unsigned long* count)
{
	ASSERT(FlagCreate() == true);

	unsigned long found = 0;

	m->mutex.Lock();

	unsigned long size = m->size;

	m->size = 0;

	while (size-- != 0)
	{
		if ((m->event[m->out].receiver == receiver) && (m->event[m->out].type == type))
		{
			if (m->event[m->out].reserved != NULL)
			{
				((CCSemaphore*)(m->event[m->out].reserved))->Give();
			}
			found++;
			m->sem.Try();
		}
		else
		{
			m->event[m->in] = m->event[m->out];
			m->in           = (m->in + 1) % CONFIG_QUEUE_SIZE;
			m->size++;
		}

		m->out = (m->out + 1) % CONFIG_QUEUE_SIZE;
	}

	m->mutex.Unlock();

	if (count != NULL)
	{
		*count = found;
	}

	return found != 0;
}

#endif




