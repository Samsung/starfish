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

/*
 * CCErrorManager class function source file
 * (Subcomponent of CCError Implementation)
 *
 * \author Young-il Choi <duddlf.choi@samsung.com>
 * \date 2006-07-28
 */




#if (CONFIG_TLS_ERROR == 0)

#include <CSP.h>

#include "CCErrorManager.h"




bool CCErrorManager::Create(void)
{
	ASSERT(m_listLock.FlagCreate() == false);
	
	if (m_listLock.Create() == false)
	{
		return false;
	}

	if (m_list.Create() == false)
	{
		m_listLock.Destroy();
		return false;
	}

	if (AddItem() == false)
	{
		m_listLock.Destroy();
		m_list.Destroy();
		return false;
	}

	return true;
}




void CCErrorManager::Destroy(void)
{
	ASSERT(m_listLock.FlagCreate() == true);
	ASSERT(m_list.Count() == 1);

	DeleteItem();

	m_list.Destroy();
	m_listLock.Destroy();
}




bool CCErrorManager::AddItem()
{
	ASSERT(m_listLock.FlagCreate() == true);

	m_listLock.Lock();

	long id = CCThread::SelfId();
	CTError* error;

	if (m_list.SetPosition(CCList::POS_BEGIN, 0))
	{
		do
		{
			error = (CTError*)m_list.Data();
			if (error->id == id)
			{
				m_listLock.Unlock();
				return false;
			}
		} while(m_list.SetPosition(CCList::POS_CURRENT, 1));
	}

	try
	{
		error = new CTError;
		if (error == NULL) throw 0;
	}
	catch (...)
	{
		m_listLock.Unlock();
		return false;
	}

	error->id = id;
	error->num = CCError::ERROR_NONE;

	if (m_list.Add(error) == false)
	{
		delete error;
		m_listLock.Unlock();
		return false;
	}

	m_listLock.Unlock();
	return true;
}




void CCErrorManager::DeleteItem()
{
	ASSERT(m_listLock.FlagCreate() == true);

	m_listLock.Lock();

	long id = CCThread::SelfId();
	CTError* error;

	if (m_list.SetPosition(CCList::POS_BEGIN, 0))
	{
		do
		{
			error = (CTError*)m_list.Data();
			if (error->id == id)
			{
				delete error;
				m_list.Remove();
				m_listLock.Unlock();
				return;
			}
		} while(m_list.SetPosition(CCList::POS_CURRENT, 1));
	}

	m_listLock.Unlock();
}




bool CCErrorManager::Set(int num)
{
	ASSERT(m_listLock.FlagCreate() == true);
	
	m_listLock.Lock();

	bool ret;
	long id = CCThread::SelfId();
	CTError* error;

	if (m_list.SetPosition(CCList::POS_BEGIN, 0))
	{
		do
		{
			error = (CTError*)m_list.Data();
			if (error->id == id)
			{
				error->num = num;
				m_listLock.Unlock();
				return true;
			}
		} while(m_list.SetPosition(CCList::POS_CURRENT, 1));
	}

	try
	{
		error = new CTError;
		if (error == NULL) throw 0;

		error->id = id;
		error->num = num;

		if (m_list.Add((void*) error) == false) 
		{
			delete error;
			throw 0;
		}

		ret = true;
	}
	catch(...)
	{
		ret = false;
	}

	m_listLock.Unlock();
	return ret;
}




int CCErrorManager::Num(void)
{
	ASSERT(m_listLock.FlagCreate() == true);

	m_listLock.Lock();

	long id = CCThread::SelfId();
	CTError* error;
	int num;

	if (m_list.SetPosition(CCList::POS_BEGIN, 0))
	{
		do
		{
			error = (CTError*)m_list.Data();
			if (error->id == id)
			{
				num = error->num;
				m_listLock.Unlock();
				return num;
			}
		} while(m_list.SetPosition(CCList::POS_CURRENT, 1));
	}

	m_listLock.Unlock();
	return CCError::ERROR_NONE;
}




#endif




