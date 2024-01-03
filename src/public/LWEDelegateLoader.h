/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
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

#define STARFISH_API_ENABLE_LOADER // only for test

#include "public/delegate/LWEDelegate.h"
#include "public/delegate/ResourceErrorDelegate.h"
#include "public/delegate/SettingsDelegate.h"
#include "public/delegate/CookieManagerDelegate.h"
#include "public/delegate/LWEWebContainerDelegate.h"
#include "public/delegate/LWEWebViewDelegate.h"

#include <dlfcn.h>
#include <iostream>

namespace LWE {

class LWEDelegateLoader {
public:
    static LWEDelegateLoader* getInstance();
    static CookieManagerProcTable kCookieManagerProcTable;
    static LWEProcTable kLWEProcTable;

    LWEDelegateLoader(const LWEDelegateLoader& other) = delete;
    LWEDelegateLoader(LWEDelegateLoader&& other) = delete;
    LWEDelegateLoader& operator=(const LWEDelegateLoader& other) = delete;

    bool load(std::string path);
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

    void* m_handle = nullptr;
};

} // namespace LWE
#endif
