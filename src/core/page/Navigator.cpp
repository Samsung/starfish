/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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
#include "Navigator.h"
#include "Starfish.h"
#include "core/dom/Document.h"
#include "core/modules/location/Geolocation.h"
#include "core/page/WebView.h"
#ifdef STARFISH_ENABLE_SERVICE_WORKER
#include "core/modules/serviceworker/ServiceWorkerContainer.h"
#endif
#if defined(OS_WINDOWS)
#include <Windows.h>
#else
#include <sys/utsname.h>
#endif

namespace Starfish {

Navigator::Navigator(Document* document)
    : ScriptWrappable(this, document->executionContext())
    , DocumentHoldable(document)
    , m_geolocation(nullptr)
#ifdef STARFISH_ENABLE_SERVICE_WORKER
    , m_serviceWorker(nullptr)
#endif
{
}

Geolocation* Navigator::geolocation()
{
    if (m_geolocation == nullptr) {
        m_geolocation = Geolocation::create(document());
    }
    return m_geolocation;
}

#ifdef STARFISH_ENABLE_SERVICE_WORKER
ServiceWorkerContainer* Navigator::serviceWorker()
{
    if (m_serviceWorker == nullptr) {
        m_serviceWorker = new ServiceWorkerContainer(document());
    }
    return m_serviceWorker;
}
#endif

void Navigator::dispose()
{
    if (m_geolocation) {
        m_geolocation->dispose();
    }
}

ScriptBindingInstance* Navigator::scriptBindingInstance()
{
    return document()->scriptBindingInstance();
}

String* Navigator::userAgent()
{
    return webView()->userAgent();
}

String* Navigator::platform()
{
    StringBuilder platformName;
#if !defined(OS_WINDOWS)
    // Unix-like systems
    struct utsname osname;
    if (uname(&osname) == 0) {
        platformName.appendString(osname.sysname);
        platformName.appendString(String::spaceString);
        platformName.appendString(osname.machine);
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

String* Navigator::language()
{
    return String::fromUTF8(webView()->locale().getName());
}
} // namespace Starfish
