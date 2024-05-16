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

#include "LWEDelegateLoader.h"

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

bool LWEDelegateLoader::load(std::string path)
{
    m_handle = dlopen(path.c_str(), RTLD_LAZY);
    if (!m_handle) {
        std::cerr << "Failed to open library: " << dlerror() << std::endl;
        return false;
    }

    return loadCookieManagerProcTable() && loadLWEProcTable() &&
           loadResourceErrorProcTable() && loadSettingsProcTable() &&
           loadWebContainerProcTable() && loadWebViewProcTable();
}

void LWEDelegateLoader::unload()
{
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
}

bool LWEDelegateLoader::loadCookieManagerProcTable()
{
    kCookieManagerProcTable.GetInstance = reinterpret_cast<uintptr_t (*)()>(
        dlsym(m_handle, "LWEDelegate_CookieManager_GetInstance"));
    kCookieManagerProcTable.Destroy = reinterpret_cast<void (*)()>(
        dlsym(m_handle, "LWEDelegate_CookieManager_Destroy"));
    return kCookieManagerProcTable.GetInstance &&
           kCookieManagerProcTable.Destroy;
}

bool LWEDelegateLoader::loadLWEProcTable()
{
    kLWEProcTable.Initialize =
        reinterpret_cast<void (*)(const char*, const char*, const char*)>(
            dlsym(m_handle, "LWEDelegate_LWE_Initialize"));
    kLWEProcTable.IsInitialized = reinterpret_cast<bool (*)()>(
        dlsym(m_handle, "LWEDelegate_LWE_IsInitialized"));
    kLWEProcTable.Finalize = reinterpret_cast<void (*)()>(
        dlsym(m_handle, "LWEDelegate_LWE_Finalize"));
    kLWEProcTable.GetGCFrequency = reinterpret_cast<unsigned char (*)()>(
        dlsym(m_handle, "LWEDelegate_LWE_GetGCFrequency"));
    kLWEProcTable.SetGCFrequency =
        reinterpret_cast<void (*)(unsigned char freq)>(
            dlsym(m_handle, "LWEDelegate_LWE_SetGCFrequency"));
    kLWEProcTable.GetVersion = reinterpret_cast<void (*)(int*, int*, int*)>(
        dlsym(m_handle, "LWEDelegate_LWE_GetVersion"));
    return kLWEProcTable.Initialize && kLWEProcTable.IsInitialized &&
           kLWEProcTable.Finalize && kLWEProcTable.GetGCFrequency &&
           kLWEProcTable.SetGCFrequency && kLWEProcTable.GetVersion;
}

bool LWEDelegateLoader::loadResourceErrorProcTable()
{
    kResourceErrorProcTable.Create =
        reinterpret_cast<uintptr_t (*)(int, const char*, const char*)>(
            dlsym(m_handle, "LWEDelegate_ResourceError_Create"));
    return kResourceErrorProcTable.Create;
}

bool LWEDelegateLoader::loadSettingsProcTable()
{
    kSettingsProcTable.Create =
        reinterpret_cast<uintptr_t (*)(const char* defaultUA, const char* ua)>(
            dlsym(m_handle, "LWEDelegate_Settings_Create"));
    kSettingsProcTable.CreateEmpty = reinterpret_cast<uintptr_t (*)()>(
        dlsym(m_handle, "LWEDelegate_Settings_Create_Empty"));
    kSettingsProcTable.CreateFromOther = reinterpret_cast<uintptr_t (*)(void*)>(
        dlsym(m_handle, "LWEDelegate_Settings_Create_From_Other"));
    return kSettingsProcTable.Create && kSettingsProcTable.CreateEmpty &&
           kSettingsProcTable.CreateFromOther;
}

bool LWEDelegateLoader::loadWebContainerProcTable()
{
    kWebContainerProcTable.Create = reinterpret_cast<uintptr_t (*)(
        unsigned, unsigned, float, const char*, const char*, const char*)>(
        dlsym(m_handle, "LWEDelegate_WebContainer_Create"));
    kWebContainerProcTable.CreateWithBuffer = reinterpret_cast<uintptr_t (*)(
        void*, unsigned, unsigned, unsigned, float, const char*, const char*,
        const char*)>(dlsym(m_handle, "LWEDelegate_WebContainer_Create"));
    kWebContainerProcTable.CreateWithPlatformImage = reinterpret_cast<
        uintptr_t (*)(unsigned, unsigned, uintptr_t, uintptr_t, float,
                      const char*, const char*, const char*)>(
        dlsym(m_handle, "LWEDelegate_WebContainer_Create_With_PlatformImage"));
    kWebContainerProcTable.CreateGL =
        reinterpret_cast<uintptr_t (*)(uintptr_t, uintptr_t)>(
            dlsym(m_handle, "LWEDelegate_WebContainer_CreateGL"));
    kWebContainerProcTable.CreateGLWithPlatformImage = reinterpret_cast<
        uintptr_t (*)(unsigned, unsigned, uintptr_t, uintptr_t, uintptr_t,
                      uintptr_t, float, const char*, const char*, const char*)>(
        dlsym(m_handle, "LWEDelegate_WebContainer_CreateGLWithPlatformImage"));
    kWebContainerProcTable.CreateHeadless = reinterpret_cast<uintptr_t (*)(
        unsigned, unsigned, float, const char*, const char*, const char*)>(
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
    kWebViewProcTable.Create = reinterpret_cast<uintptr_t (*)(
        void*, unsigned, unsigned, unsigned, unsigned, float, const char*,
        const char*, const char*)>(
        dlsym(m_handle, "LWEDelegate_WebView_Create"));
    return kWebViewProcTable.Create;
}

void LWEDelegateLoader::unloadCookieManagerProcTable()
{
    kCookieManagerProcTable = { nullptr, nullptr };
}

void LWEDelegateLoader::unloadLWEProcTable()
{
    kLWEProcTable = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
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
