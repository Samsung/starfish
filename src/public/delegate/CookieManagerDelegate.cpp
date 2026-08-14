/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#include "StarfishConfig.h"

#include "public/contract/CookieManagerDelegate.h"

#include "public/contract/LWEDelegate.h"
#include "ThreadedCallHelper.h"
#include "platform/network/curl/NetworkSharedResourceManager.h"

namespace LWEDelegate {

class CookieManagerImpl : public CookieManager {
public:
    virtual std::string GetCookie(std::string url) override;
    virtual bool HasCookies() override;
    virtual void ClearCookies() override;

    CookieManagerImpl() = default;
    ~CookieManagerImpl() = default;
};

static CookieManagerImpl* g_cookieManager;

std::string CookieManagerImpl::GetCookie(std::string url)
{
    std::string result;
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync([&]() -> void {
        result =
            Starfish::NetworkSharedResourceManager::getInstance()
                ->cookies(new Starfish::ResourceURL(url.c_str(), url.size()))
                ->toUTF8NonGCString();
    });
    return result;
}

bool CookieManagerImpl::HasCookies()
{
    bool hasCookies;
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync([&]() -> void {
        hasCookies =
            Starfish::NetworkSharedResourceManager::getInstance()->hasCookies();
    });
    return hasCookies;
}
void CookieManagerImpl::ClearCookies()
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync([&]() -> void {
        Starfish::NetworkSharedResourceManager::getInstance()->clearCookies();
    });
}

CookieManager* CookieManager::GetInstance()
{
    if (!LWEDelegate::LWE::IsInitialized()) {
        STARFISH_LOG_ERROR(
            "You must call LWE::Initialize function before using "
            "CookieManager");
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        return nullptr;
    }

    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync([&]() -> void {
        if (!g_cookieManager) {
            g_cookieManager = new CookieManagerImpl();
        }
    });
    return g_cookieManager;
}

void CookieManager::Destroy()
{
    if (!LWEDelegate::LWE::IsInitialized()) {
        return;
    }

    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync([&]() -> void {
        if (g_cookieManager) {
            delete g_cookieManager;
            g_cookieManager = nullptr;
        }
    });
}
} // namespace LWEDelegate

extern "C" {

uintptr_t LWEDelegate_CookieManager_GetInstance()
{
    return reinterpret_cast<uintptr_t>(
        LWEDelegate::CookieManager::GetInstance());
}

void LWEDelegate_CookieManager_Destroy()
{
    LWEDelegate::CookieManager::Destroy();
}
}
