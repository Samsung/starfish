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

#if defined(STARFISH_WEBWORKER_HOST) && defined(STARFISH_API_ENABLE_LOADER)

#ifndef __LWEWorkerDelegateLoader__
#define __LWEWorkerDelegateLoader__

#include <string>

#include "public/contract/LWEWorkerDelegate.h"

namespace LWE {

class LWEWorkerDelegateLoader {
public:
    static LWEWorkerDelegateLoader* getInstance();

    // If called without loading, an assertion will be raised.
    static LWEWorkerDelegateLoader* getSafeInstance();

    static LWEWorkerProcTable kLWEWorkerProcTable;

    LWEWorkerDelegateLoader(const LWEWorkerDelegateLoader& other) = delete;
    LWEWorkerDelegateLoader(LWEWorkerDelegateLoader&& other) = delete;
    LWEWorkerDelegateLoader& operator=(const LWEWorkerDelegateLoader& other) =
        delete;

    void setVersionPreference(bool preferUpdatedVersion)
    {
        m_preferUpdatedVersion = preferUpdatedVersion;
    }

    bool load();
    void unload();

private:
    LWEWorkerDelegateLoader() = default;
    ~LWEWorkerDelegateLoader() = default;

    bool loadLWEWorkerProcTable();

    void unloadLWEWorkerProcTable();

    void* m_handle = nullptr;
    bool m_preferUpdatedVersion = false;
};

} // namespace LWE

#endif
#endif
