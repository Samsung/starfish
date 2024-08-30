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

#include <dlfcn.h>
#include <iostream>
#include <cassert>

#include "LWEWorker.h"
#include "LWELoaderUtils.h"
#include "LWEWorkerDelegateLoader.h"

namespace LWE {

LWEWorkerProcTable LWEWorkerDelegateLoader::kLWEWorkerProcTable;

LWEWorkerDelegateLoader* LWEWorkerDelegateLoader::getInstance()
{
    static LWEWorkerDelegateLoader* instance = nullptr;
    if (!instance) {
        instance = new LWEWorkerDelegateLoader();
    }
    return instance;
}

LWEWorkerDelegateLoader* LWEWorkerDelegateLoader::getSafeInstance()
{
    LWEWorkerDelegateLoader* instance = LWEWorkerDelegateLoader::getInstance();
    if (!instance->m_handle) {
        assert(false);
    }
    return instance;
}

bool LWEWorkerDelegateLoader::load()
{
#if defined(STARFISH_ENABLE_SHARED_WORKER)
    std::string targetName = STARFISH_SHARED_WORKER_API_TARGET_NAME;
#elif defined(STARFISH_ENABLE_SERVICE_WORKER)
    std::string targetName = STARFISH_SERVICE_WORKER_API_TARGET_NAME;
#else
#error \
    "Please define STARFISH_ENABLE_SHARED_WORKER or STARFISH_ENABLE_SERVICE_WORKER."
#endif

    if (!LWELoaderUtils::openLWELibrary(m_handle, targetName,
                                        m_preferUpdatedVersion)) {
        return false;
    }

    return loadLWEWorkerProcTable();
}

void LWEWorkerDelegateLoader::unload()
{
    if (m_handle) {
        dlclose(m_handle);
        m_handle = nullptr;
    }

    unloadLWEWorkerProcTable();
}

bool LWEWorkerDelegateLoader::loadLWEWorkerProcTable()
{
    kLWEWorkerProcTable.Initialize =
        reinterpret_cast<void (*)(const std::string&)>(
            dlsym(m_handle, "LWEWorkerDelegate_LWEWorker_Initialize"));
    kLWEWorkerProcTable.RegisterOnStatusChangedHandler = reinterpret_cast<
        void (*)(const std::function<void(::LWE::WorkerProcessState)>&)>(
        dlsym(m_handle,
              "LWEWorkerDelegate_LWEWorker_RegisterOnStatusChangedHandler"));
    kLWEWorkerProcTable.Finalize = reinterpret_cast<void (*)()>(
        dlsym(m_handle, "LWEWorkerDelegate_LWEWorker_Finalize"));

    return kLWEWorkerProcTable.Initialize &&
           kLWEWorkerProcTable.RegisterOnStatusChangedHandler &&
           kLWEWorkerProcTable.Finalize;
}

void LWEWorkerDelegateLoader::unloadLWEWorkerProcTable()
{
    kLWEWorkerProcTable = { nullptr, nullptr, nullptr };
}

} // namespace LWE

#endif
