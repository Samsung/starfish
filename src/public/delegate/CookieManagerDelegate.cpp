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

#include "CookieManagerDelegate.h"
#include "LWEDelegate.h"
#include "platform/network/curl/NetworkSharedResourceManager.h"
#include "core/modules/message_loop/MessageLoop.h"

#ifdef PORT_NEEDS_THREADED_PUBLIC_API
#define START_SIMPLE_THREADED_PUBLIC_API_WRAPPER Starfish::MessageLoop::runOnMainThreadSync([&]() -> size_t {
#define END_SIMPLE_THREADED_PUBLIC_API_WRAPPER \
    return 0;                                  \
    });
#else
#define START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
#define END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
#endif

#ifdef PORT_NEEDS_THREADED_PUBLIC_API
#define START_ASYNC_THREADED_PUBLIC_API_WRAPPER TO_WEBVIEW(m_impl)->messageLoop()->runOnMainThreadAsync([=]() -> void {
#define END_ASYNC_THREADED_PUBLIC_API_WRAPPER \
    });
#else
#define START_ASYNC_THREADED_PUBLIC_API_WRAPPER
#define END_ASYNC_THREADED_PUBLIC_API_WRAPPER
#endif

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
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    result = Starfish::NetworkSharedResourceManager::getInstance()
                 ->cookies(new Starfish::ResourceURL(url.c_str(), url.size()))
                 ->toUTF8NonGCString();
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER

    return result;
}

bool CookieManagerImpl::HasCookies()
{
    bool hasCookies;
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    hasCookies =
        Starfish::NetworkSharedResourceManager::getInstance()->hasCookies();
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER

    return hasCookies;
}
void CookieManagerImpl::ClearCookies()
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    Starfish::NetworkSharedResourceManager::getInstance()->clearCookies();
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
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
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    if (!g_cookieManager) {
        g_cookieManager = new CookieManagerImpl();
    }
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    return g_cookieManager;
}

void CookieManager::Destroy()
{
    if (!LWEDelegate::LWE::IsInitialized()) {
        return;
    }
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    if (g_cookieManager) {
        delete g_cookieManager;
        g_cookieManager = nullptr;
    }
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
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
