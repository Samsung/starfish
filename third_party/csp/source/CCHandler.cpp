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
 * \brief CCHandler class source file
 * \author Young-il Choi <duddlf.choi@samsung.com>
 * \date 2006-07-28
 */




#include <CSP.h>

#include "CCHandlerDefine.h"




bool CCHandler::t_Create(const class CCTask* task)
{
	ASSERT(t_FlagCreate() == false);
	ASSERT(task != NULL);

	try
	{
		m = new CTHandlerMember;
		if (m == NULL) throw 0;
	}
	catch (...)
	{
		return false;
	}

	m->task = (CCTask*)task;

	THREADSAFE_CHECK_INITIALIZE

	return true;
}




void CCHandler::t_Destroy(void)
{
	ASSERT(t_FlagCreate() == true);

    unsigned long count;

    // Delete events for this handler.
	m->task->Remove(this, &count);

	delete m;
	m = NULL;
}




CCTask* CCHandler::Task(void)
{
//	THREADSAFE_CHECK

    ASSERT(t_FlagCreate() == true);

    return m->task;
}




void CCHandler::OnEvent(const CTEvent* event)
{
//	THREADSAFE_CHECK

    ASSERT(t_FlagCreate() == true);
    ASSERT(event != NULL);

    t_OnEvent(event);

    return;
}




