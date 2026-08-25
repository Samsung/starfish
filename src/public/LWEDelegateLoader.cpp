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

#ifdef STARFISH_API_ENABLE_LOADER

#include "LWELoaderUtils.h"
#include "LWEDelegateLoader.h"

#include <dlfcn.h>
#include <assert.h>
#include <iostream>

namespace LWE {

CookieManagerProcTable LWEDelegateLoader::kCookieManagerProcTable;
LWEProcTable LWEDelegateLoader::kLWEProcTable;
ResourceErrorProcTable LWEDelegateLoader::kResourceErrorProcTable;
SettingsProcTable LWEDelegateLoader::kSettingsProcTable;
WebContainerProcTable LWEDelegateLoader::kWebContainerProcTable;
WebViewProcTable LWEDelegateLoader::kWebViewProcTable;

LWEDelegateLoader* LWEDelegateLoader::getInstance()
{
    static LWEDelegateLoader* instance = nullptr;
    if (!instance) {
        instance = new LWEDelegateLoader();
    }
    return instance;
}

LWEDelegateLoader* LWEDelegateLoader::getSafeInstance()
{
    LWEDelegateLoader* instance = LWEDelegateLoader::getInstance();
    if (!instance->isLoaded()) {
        abort();
    }
    return instance;
}

bool LWEDelegateLoader::load()
{
    if (LWELoaderUtils::shouldUseUpdatedLibrary(m_preferUpdatedVersion) &&
        tryLoadAndValidate(LWELibrarySource::Updated)) {
        return true;
    }

    return tryLoadAndValidate(LWELibrarySource::Default);
}

bool LWEDelegateLoader::tryLoadAndValidate(LWELibrarySource source)
{
    if (!LWELoaderUtils::openLWELibrary(m_handle, STARFISH_API_TARGET_NAME,
                                        source)) {
        return false;
    }

    if (validateAbiEpoch() && loadProcTables()) {
        return true;
    }

    if (source == LWELibrarySource::Updated) {
        std::cerr << "Updated LWE validation failed; falling back to default "
                     "LWE."
                  << std::endl;
    }
    discardFailedLibrary();
    return false;
}

bool LWEDelegateLoader::validateAbiEpoch()
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

bool LWEDelegateLoader::loadProcTables()
{
    return loadCookieManagerProcTable() && loadLWEProcTable() &&
           loadResourceErrorProcTable() && loadSettingsProcTable() &&
           loadWebContainerProcTable() && loadWebViewProcTable();
}

void LWEDelegateLoader::discardFailedLibrary()
{
    dlclose(m_handle);
    m_handle = nullptr;
    unloadCookieManagerProcTable();
    unloadLWEProcTable();
    unloadResourceErrorProcTable();
    unloadSettingsProcTable();
    unloadWebContainerProcTable();
    unloadWebViewProcTable();
}

void LWEDelegateLoader::unload()
{
    // FIXME we cannot close shared-library
    // dlclose does not end lwe main thread
    // but global variables are reseted next dlopen with clang-compiled binary
    return;
    /*
    if (m_handle) {
        dlclose(m_handle);
        m_handle = nullptr;
    }

    unloadCookieManagerProcTable();
    unloadLWEProcTable();
    unloadResourceErrorProcTable();
    unloadSettingsProcTable();
    unloadWebContainerProcTable();
    unloadWebViewProcTable();
    */
}

bool LWEDelegateLoader::isLoaded()
{
    return m_handle != nullptr;
}

// Every dlsym cast below names its ProcTable member's own type rather than
// respelling the signature: the typedef in the contract header is the one
// place that has to agree with the impl, and respelling it here silently
// diverged once already (CreateWithBuffer was dlsym'ing the wrong symbol,
// with a signature that didn't match it either).
bool LWEDelegateLoader::loadCookieManagerProcTable()
{
    kCookieManagerProcTable.GetInstance =
        reinterpret_cast<decltype(CookieManagerProcTable::GetInstance)>(
            dlsym(m_handle, "LWEDelegate_CookieManager_GetInstance"));
    kCookieManagerProcTable.Destroy =
        reinterpret_cast<decltype(CookieManagerProcTable::Destroy)>(
            dlsym(m_handle, "LWEDelegate_CookieManager_Destroy"));
    return kCookieManagerProcTable.GetInstance &&
           kCookieManagerProcTable.Destroy;
}

bool LWEDelegateLoader::loadLWEProcTable()
{
    kLWEProcTable.Initialize =
        reinterpret_cast<decltype(LWEProcTable::Initialize)>(
            dlsym(m_handle, "LWEDelegate_LWE_Initialize"));
    kLWEProcTable.IsInitialized =
        reinterpret_cast<decltype(LWEProcTable::IsInitialized)>(
            dlsym(m_handle, "LWEDelegate_LWE_IsInitialized"));
    kLWEProcTable.Finalize = reinterpret_cast<decltype(LWEProcTable::Finalize)>(
        dlsym(m_handle, "LWEDelegate_LWE_Finalize"));
    kLWEProcTable.GetGCFrequency =
        reinterpret_cast<decltype(LWEProcTable::GetGCFrequency)>(
            dlsym(m_handle, "LWEDelegate_LWE_GetGCFrequency"));
    kLWEProcTable.SetGCFrequency =
        reinterpret_cast<decltype(LWEProcTable::SetGCFrequency)>(
            dlsym(m_handle, "LWEDelegate_LWE_SetGCFrequency"));
    kLWEProcTable.GetVersion =
        reinterpret_cast<decltype(LWEProcTable::GetVersion)>(
            dlsym(m_handle, "LWEDelegate_LWE_GetVersion"));
    kLWEProcTable.IsUsingSeparateThread =
        reinterpret_cast<decltype(LWEProcTable::IsUsingSeparateThread)>(
            dlsym(m_handle, "LWEDelegate_LWE_IsUsingSeparateThread"));
    return kLWEProcTable.Initialize && kLWEProcTable.IsInitialized &&
           kLWEProcTable.Finalize && kLWEProcTable.GetGCFrequency &&
           kLWEProcTable.SetGCFrequency && kLWEProcTable.GetVersion &&
           kLWEProcTable.IsUsingSeparateThread;
}

bool LWEDelegateLoader::loadResourceErrorProcTable()
{
    kResourceErrorProcTable.Create =
        reinterpret_cast<decltype(ResourceErrorProcTable::Create)>(
            dlsym(m_handle, "LWEDelegate_ResourceError_Create"));
    return kResourceErrorProcTable.Create;
}

bool LWEDelegateLoader::loadSettingsProcTable()
{
    kSettingsProcTable.Create =
        reinterpret_cast<decltype(SettingsProcTable::Create)>(
            dlsym(m_handle, "LWEDelegate_Settings_Create"));
    kSettingsProcTable.CreateEmpty =
        reinterpret_cast<decltype(SettingsProcTable::CreateEmpty)>(
            dlsym(m_handle, "LWEDelegate_Settings_Create_Empty"));
    kSettingsProcTable.CreateFromOther =
        reinterpret_cast<decltype(SettingsProcTable::CreateFromOther)>(
            dlsym(m_handle, "LWEDelegate_Settings_Create_From_Other"));
    return kSettingsProcTable.Create && kSettingsProcTable.CreateEmpty &&
           kSettingsProcTable.CreateFromOther;
}

bool LWEDelegateLoader::loadWebContainerProcTable()
{
    kWebContainerProcTable.Create =
        reinterpret_cast<decltype(WebContainerProcTable::Create)>(
            dlsym(m_handle, "LWEDelegate_WebContainer_Create"));
    kWebContainerProcTable.CreateWithBuffer =
        reinterpret_cast<decltype(WebContainerProcTable::CreateWithBuffer)>(
            dlsym(m_handle, "LWEDelegate_WebContainer_CreateWithBuffer"));
    kWebContainerProcTable.CreateWithPlatformImage = reinterpret_cast<
        decltype(WebContainerProcTable::CreateWithPlatformImage)>(
        dlsym(m_handle, "LWEDelegate_WebContainer_Create_With_PlatformImage"));
    kWebContainerProcTable.CreateGL =
        reinterpret_cast<decltype(WebContainerProcTable::CreateGL)>(
            dlsym(m_handle, "LWEDelegate_WebContainer_CreateGL"));
    kWebContainerProcTable.CreateGLWithPlatformImage = reinterpret_cast<
        decltype(WebContainerProcTable::CreateGLWithPlatformImage)>(
        dlsym(m_handle, "LWEDelegate_WebContainer_CreateGLWithPlatformImage"));
    kWebContainerProcTable.CreateHeadless =
        reinterpret_cast<decltype(WebContainerProcTable::CreateHeadless)>(
            dlsym(m_handle, "LWEDelegate_WebContainer_CreateHeadless"));

    return kWebContainerProcTable.Create &&
           kWebContainerProcTable.CreateWithBuffer &&
           kWebContainerProcTable.CreateWithPlatformImage &&
           kWebContainerProcTable.CreateGL &&
           kWebContainerProcTable.CreateGLWithPlatformImage &&
           kWebContainerProcTable.CreateHeadless;
}
bool LWEDelegateLoader::loadWebViewProcTable()
{
    kWebViewProcTable.Create =
        reinterpret_cast<decltype(WebViewProcTable::Create)>(
            dlsym(m_handle, "LWEDelegate_WebView_Create"));
    return kWebViewProcTable.Create;
}

void LWEDelegateLoader::unloadCookieManagerProcTable()
{
    kCookieManagerProcTable = { nullptr, nullptr };
}

void LWEDelegateLoader::unloadLWEProcTable()
{
    kLWEProcTable = { nullptr, nullptr, nullptr, nullptr,
                      nullptr, nullptr, nullptr };
}

void LWEDelegateLoader::unloadResourceErrorProcTable()
{
    kResourceErrorProcTable = { nullptr };
}

void LWEDelegateLoader::unloadSettingsProcTable()
{
    kSettingsProcTable = { nullptr, nullptr, nullptr };
}

void LWEDelegateLoader::unloadWebContainerProcTable()
{
    kWebContainerProcTable = { nullptr, nullptr, nullptr,
                               nullptr, nullptr, nullptr };
}

void LWEDelegateLoader::unloadWebViewProcTable()
{
    kWebViewProcTable = { nullptr };
}

} // namespace LWE

#endif
