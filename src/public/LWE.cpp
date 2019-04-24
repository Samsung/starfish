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

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/modules/message_loop/MessageLoop.h"

#include "LWEWebView.h"

#define THREAD_MINIMUM_STACK_SIZE \
    4 * 1024 * 1024 // we need at least 4MB for stack

namespace LWE {

Starfish::Starfish* g_starfishInstance;

#if defined(PORT_NEEDS_THREADED_PUBLIC_API)

static bool g_isLWEThreadStarted;
static pthread_mutex_t g_mainThreadInitLocker;

void* LWEMainThread(void*)
{
    Starfish::MessageLoop::init();
    pthread_mutex_unlock(&g_mainThreadInitLocker);
    STARFISH_LOG_INFO("Worker thread started!\n");
    Starfish::MessageLoop::run();
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

        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setstacksize(&attr, THREAD_MINIMUM_STACK_SIZE);
        pthread_t tid;
        pthread_create(&tid, &attr, LWEMainThread, NULL);

        pthread_mutex_lock(&g_mainThreadInitLocker);
        pthread_mutex_unlock(&g_mainThreadInitLocker);
        pthread_mutex_destroy(&g_mainThreadInitLocker);
        g_isLWEThreadStarted = true;
    }

    Starfish::MessageLoop::runOnMainThreadSync([&]() -> size_t {
        g_starfishInstance = new (NoGC) Starfish::Starfish(
            localStorageDataFilePath, cookieStoreDataFilePath,
            httpCacheDataDirectorypath);
        return 0;
    });
}

void LWE::Finalize()
{
    STARFISH_RELEASE_ASSERT(IsInitialized());

    Starfish::MessageLoop::runOnMainThreadSync([]() -> size_t {
        STARFISH_RELEASE_ASSERT(g_starfishInstance->webViewInstanceCount() ==
                                0);

        g_starfishInstance->destroy();
        g_starfishInstance = nullptr;

        clearStack<ELABORATE_CLEAR_STACK_SIZE>();

        // do implicit calling GC funciton takes a lots time
        // we should remove this if possible
        Starfish::Starfish::doFullGCWithoutSeeingStack();
        Starfish::Starfish::doFullGCWithoutSeeingStack();
        Starfish::Starfish::doFullGCWithoutSeeingStack();

        return 0;
    });
}

#else
void LWE::Initialize(const char* localStorageDataFilePath,
                     const char* cookieStoreDataFilePath,
                     const char* httpCacheDataDirectorypath)
{
    STARFISH_RELEASE_ASSERT(!IsInitialized());
    g_starfishInstance = new (NoGC)
        Starfish::Starfish(localStorageDataFilePath, cookieStoreDataFilePath,
                           httpCacheDataDirectorypath);
}

void LWE::Finalize()
{
    STARFISH_RELEASE_ASSERT(IsInitialized());
    STARFISH_RELEASE_ASSERT(g_starfishInstance->webViewInstanceCount() == 0);

    g_starfishInstance->destroy();
    g_starfishInstance = nullptr;

    clearStack<ELABORATE_CLEAR_STACK_SIZE>();

    // do implicit calling GC funciton takes a lots time
    // we should remove this if possible
    Starfish::Starfish::doFullGCWithoutSeeingStack();
    Starfish::Starfish::doFullGCWithoutSeeingStack();
    Starfish::Starfish::doFullGCWithoutSeeingStack();
}
#endif

bool LWE::IsInitialized()
{
    return g_starfishInstance;
}
}
