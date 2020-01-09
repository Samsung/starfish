/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#include "StarfishConfig.h"
#include "core/page/NavigatorMixin.h"
#include "core/dom/ExecutionContext.h"
#include "core/page/WebBase.h"

#if defined(OS_WINDOWS)
#include <Windows.h>
#else
#include <sys/utsname.h>
#endif

namespace Starfish {

NavigatorMixin::NavigatorMixin(ExecutionContext* executionContext)
    : m_executionContext(executionContext)
{
    STARFISH_ASSERT(executionContext != nullptr);
}

String* NavigatorMixin::userAgent()
{
    return m_executionContext->webBase()->userAgent();
}

String* NavigatorMixin::platform()
{
    StringBuilder platformName;
#if !defined(OS_WINDOWS)
    // Unix-like systems
    struct utsname osname;
    if (uname(&osname) == 0) {
        platformName.appendString(osname.sysname, strlen(osname.sysname));
        platformName.appendString(String::spaceString);
        platformName.appendString(osname.machine, strlen(osname.machine));
    }
#else
    OSVERSIONINFO info;
    ZeroMemory(&info, sizeof(OSVERSIONINFO));
    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&info);

    platformName.appendString("Windows ");
    platformName.appendChar((char32_t)(info.dwMajorVersion + '0'));
    platformName.appendChar(' ');
    platformName.appendChar((char32_t)(info.dwMinorVersion + '0'));
#endif
    return platformName.finalize();
}

String* NavigatorMixin::language()
{
    return String::fromUTF8(executionContext()->webBase()->locale().data(),
                            executionContext()->webBase()->locale().size());
}
}
