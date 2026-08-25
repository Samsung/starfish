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

    if (LWELoaderUtils::shouldUseUpdatedLibrary(m_preferUpdatedVersion) &&
        tryLoadAndValidate(targetName, LWELibrarySource::Updated)) {
        return true;
    }

    return tryLoadAndValidate(targetName, LWELibrarySource::Default);
}

bool LWEWorkerDelegateLoader::tryLoadAndValidate(const std::string& targetName,
                                                 LWELibrarySource source)
{
    if (!LWELoaderUtils::openLWELibrary(m_handle, targetName, source)) {
        return false;
    }

    if (validateAbiEpoch() && loadLWEWorkerProcTable()) {
        return true;
    }

    if (source == LWELibrarySource::Updated) {
        std::cerr << "Updated LWE worker validation failed; falling back to "
                     "default LWE."
                  << std::endl;
    }
    discardFailedLibrary();
    return false;
}

bool LWEWorkerDelegateLoader::validateAbiEpoch()
{
    auto getAbiEpoch =
        reinterpret_cast<decltype(DelegateContractProcTable::GetAbiEpoch)>(
            dlsym(m_handle, "LWEDelegate_GetAbiEpoch"));
    if (!getAbiEpoch) {
        std::cerr << "LWE delegate ABI epoch symbol is missing." << std::endl;
        return false;
    }

    uint32_t epoch = getAbiEpoch();
    if (epoch != LWEDelegate::kDelegateAbiEpoch) {
        std::cerr << "LWE delegate ABI epoch mismatch: expected "
                  << LWEDelegate::kDelegateAbiEpoch << ", got " << epoch << "."
                  << std::endl;
        return false;
    }
    return true;
}

void LWEWorkerDelegateLoader::discardFailedLibrary()
{
    dlclose(m_handle);
    m_handle = nullptr;
    unloadLWEWorkerProcTable();
}

void LWEWorkerDelegateLoader::unload()
{
    if (m_handle) {
        dlclose(m_handle);
        m_handle = nullptr;
    }

    unloadLWEWorkerProcTable();
}

// Casts name their ProcTable member's own type rather than respelling the
// signature -- see the same note in LWEDelegateLoader.cpp.
bool LWEWorkerDelegateLoader::loadLWEWorkerProcTable()
{
    kLWEWorkerProcTable.Initialize =
        reinterpret_cast<decltype(LWEWorkerProcTable::Initialize)>(
            dlsym(m_handle, "LWEWorkerDelegate_LWEWorker_Initialize"));
    kLWEWorkerProcTable.RegisterOnStatusChangedHandler = reinterpret_cast<
        decltype(LWEWorkerProcTable::RegisterOnStatusChangedHandler)>(
        dlsym(m_handle,
              "LWEWorkerDelegate_LWEWorker_RegisterOnStatusChangedHandler"));
    kLWEWorkerProcTable.Finalize =
        reinterpret_cast<decltype(LWEWorkerProcTable::Finalize)>(
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
