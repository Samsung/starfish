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
 * \brief CCString class source file
 * \author Young-il Choi <duddlf.choi@samsung.com>
 * \date 2006-07-28
 */




#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <CSP.h>




void CCString::Concate(char* dest, const char* source)
{
	ASSERT(dest != NULL);
	ASSERT(source != NULL);

	strcat(dest, source);

	return;
}




void CCString::Concate(char* dest, const char* source, unsigned long size) 
{
	ASSERT(dest != NULL);
	ASSERT(source != NULL);

	strncat(dest, source, size);

	return;
}




long CCString::Diff(const char* dest, const char* source)
{ 
	ASSERT(dest != NULL);
	ASSERT(source != NULL);

	return strcmp(dest, source);
}




long CCString::Diff(const char* dest, const char* source, unsigned long size) 
{
	ASSERT(dest != NULL);
	ASSERT(source != NULL);

	return strncmp(dest, source, size);
}




void CCString::Copy(char* dest, const char* source) 
{
	ASSERT(dest != NULL);
	ASSERT(source != NULL);

	strcpy(dest, source);

	return;
}




void CCString::Copy(char* dest, const char* source, unsigned long size) 
{
	ASSERT(dest != NULL);
	ASSERT(source != NULL);

	strncpy(dest, source, size);

	return;
}




unsigned long CCString::Length(const char* str) 
{
	ASSERT(str != NULL);

	return strlen(str);
}




char* CCString::Char(const char* str, char ch)
{
	ASSERT(str != NULL);

	return strchr(str, ch);

	return NULL;
}




void CCString::Print(char* str, const char* format, ...)
{
	ASSERT(str != NULL);

	va_list arg;

	va_start(arg, format);
	vsprintf(str, format, arg);
	va_end(arg);

	return;
}




int CCString::Integer(const char* str)
{
	ASSERT(str != NULL);

	return atoi(str);
}




