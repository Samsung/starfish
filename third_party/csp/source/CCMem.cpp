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
 * \brief CCMem class source file
 * \author Young-il Choi <duddlf.choi@samsung.com>
 * \date 2006-07-28
 */




#include <string.h>

#include <CSP.h>




void CCMem::Set(void* dest, int value, unsigned long size)
{
	ASSERT(dest != NULL);

	memset(dest, value, size);

	return;
}




long CCMem::Diff(const void* dest, const void* source, unsigned long size)
{
	ASSERT(dest != NULL);
	ASSERT(source != NULL);

	return memcmp(dest, source, size); 
}




void CCMem::Copy(void* dest, const void* source, unsigned long size) 
{ 
	ASSERT(dest != NULL);
	ASSERT(source != NULL);

	memcpy(dest, source, size); 

	return;
}




void CCMem::Move(void* dest, const void* source, unsigned long size) 
{ 
	ASSERT(dest != NULL);
	ASSERT(source != NULL);

	memmove(dest, source, size);

	return;
}




