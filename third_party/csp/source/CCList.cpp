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
 * \brief CCList class source file
 * \author Young-il Choi <duddlf.choi@samsung.com>
 * \date 2004-12-31
 */
 



#include <CSP.h>

#include "CCListDefine.h"




bool CCList::Create(void)
{
	ASSERT(FlagCreate() == false);

	try
	{
		m = new CTListMember;
		if (m == NULL) throw 0;
	}
	catch (...)
	{
		return false;
	}

#if defined (_USE_COMPAT_11)
	if (CCMutex::Create() == false)
	{
		delete M;
		m = NULL;
		return false;
	}
#endif

	m->head = NULL;
	m->tail = NULL;
	m->cur  = NULL;

	m->dummy.data = NULL;

	m->size = 0;

	THREADSAFE_CHECK_INITIALIZE

	return true;
}




void CCList::Destroy(void)
{
	THREADSAFE_CHECK

	ASSERT(FlagCreate() == true);

#if defined (_USE_COMPAT_11)
	Lock();
#endif

	ASSERT(m->size == 0);

/*
	CTNode* node = m->head;
	CTNode* tnode;

	while(node != NULL) 
	{
		tnode = node->after;

		delete node;

		node = tnode;
	}
*/

    m->head = NULL;
    m->tail = NULL;
    m->cur  = NULL;

    m->size = 0;

#if defined (_USE_COMPAT_11)
	CCMutex::Destroy();
#endif

	delete m;
	m = NULL;
}




unsigned long CCList::Count(void)
{
	THREADSAFE_CHECK

	ASSERT(FlagCreate() == true);

	return m->size;
}




void* CCList::Data(void)
{
	THREADSAFE_CHECK

	ASSERT(FlagCreate() == true);

	if (m->cur == NULL)
	{
		return NULL;
	}

	return m->cur->data;
}




bool CCList::Add(const void* data, int mode)
{
	THREADSAFE_CHECK
	
	ASSERT(FlagCreate() == true);

	CTNode* node;
	
	try
	{
		node = new CTNode;
		if (node == NULL) throw 0;
	}
	catch (...)
	{
		return false;
	}

	node->data    = (void*)data;
	
	switch (mode)
	{
	case ADD_BEFORE:

		if (m->size == 0)
		{
			node->before = NULL;
			node->after  = NULL;

			m->head = node;
			m->tail = node;
		}
		else
		{
			if (m->cur != &m->dummy)
			{
				if (m->cur == m->head)
				{
					m->head = node;
				}

				node->before = m->cur->before;
				node->after  = m->cur;

				if (m->cur->before)
				{
					m->cur->before->after = node;
				}

				m->cur->before  = node;
			}
			else
			{
				if (m->cur->after == m->head)
				{
					m->head = node;
				}

				if (m->cur->before == m->tail)
				{
					m->tail = node;
				}
	
				node->before = m->cur->before;
				node->after = m->cur->after;

				if (m->cur->after)
				{
					m->cur->after->before = node;
				}
				if (m->cur->before)
				{
					m->cur->before->after = node;
				}

				m->cur->before = node;
			}
		}

		break;

	case ADD_AFTER:

		if (m->size == 0)
		{
			node->after  = NULL;
			node->before = NULL;
			
			m->head = node;
			m->tail = node;
		}
		else
		{
			if (m->cur != &m->dummy)
			{
				if (m->cur == m->tail)
				{
					m->tail = node;
				}
	
				node->after  = m->cur->after;
				node->before = m->cur;
	
				if (m->cur->after)
				{
					m->cur->after->before = node;
				}

				m->cur->after = node;
			}
			else
			{
				if (m->cur->before == m->tail)
				{
					m->tail = node;
				}

				if (m->cur->after == m->head)
				{
					m->head = node;
				}

				node->before = m->cur->before;
				node->after = m->cur->after;

				if (m->cur->after)
				{
					m->cur->after->before = node;
				}
				if (m->cur->before)
				{
					m->cur->before->after = node;
				}

				m->cur->after = node;
			}
		}

		break;
	}

	m->cur = node;
	m->size++;

	return true;
}




#if defined (_USE_COMPAT_11)

bool CCList::Find(const void* data)
{
	THREADSAFE_CHECK

	ASSERT(FlagCreate() == true);

	CTNode *node;

	for (node = m->head; node != NULL; node = node->after)
	{
		if (node->data == data)
		{
			m->cur = node;

			return true;
		}
	}

	return false;
}

#endif




bool CCList::SetPosition(int whence, int index)
{
	THREADSAFE_CHECK

	ASSERT(FlagCreate() == true);

	CTNode* node;
	int      pos;

	if (m->head == NULL)
	{
		return false;
	}

	switch (whence)
	{
	case POS_BEGIN:

		node = m->head;

		if (index < 0)
		{
			return false;
		}

		for (pos = 0; (pos < index) && (node->after != NULL); pos++)
		{
			node = node->after;
		}
		
		if (pos != index)
		{
			return false;
		}

		m->cur = node;

		break;

	case POS_CURRENT:

		node = m->cur;

		if (index < 0)
		{
			for (pos = index; (pos < 0) && (node->before != NULL); pos++)
			{
				node = node->before;	
			}
		
			if ((pos < 0) || (node == NULL))
			{
				return false;
			}

		}
		else
		{
			for (pos = 0; (pos < index) && (node->after != NULL); pos++)
			{
				node = node->after;
			}
			
			if ((pos != index) || (node == NULL))
			{
				return false;
			}
		}

		m->cur = node;

		break;

	case POS_END:

		node = m->tail;

		if (index > 0)
		{
			return false;
		}

		for (pos = index; (pos < 0) && (node->before != NULL); pos++)
		{
			node = node->before;	
		}
	
		if ((pos < 0) || (node == NULL))
		{
			return false;
		}

		m->cur = node;

		break;
	}

	if (m->cur == NULL)
	{
		return false;
	}

	return true;
}




void CCList::Remove(void)
{
	THREADSAFE_CHECK

	ASSERT(FlagCreate() == true);

	if (m->cur == NULL || m->cur == &m->dummy)
	{
		return;
	}

	m->dummy.before = m->cur->before;
	m->dummy.after = m->cur->after;
	m->dummy.data = NULL;

	if (m->cur->before != NULL)
	{
		m->cur->before->after = m->cur->after;
	}
	else
	{
		m->head = m->cur->after;
		if (m->head != NULL)
			m->head->before = NULL;
	}

	if (m->cur->after != NULL)
	{
		m->cur->after->before = m->cur->before;
	}
	else
	{
		m->tail = m->cur->before;
		if (m->tail != NULL)
			m->tail->after = NULL;
	}

	delete m->cur;

	m->size--;
	m->cur = &m->dummy;

	return;
}




#if defined (_USE_COMPAT_11)

void CCList::Delete(void)
{
	THREADSAFE_CHECK

	ASSERT(FlagCreate() == true);

	if (m->cur == NULL)
	{
		return;
	}

	CTNode* node;

	if (m->cur->before != NULL) 
	{
		m->cur->before->after = m->cur->after;
	}
	else 
	{
		m->head = m->cur->after;

		if (m->head != NULL) 
		{
			m->head->before = NULL;
		}
	}

	node = m->cur->after;

	if (m->cur->after != NULL)
	{
		m->cur->after->before = m->cur->before;
	}
	else
	{
		m->tail = m->cur->before;

		if (m->tail != NULL) 
		{
			m->tail->after = NULL;
		}
	}

	if (node == NULL)
	{
		node = m->cur->before;
	}

	delete m->cur;

	m->size--;
	m->cur = node;

	return;
}

#endif




