/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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

#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/modules/message_loop/MessageLoop.h"

#include "LWEWebView.h"

namespace LWE {

StarFish::StarFish* g_starFishInstance;

#if defined(PORT_NEEDS_THREADED_PUBLIC_API)

static bool g_isLWEThreadStarted;
static pthread_mutex_t g_mainThreadInitLocker;

void* LWEMainThread(void*)
{
    StarFish::MessageLoop::init();
    pthread_mutex_unlock(&g_mainThreadInitLocker);
    STARFISH_LOG_INFO("Worker thread started!");
    StarFish::MessageLoop::run();
    return nullptr;
}

void LWE::Initialize(const char* localStorageDataFilePath,
                     const char* cookieStoreDataFilePath,
                     const char* httpCacheDataDirectorypath)
{
    STARFISH_RELEASE_ASSERT(!IsInitialized());
    if (!g_isLWEThreadStarted) {
        pthread_mutex_init(&g_mainThreadInitLocker, NULL);
        pthread_mutex_lock(&g_mainThreadInitLocker);
        pthread_t tid;
        pthread_create(&tid, NULL, LWEMainThread, NULL);
        pthread_mutex_lock(&g_mainThreadInitLocker);
        pthread_mutex_unlock(&g_mainThreadInitLocker);
        pthread_mutex_destroy(&g_mainThreadInitLocker);
        g_isLWEThreadStarted = true;
    }

    StarFish::MessageLoop::runOnMainThreadSync([&]() -> size_t {
        g_starFishInstance = new (NoGC) StarFish::StarFish(
            localStorageDataFilePath, cookieStoreDataFilePath,
            httpCacheDataDirectorypath);
        return 0;
    });
}

void LWE::Finalize()
{
    STARFISH_RELEASE_ASSERT(IsInitialized());

    StarFish::MessageLoop::runOnMainThreadSync([]() -> size_t {
        STARFISH_RELEASE_ASSERT(g_starFishInstance->webViewInstanceCount() ==
                                0);

        g_starFishInstance->destroy();
        g_starFishInstance = nullptr;

        clearStack<ELABORATE_CLEAR_STACK_SIZE>();

        // do implicit calling GC funciton takes a lots time
        // we should remove this if possible
        StarFish::StarFish::doFullGCWithoutSeeingStack();
        StarFish::StarFish::doFullGCWithoutSeeingStack();
        StarFish::StarFish::doFullGCWithoutSeeingStack();

        return 0;
    });
}

#else
void LWE::Initialize(const char* localStorageDataFilePath,
                     const char* cookieStoreDataFilePath,
                     const char* httpCacheDataDirectorypath)
{
    STARFISH_RELEASE_ASSERT(!IsInitialized());
    g_starFishInstance = new (NoGC)
        StarFish::StarFish(localStorageDataFilePath, cookieStoreDataFilePath,
                           httpCacheDataDirectorypath);
}

void LWE::Finalize()
{
    STARFISH_RELEASE_ASSERT(IsInitialized());
    STARFISH_RELEASE_ASSERT(g_starFishInstance->webViewInstanceCount() == 0);

    g_starFishInstance->destroy();
    g_starFishInstance = nullptr;

    clearStack<ELABORATE_CLEAR_STACK_SIZE>();

    // do implicit calling GC funciton takes a lots time
    // we should remove this if possible
    StarFish::StarFish::doFullGCWithoutSeeingStack();
    StarFish::StarFish::doFullGCWithoutSeeingStack();
    StarFish::StarFish::doFullGCWithoutSeeingStack();
}
#endif

bool LWE::IsInitialized()
{
    return g_starFishInstance;
}
}
