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

#include "LWEDelegateLoader.h"

namespace LWE {

CookieManagerProcTable LWEDelegateLoader::kCookieManagerProcTable;
LWEProcTable LWEDelegateLoader::kLWEProcTable;
ResourceErrorProcTable LWEDelegateLoader::kResourceErrorProcTable;

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
           loadResourceErrorProcTable();
}

void LWEDelegateLoader::unload()
{
    if (m_handle) {
        dlclose(m_handle);
        m_handle = nullptr;
    }
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
    return kLWEProcTable.Initialize && kLWEProcTable.IsInitialized &&
           kLWEProcTable.Finalize && kLWEProcTable.GetGCFrequency &&
           kLWEProcTable.SetGCFrequency;
}

bool LWEDelegateLoader::loadResourceErrorProcTable()
{
    kResourceErrorProcTable.Create =
        reinterpret_cast<uintptr_t (*)(int, const char*, const char*)>(
            dlsym(m_handle, "LWEDelegate_ResourceError_Create"));
    return kResourceErrorProcTable.Create;
}

} // namespace LWE
