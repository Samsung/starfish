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
 * \ingroup csp
 * \brief main() function source file
 * \author Youngil Choi <duddlf.choi@samsung.com>
 * \date 2006-12-01
 */




#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#if defined (_LINUX)

#include <execinfo.h>

#endif

#include <CSP.h>




#if !defined (_LINUX)
#error OS not specified
#endif




#if defined (_BUILD_INFO)
char* BUILD_INFO_STRING = _BUILD_INFO;
#endif




CCList        g_taskList;
CCMutex       g_taskMutex;




#include "CCError.cfg"




#if (CONFIG_TLS_ERROR == 0)

#include "CCErrorManager.h"

CCErrorManager g_errorManager;

#endif



void _PRINT(const char* format, ...)
{
    va_list vaList;
    va_start(vaList, format);
    vprintf(format, vaList);
    va_end(vaList);

    putchar('\n');
}




void _ASSERT(const char* expression, const char* file, unsigned long line)
{
	char* taskName = NULL;

	if (g_taskList.FlagCreate() == true)
	{
		g_taskMutex.Lock();

		if (g_taskList.SetPosition(CCList::POS_BEGIN, 0) == true)
		{
			do
			{
				CCTask* task = (CCTask*)g_taskList.Data();

				if (task->Id() == CCThread::SelfId())
				{
					taskName = (char*)task->Name();

					break;
				}

			} while (g_taskList.SetPosition(CCList::POS_CURRENT, 1) == true);
		}

		g_taskMutex.Unlock();
	}

	PRINT("ASSERT: %s (%lu) - %s [%s]", file, line, expression, taskName);

#if defined (_LINUX)

	void*  btrace[50];
	size_t size;
	int    fd = fileno(stderr);

	size = backtrace(btrace, 50);
	fprintf(stderr, "Call stack:\n");
	backtrace_symbols_fd(btrace, size, fd);

#endif

	CCTime::Sleep(100);

	abort();

	return;
}



/*
#if defined (_LINUX)

int main(void)

#endif
{
#if (CONFIG_TLS_ERROR == 0)
	g_errorManager.Create();
	ASSERT(g_errorManager.FlagCreate() == true);
#endif

	g_taskMutex.Create();
	ASSERT(g_taskMutex.FlagCreate() == true);

	g_taskList.Create();
	ASSERT(g_taskList.FlagCreate() == true);

	PRINT("  SAMSUNG (R) Core SW Platform Version 2.0");
	PRINT("Copyright (C) SAMSUNG Electronics Co., Ltd.");
	PRINT("");

	int ret = Main();

	g_taskList.Destroy();
	g_taskMutex.Destroy();

#if (CONFIG_TLS_ERROR == 0)
	g_errorManager.Destroy();
#endif

	return ret;
}
*/

void init_csp()
{
#if (CONFIG_TLS_ERROR == 0)
	g_errorManager.Create();
	ASSERT(g_errorManager.FlagCreate() == true);
#endif

	g_taskMutex.Create();
	ASSERT(g_taskMutex.FlagCreate() == true);

	g_taskList.Create();
	ASSERT(g_taskList.FlagCreate() == true);
}

void deinit_csp()
{
	g_taskList.Destroy();
	g_taskMutex.Destroy();

#if (CONFIG_TLS_ERROR == 0)
	g_errorManager.Destroy();
#endif
}
