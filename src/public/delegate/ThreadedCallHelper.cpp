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

#include "StarfishConfig.h"

#include "ThreadedCallHelper.h"

#include "core/modules/message_loop/MessageLoop.h"
#include "core/page/WebView.h"
#include <EscargotPublic.h>
#include <condition_variable>
#include <mutex>

#define THREAD_MINIMUM_STACK_SIZE \
    4 * 1024 * 1024 // we need at least 4MB for stack

namespace LWEDelegate {

ThreadedCallHelper* ThreadedCallHelper::Instance()
{
    static ThreadedCallHelper instance;
    return &instance;
}

ThreadedCallHelper::ThreadedCallHelper()
    : m_isLWEThreadStarted(false)
    , m_isThreadMode(false)
    , m_isMessageLoopInitialized(false)
    , m_didShutdown(false)
    , m_mainThread()
{
}

void ThreadedCallHelper::Initialize(bool isThreadMode)
{
    STARFISH_RELEASE_ASSERT(!m_didShutdown);
    STARFISH_RELEASE_ASSERT(!m_isMessageLoopInitialized ||
                            m_isThreadMode == isThreadMode);
    m_isThreadMode = isThreadMode;
    if (isThreadMode) {
        if (!m_isLWEThreadStarted) {
            CreateLWEMainThread();
        }
    } else {
        STARFISH_RELEASE_ASSERT(!m_isMessageLoopInitialized ||
                                pthread_equal(pthread_self(), m_mainThread));
        m_mainThread = pthread_self();
        Starfish::MessageLoop::init();
    }
    m_isMessageLoopInitialized = true;
}

bool ThreadedCallHelper::ShutdownForUnload()
{
    if (!m_isMessageLoopInitialized) {
        return true;
    }
    if (m_isLWEThreadStarted) {
        if (pthread_equal(pthread_self(), m_mainThread)) {
            return false;
        }
        {
            std::lock_guard<std::mutex> lock(m_shutdownMutex);
            PostTaskToLWEMainThreadSync([] { Starfish::MessageLoop::stop(); });
        }
        STARFISH_RELEASE_ASSERT(pthread_join(m_mainThread, nullptr) == 0);
    } else {
        // Final collector destruction must run on the thread that finalized
        // Escargot, even when the embedding application owns that thread.
        if (!pthread_equal(pthread_self(), m_mainThread)) {
            return false;
        }
        Starfish::MessageLoop::shutdown();
        Escargot::Globals::destroyRuntimeForSharedLibraryUnload();
    }
    m_isLWEThreadStarted = false;
    m_isThreadMode = false;
    m_isMessageLoopInitialized = false;
    m_didShutdown = true;
    return true;
}

bool ThreadedCallHelper::isThreadMode() const
{
    return m_isThreadMode;
}

void ThreadedCallHelper::PostTaskToLWEMainThreadSync(
    const std::function<void()>& functor)
{
    Starfish::MessageLoop::runOnMainThreadSync(functor);
}

void ThreadedCallHelper::PostTaskToLWEMainThreadAsync(
    Starfish::MessageLoop* messageLoop, const std::function<void()>& functor)
{
    messageLoop->runOnMainThreadAsync(functor);
}

void ThreadedCallHelper::CreateLWEMainThread()
{
    struct InitData {
        std::mutex mutex;
        std::condition_variable condition;
        bool isReady{ false };
    } init;
    std::unique_lock<std::mutex> lock(init.mutex);

    pthread_attr_t attr;
    STARFISH_RELEASE_ASSERT(pthread_attr_init(&attr) == 0);
    STARFISH_RELEASE_ASSERT(
        pthread_attr_setstacksize(&attr, THREAD_MINIMUM_STACK_SIZE) == 0);

    int result = pthread_create(
        &m_mainThread, &attr,
        [](void* data) -> void* {
            STARFISH_LOG_INFO("Start LWE thread to run MessageLoop.");
            InitData* init = static_cast<InitData*>(data);
            Starfish::MessageLoop::init();
            {
                std::lock_guard<std::mutex> lock(init->mutex);
                init->isReady = true;
                init->condition.notify_one();
            }
            Starfish::MessageLoop::run();
            // A stop callback can run before its caller finishes dispatching
            // the source or releases the sync guard. Keep the loop alive
            // until the last synchronous call has returned completely.
            std::lock_guard<std::mutex> lock(Instance()->m_shutdownMutex);
            // Drain close callbacks on their owning thread before releasing
            // the collector. Ordinary Finalize keeps both for reuse.
            Starfish::MessageLoop::shutdown();
            Escargot::Globals::destroyRuntimeForSharedLibraryUnload();
            return nullptr;
        },
        &init);

    STARFISH_RELEASE_ASSERT(pthread_attr_destroy(&attr) == 0);
    STARFISH_RELEASE_ASSERT(result == 0);
    init.condition.wait(lock, [&init] { return init.isReady; });
    m_isLWEThreadStarted = true;
}

} // namespace LWEDelegate
