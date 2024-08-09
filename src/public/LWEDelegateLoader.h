/*
 * Copyright (c) 2024-present Samsung Electronics Co., Ltd
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

#ifndef __LWEDelegateLoader__
#define __LWEDelegateLoader__

#ifdef STARFISH_API_ENABLE_LOADER

#include "public/delegate/LWEDelegate.h"
#include "public/delegate/ResourceErrorDelegate.h"
#include "public/delegate/SettingsDelegate.h"
#include "public/delegate/CookieManagerDelegate.h"
#include "public/delegate/LWEWebContainerDelegate.h"
#include "public/delegate/LWEWebViewDelegate.h"

namespace LWE {

class LWEDelegateLoader {
public:
    static LWEDelegateLoader* getInstance();
    static CookieManagerProcTable kCookieManagerProcTable;
    static LWEProcTable kLWEProcTable;
    static ResourceErrorProcTable kResourceErrorProcTable;
    static SettingsProcTable kSettingsProcTable;
    static WebContainerProcTable kWebContainerProcTable;
    static WebViewProcTable kWebViewProcTable;

    LWEDelegateLoader(const LWEDelegateLoader& other) = delete;
    LWEDelegateLoader(LWEDelegateLoader&& other) = delete;
    LWEDelegateLoader& operator=(const LWEDelegateLoader& other) = delete;

    void setVersionPreference(bool preferUpdatedVersion)
    {
        m_preferUpdatedVersion = preferUpdatedVersion;
    }

    bool load();
    void unload();

private:
    LWEDelegateLoader()
    {
    }

    ~LWEDelegateLoader()
    {
    }

    bool loadCookieManagerProcTable();
    bool loadLWEProcTable();
    bool loadResourceErrorProcTable();
    bool loadSettingsProcTable();
    bool loadWebContainerProcTable();
    bool loadWebViewProcTable();

    void unloadCookieManagerProcTable();
    void unloadLWEProcTable();
    void unloadResourceErrorProcTable();
    void unloadSettingsProcTable();
    void unloadWebContainerProcTable();
    void unloadWebViewProcTable();

    void* m_handle = nullptr;
    bool m_preferUpdatedVersion = false;
};

} // namespace LWE

#endif
#endif
