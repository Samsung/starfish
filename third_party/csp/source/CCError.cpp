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
 * \brief CCError class source file
 * \author Young-il Choi <duddlf.choi@samsung.com>
 * \date 2006-07-28
 */




#include <CSP.h>

#include "CCError.cfg"

#if !(CONFIG_TLS_ERROR == 0)



// TLS supports only Win32 and Linux latest kernel
#if defined (_LINUX)

__thread int tls_error;


#else 
#if defined (_WIN32)

__declspec(thread) int tls_error;

#else

#error Thread-Local-Storage not supported

#endif

#endif



#else

#include "CCErrorManager.h"

extern CCErrorManager g_errorManager;

#endif




void CCError::Set(int number)
{
#if !(CONFIG_TLS_ERROR == 0)

	tls_error = number;

#else

	g_errorManager.Set(number);

#endif

}




int CCError::Num(void)
{
#if !(CONFIG_TLS_ERROR == 0)

	return tls_error;

#else

	return g_errorManager.Num();

#endif
}




