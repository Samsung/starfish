/*
* Copyright (c) 2018-present Samsung Electronics Co., Ltd
*
*  This library is free software; you can redistribute it and/or
*  modify it under the terms of the GNU Lesser General Public
*  License as published by the Free Software Foundation; either
*  version 2 of the License, or (at your option) any later version.
*
*  This library is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*  Lesser General Public License for more details.
*
*  You should have received a copy of the GNU Lesser General Public
*  License along with this library; if not, write to the Free Software
*  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
*  USA
*/

#include "StarFishConfig.h"
#include "StarFish.h"

namespace StarFish {

__declspec(thread) bool g_postLogMessageToThreadMessageQueue = false;

void forwardPrintingLogInfo(const char *fmt ...)
{
    char buf[1024];
    va_list myargs;
    va_start(myargs, fmt);
    int writtenLen = vsnprintf(buf, sizeof buf, fmt, myargs);
    va_end(myargs);
    if (writtenLen > 0) {
        OutputDebugStringA("[LOG_INFO]------------------------------\n");
        OutputDebugStringA(buf);
        OutputDebugStringA("----------------------------------------\n");

        if (g_postLogMessageToThreadMessageQueue) {
            void* buffer = LocalAlloc(LMEM_FIXED, writtenLen + 1);
            memcpy(buffer, buf, writtenLen + 1);
            PostMessage(NULL, 0x0409, (WPARAM)buffer, writtenLen);
        }
    }
}
void forwardPrintingLogError(const char *fmt ...)
{
    char buf[1024];
    va_list myargs;
    va_start(myargs, fmt);
    int writtenLen = vsnprintf(buf, sizeof buf, fmt, myargs);
    va_end(myargs);
    if (writtenLen > 0) {
        OutputDebugStringA("[LOG_ERROR]-----------------------------\n");
        OutputDebugStringA(buf);
        OutputDebugStringA("----------------------------------------\n");

        if (g_postLogMessageToThreadMessageQueue) {
            void* buffer = LocalAlloc(LMEM_FIXED, writtenLen + 1);
            memcpy(buffer, buf, writtenLen + 1);
            PostMessage(NULL, 0x0410, (WPARAM)buffer, writtenLen);
        }
    }
}
void forwardPrintingLogWarn(const char *fmt ...)
{
    char buf[1024];
    va_list myargs;
    va_start(myargs, fmt);
    int writtenLen = vsnprintf(buf, sizeof buf, fmt, myargs);
    va_end(myargs);
    if (writtenLen > 0) {
        OutputDebugStringA("[LOG_WRAN]------------------------------\n");
        OutputDebugStringA(buf);
        OutputDebugStringA("----------------------------------------\n");

        if (g_postLogMessageToThreadMessageQueue) {
            void* buffer = LocalAlloc(LMEM_FIXED, writtenLen + 1);
            memcpy(buffer, buf, writtenLen + 1);
            PostMessage(NULL, 0x0411, (WPARAM)buffer, writtenLen);
        }
    }
}
}