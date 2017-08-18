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
 * CCThread Implementation header file
 *
 * \author Young-il Choi <duddlf.choi@samsung.com>
 * \date 2006-07-28
 */




#include "ThreadSafeCheck.h"




struct CTThreadSync
{
	bool flag;
	CCMutex sync;
};




struct CTThreadMember
{
	bool flagDetach;
	long id;
	CCSemaphore sync[2];

#if defined (_LINUX)

	int pid;
	CCSemaphore pidSync;

#elif defined (_WIN32)

	int handle;

#endif
	CTEvent* event;
	bool priority;

	CTThreadSync* threadSync;

	THREADSAFE_CHECK_PREPARE
};




struct CTThreadArg
{
	void (*start)(void*);
	void* arg;
	CTThreadSync* threadSync;
	CTThreadMember* m;
};




