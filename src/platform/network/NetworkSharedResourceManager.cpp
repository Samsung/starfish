/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "StarFishConfig.h"
#include "NetworkSharedResourceManager.h"
#include "core/modules/threading/Locker.h"
#include "core/modules/threading/Mutex.h"
namespace StarFish {

static NetworkSharedResourceManager* instance = nullptr;

static void curlLockCallback(CURL* handle, curl_lock_data data,
                             curl_lock_access access, void* userPtr)
{
    NetworkSharedResourceManager* manager =
        static_cast<NetworkSharedResourceManager*>(userPtr);
    manager->resourceMutex(data)->lock();
}

static void curlUnlockCallback(CURL* handle, curl_lock_data data, void* userPtr)
{
    NetworkSharedResourceManager* manager =
        static_cast<NetworkSharedResourceManager*>(userPtr);
    manager->resourceMutex(data)->unlock();
}

NetworkSharedResourceManager* NetworkSharedResourceManager::getInstance()
{
    if (!instance) {
        instance = new NetworkSharedResourceManager();
    }
    return instance;
}

void NetworkSharedResourceManager::close()
{
    STARFISH_ASSERT(instance);
    delete instance;
}

NetworkSharedResourceManager::NetworkSharedResourceManager()
    : m_curlShareHandle(nullptr)
    , m_cookieJarFileName("/tmp/StarFish_Cookies.txt") // Temporary name
    , m_cookieMutex(new (NoGC) Mutex())
    , m_dnsMutex(new (NoGC) Mutex())
    , m_shareMutex(new (NoGC) Mutex())
    , m_storeCookieFile(false)
{
    curl_global_init(CURL_GLOBAL_ALL);
    m_curlShareHandle = curl_share_init();
    curl_share_setopt(m_curlShareHandle, CURLSHOPT_SHARE,
                      CURL_LOCK_DATA_COOKIE);
    curl_share_setopt(m_curlShareHandle, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS);
    curl_share_setopt(m_curlShareHandle, CURLSHOPT_LOCKFUNC, curlLockCallback);
    curl_share_setopt(m_curlShareHandle, CURLSHOPT_UNLOCKFUNC,
                      curlUnlockCallback);
    curl_share_setopt(m_curlShareHandle, CURLSHOPT_USERDATA, this);
}

NetworkSharedResourceManager::~NetworkSharedResourceManager()
{
    curl_share_cleanup(m_curlShareHandle);
    curl_global_cleanup();

    delete m_cookieMutex;
    delete m_dnsMutex;
    delete m_shareMutex;
}

CURLSH* NetworkSharedResourceManager::curlShareHandle() const
{
    return m_curlShareHandle;
}

std::string NetworkSharedResourceManager::cookieJarFileName() const
{
    return m_cookieJarFileName;
}

void NetworkSharedResourceManager::setCookieJarFileName(const std::string& name)
{
    m_cookieJarFileName = name;
}

void NetworkSharedResourceManager::initCookieSession()
{
    CURL* curl = curl_easy_init();

    if (!curl) {
        STARFISH_ASSERT_NOT_REACHED();
        return;
    }

    curl_easy_setopt(curl, CURLOPT_SHARE, m_curlShareHandle);
    if (m_storeCookieFile) {
        curl_easy_setopt(curl, CURLOPT_COOKIEFILE, m_cookieJarFileName.data());
        curl_easy_setopt(curl, CURLOPT_COOKIEJAR, m_cookieJarFileName.data());
    }
    curl_easy_setopt(curl, CURLOPT_COOKIESESSION, 1);
    curl_easy_cleanup(curl);
}

Mutex* NetworkSharedResourceManager::resourceMutex(curl_lock_data data)
{
    switch (data) {
    case CURL_LOCK_DATA_COOKIE:
        return m_cookieMutex;
    case CURL_LOCK_DATA_DNS:
        return m_dnsMutex;
    case CURL_LOCK_DATA_SHARE:
        return m_shareMutex;
    default:
        STARFISH_ASSERT_NOT_REACHED();
        return nullptr;
    }
}

void NetworkSharedResourceManager::enableToStoreCookiesJarAsFile()
{
    m_storeCookieFile = true;
}

void NetworkSharedResourceManager::disableToStoreCookiesJarAsFile()
{
    m_storeCookieFile = false;
}
}
