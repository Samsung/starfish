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
#ifndef __CookieManagerDelegate__
#define __CookieManagerDelegate__

#include "LWEDelegateConfig.h"

#include <string>

namespace LWEDelegate {

class EXPORT_UNMANAGED_API CookieManager {
public:
    static CookieManager* GetInstance();
    static void Destroy();

    virtual std::string GetCookie(std::string url) = 0;
    virtual bool HasCookies() = 0;
    virtual void ClearCookies() = 0;

protected:
    CookieManager() = default;
    virtual ~CookieManager() = default;

private:
    CookieManager(const CookieManager& other) = delete;
    CookieManager(CookieManager&& other) = delete;
    CookieManager& operator=(const CookieManager& other) = delete;
};

} // namespace LWEDelegate

// C wrappers used for dlopen/dlsym.
extern "C" {
uintptr_t EXPORT_UNMANAGED_API LWEDelegate_CookieManager_GetInstance();
void EXPORT_UNMANAGED_API LWEDelegate_CookieManager_Destroy();

typedef struct {
    uintptr_t (*GetInstance)();
    void (*Destroy)();
} CookieManagerProcTable;
}

#endif
