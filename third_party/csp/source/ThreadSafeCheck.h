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

#if defined (_USE_THREADSAFE_CHECK) 

#define	THREADSAFE_CHECK_PREPARE \
	int __check_id; 

#define THREADSAFE_CHECK_INITIALIZE \
	M->__check_id = PCThread::SelfId();

#define THREADSAFE_CHECK \
	if (M->__check_id != PCThread::SelfId()) \
	{ \
		PRINT("ThreadSafe Violation [%s:%d]", __FILE__, __LINE__); \
	} \

#else

#define THREADSAFE_CHECK_PREPARE
#define THREADSAFE_CHECK_INITIALIZE
#define THREADSAFE_CHECK 

#endif
