#if defined(STARFISH_WINDOWS)
/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
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

#include "StarfishConfig.h"
#include "Starfish.h"

#define _WIN32_WINNT _WIN32_WINNT_WIN7
#include <Windows.h>
#include <ShellAPI.h>
#include <KnownFolders.h>
#include <ShlObj.h>
#include <stdio.h>
#include <stdarg.h>

namespace Starfish {

const char* getWindowsTempDir()
{
    static char pBuffer[MAX_PATH];
    if (!pBuffer[0]) {
        GetEnvironmentVariableA("LOCALAPPDATA", pBuffer, sizeof pBuffer);
        strcat(pBuffer, "\\");
    }
    return pBuffer;
}

__declspec(thread) bool g_postLogMessageToThreadMessageQueue = false;

// OutputDebugStringA alone is invisible without a debugger attached, which
// left every engine-side diagnostic unreachable from a console host -- the
// whole blank-render investigation on this port was blind until these lines
// reached stderr. Warnings and errors always go there; the (very chatty) info
// stream only when STARFISH_LOG_STDERR is set.
static void emit(const char* level, const char* text, bool isInfo)
{
    static int infoEnabled = -1;
    if (isInfo) {
        if (infoEnabled < 0) {
            infoEnabled =
                GetEnvironmentVariableA("STARFISH_LOG_STDERR", nullptr, 0) != 0;
        }
        if (!infoEnabled) {
            return;
        }
    }
    fprintf(stderr, "%s %s\n", level, text);
    fflush(stderr);
}

void forwardPrintingLogInfo(const char* fmt...)
{
    char buf[4096];
    va_list myargs;
    va_start(myargs, fmt);
    int writtenLen = vsnprintf(buf, sizeof buf, fmt, myargs);
    va_end(myargs);
    if (writtenLen > 0) {
        emit("[LOG_INFO]", buf, true);
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
void forwardPrintingLogError(const char* fmt...)
{
    char buf[4096];
    va_list myargs;
    va_start(myargs, fmt);
    int writtenLen = vsnprintf(buf, sizeof buf, fmt, myargs);
    va_end(myargs);
    if (writtenLen > 0) {
        emit("[LOG_ERROR]", buf, false);
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
void forwardPrintingLogWarn(const char* fmt...)
{
    char buf[4096];
    va_list myargs;
    va_start(myargs, fmt);
    int writtenLen = vsnprintf(buf, sizeof buf, fmt, myargs);
    va_end(myargs);
    if (writtenLen > 0) {
        emit("[LOG_WARN]", buf, false);
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
} // namespace Starfish
#endif
