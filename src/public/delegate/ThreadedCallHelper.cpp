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

#define THREAD_MINIMUM_STACK_SIZE \
    4 * 1024 * 1024 // we need at least 4MB for stack

namespace LWEDelegate {

class SimpleCaller : public Caller {
public:
    virtual size_t SyncCall(const std::function<size_t()>& functor) override
    {
        return functor();
    }

    virtual void AsyncCall(Starfish::MessageLoop*,
                           const std::function<void()>& functor) override
    {
        functor();
    }
};

class ThreadedCaller : public Caller {
public:
    virtual size_t SyncCall(const std::function<size_t()>& functor) override
    {
        return Starfish::MessageLoop::runOnMainThreadSync(functor);
    }

    virtual void AsyncCall(Starfish::MessageLoop* messageLoop,
                           const std::function<void()>& functor) override
    {
        messageLoop->runOnMainThreadAsync(functor);
    }
};

ThreadedCallHelper* ThreadedCallHelper::m_instance = nullptr;

ThreadedCallHelper* ThreadedCallHelper::Instance()
{
    if (m_instance == nullptr) {
        m_instance = new ThreadedCallHelper();
    }
    return m_instance;
}

ThreadedCallHelper::ThreadedCallHelper()
    : m_isThreadMode(false)
    , m_isLWEThreadStarted(false)
{
}

void ThreadedCallHelper::Initialize(const std::string& backend)
{
    if (backend == "glfw_cairo_gl" || backend == "x11_cairo_gl" ||
        backend == "dali" || backend == "flutter") {
        m_isThreadMode = true;
    }

    if (m_isThreadMode) {
        m_caller = std::make_unique<ThreadedCaller>();
        if (!m_isLWEThreadStarted) {
            CreateLWEMainThread();
        }
    } else {
        m_caller = std::make_unique<SimpleCaller>();
    }
}

size_t ThreadedCallHelper::PostTaskToLWEMainThreadSync(
    const std::function<size_t()>& functor)
{
    return m_caller->SyncCall(functor);
}

void ThreadedCallHelper::PostTaskToLWEMainThreadAsync(
    Starfish::MessageLoop* messageLoop, const std::function<void()>& functor)
{
    return m_caller->AsyncCall(messageLoop, functor);
}

void ThreadedCallHelper::CreateLWEMainThread()
{
    pthread_mutex_init(&m_mainThreadInitLocker, nullptr);
    pthread_mutex_lock(&m_mainThreadInitLocker);

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, THREAD_MINIMUM_STACK_SIZE);
    pthread_t tid;

    pthread_create(
        &tid, &attr,
        [](void* data) -> void* {
            ThreadedCallHelper* self = static_cast<ThreadedCallHelper*>(data);
            Starfish::MessageLoop::init();
            pthread_mutex_unlock(&self->m_mainThreadInitLocker);
            STARFISH_LOG_INFO("Worker thread started!");
            Starfish::MessageLoop::run();
            return nullptr;
        },
        this);

    pthread_mutex_lock(&m_mainThreadInitLocker);
    pthread_mutex_unlock(&m_mainThreadInitLocker);
    pthread_mutex_destroy(&m_mainThreadInitLocker);
    m_isLWEThreadStarted = true;
}

} // namespace LWEDelegate
