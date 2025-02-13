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

#if defined(STARFISH_ENABLE_WORKER)

#include "StarfishConfig.h"
#include "Starfish.h"

#include "platform/loader/ResourceURL.h"
#include "core/page/WebBase.h"
#include "core/page/GlobalScope.h"
#include "core/dom/ExecutionContext.h"
#include "core/modules/message_loop/RunLoop.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/threading/Locker.h"
#include "core/modules/threading/Mutex.h"
#include "core/modules/threading/Thread.h"
#include "core/modules/worker/Worker.h"
#include "core/modules/worker/WorkerHost.h"
#include "core/util/debug/Trace.h"

#include "core/modules/worker/WorkerThread.h"

namespace Starfish {

// This class is used as a key to clear pending idlers in the message loop
// when a worker terminates.
class WorkerProxyGlobalScope : public GlobalScope, public gc {
public:
    WorkerProxyGlobalScope()
        : GlobalScope(nullptr)
    {
    }

    ExecutionContext* executionContext() const
    {
        STARFISH_ASSERT_NOT_REACHED();
        return nullptr;
    }
};

void WorkerThreadClient::onThreadStarted(Thread* thread)
{
}

void WorkerThreadClient::onThreadFinished(Thread* thread)
{
    if (m_threadFinishedCallback) {
        m_threadFinishedCallback(m_threadFinishedCallbackData);
    }
}

void WorkerThreadClient::setThreadFinishedCallback(
    ThreadFinishedCallback callback, void* data)
{
    m_threadFinishedCallback = callback;
    m_threadFinishedCallbackData = data;
}

WorkerThread::WorkerThread(Starfish* starfish, MessageLoop* messageLoop)
    : m_starfish(starfish)
    , m_messageLoop(messageLoop)
    , m_mutex(new Mutex())
    , m_runLoop(nullptr)
    , m_state(State::None)
    , m_workerMessageLoopGlobalScope(new WorkerProxyGlobalScope())
    , m_childThreadDataLock(new Mutex())
{
    m_mainThreadClient = new WorkerThreadClient();
    m_mainThread = new Thread(m_mainThreadClient);
}

WorkerThread::WorkerThread(Starfish* starfish, MessageLoop* messageLoop,
                           const WorkerHostInitData& initData)
    : WorkerThread(starfish, messageLoop)
{
    m_workerHostInitData = initData;
}

WorkerThread::WorkerThread(WebBase* webBase, ResourceURL* scriptURL)
    : WorkerThread(webBase->starfish(), webBase->messageLoop())
{
    m_workerHostInitData.url = scriptURL->urlString()->toUTF8NonGCString();
    m_workerHostInitData.baseURL = scriptURL->baseURI()->toUTF8NonGCString();
    m_workerHostInitData.locale = webBase->locale();
    m_workerHostInitData.timezoneID =
        webBase->timezoneID()->toUTF8NonGCString();
    m_workerHostInitData.userAgent = webBase->userAgent()->toUTF8NonGCString();
}

WorkerThread::~WorkerThread() = default;

void* WorkerThread::workerMainThreadWork(void* data,
                                         std::future<void>&& stopTask)
{
    WorkerThread* self = static_cast<WorkerThread*>(data);

    TRACE(WORKER, "start worker thread", getCurrentThreadID());

    std::promise<void> workerHostThreadSignal;
    auto workerHostThreadFuture = workerHostThreadSignal.get_future();
    self->m_workerThread = std::thread(
        [](std::promise<void> signal, void* data) {
            auto* workerThread = static_cast<WorkerThread*>(data);
            workerThread->initializeWorkerThread();

            WorkerHost::run(data);

            signal.set_value();
        },
        std::move(workerHostThreadSignal), data);
    stopTask.wait();

    if (workerHostThreadFuture.wait_for(std::chrono::seconds(1)) ==
        std::future_status::timeout) {
        self->destroyWorkerThread();
    }

    self->terminateChildThreads();

    if (self->m_workerThread.joinable()) {
        self->m_workerThread.join();
    }

    TRACE(WORKER, "finish worker thread", getCurrentThreadID());
    return nullptr;
}

void WorkerThread::start()
{
    STARFISH_ASSERT(m_messageLoop->calledOnValidThread());

    m_state = State::Running;

    Locker<Mutex> locker(*m_mutex);

    m_mainThread->run(m_messageLoop, workerMainThreadWork, this);
}

void WorkerThread::terminate()
{
    TRACE(WORKER);

    if (m_state != State::Running) {
        return;
    }

    m_state = State::Terminated;

    Locker<Mutex> locker(*m_mutex);

    stopWorkerRunLoop();
    m_mainThread->stop();
}

void WorkerThread::setOnTerminatedCallback(
    WorkerThreadClient::ThreadFinishedCallback callback, void* data)
{
    m_mainThreadClient->setThreadFinishedCallback(callback, data);
}

void WorkerThread::initializeWorkerThread()
{
#if defined(OS_POSIX) && !defined(STARFISH_ANDROID)
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
#else
    STARFISH_UNSUPPORTED("Unsupported platform");
#endif
}

void WorkerThread::destroyWorkerThread()
{
#if defined(OS_POSIX) && !defined(STARFISH_ANDROID)
    pthread_cancel(m_workerThread.native_handle());
#else
    STARFISH_UNSUPPORTED("Unsupported platform");
#endif
}

void WorkerThread::onWorkerRunLoopStarted(RunLoop* runLoop)
{
    Locker<Mutex> locker(*m_mutex);

    STARFISH_ASSERT(!m_runLoop);
    m_runLoop = runLoop;
}

bool WorkerThread::stopWorkerRunLoop()
{
    if (!m_runLoop) {
        return false;
    }

    m_runLoop->stop();
    m_runLoop = nullptr;

    return true;
}

void WorkerThread::addChildThread(WorkerThread* thread)
{
    Locker<Mutex> lock(*m_childThreadDataLock);

    m_childThreads.push_back(thread);
}

void WorkerThread::removeChildThread(WorkerThread* thread)
{
    Locker<Mutex> lock(*m_childThreadDataLock);

    auto iter = std::find(m_childThreads.begin(), m_childThreads.end(), thread);
    if (iter != m_childThreads.end()) {
        m_childThreads.erase(iter);
    }
}

void WorkerThread::terminateChildThreads()
{
    Locker<Mutex> lock(*m_childThreadDataLock);

    if (m_childThreads.empty()) {
        return;
    }

    for (WorkerThread* thread : m_childThreads) {
        thread->terminate();
    }

    m_childThreads.clear();
    m_childThreads.shrink_to_fit();
}

bool WorkerThread::isRunning()
{
    return m_state == State::Running;
}

bool WorkerThread::wasTerminated()
{
    return m_state == State::Terminated;
}

ResourceURL* WorkerThread::createScriptURL()
{
    return new ResourceURL(
        String::fromUTF8(m_workerHostInitData.url.data(),
                         m_workerHostInitData.url.length()),
        String::fromUTF8(m_workerHostInitData.baseURL.data(),
                         m_workerHostInitData.baseURL.length()));
}

} // namespace Starfish

#endif
