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
#include "core/fileapi/Blob.h"
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
    , m_state(State::None)
    , m_workerMessageLoopGlobalScope(new WorkerProxyGlobalScope())
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

    // A blob: script URL can only be resolved against this (the page)
    // thread's blob store; capture the source now so the worker can
    // evaluate it without going through the resource loader.
    if (scriptURL->isBlobURL()) {
        BlobURLStore store;
        if (WebBase::stringToBlobURLString(scriptURL->urlString(), store) &&
            webBase->isValidBlobURL(store)) {
            Blob* blob = static_cast<Blob*>(store.m_blob);
            if (blob->size()) {
                m_workerHostInitData.inlineMainScript.assign(
                    reinterpret_cast<const char*>(blob->data()), blob->size());
            }
            m_workerHostInitData.hasInlineMainScript = true;
        }
    }
}

WorkerThread::~WorkerThread() = default;

void* WorkerThread::workerMainThreadWork(void* data,
                                         std::future<void>&& stopTask)
{
    WorkerThread* self = static_cast<WorkerThread*>(data);

    TRACE(WORKER, "start worker thread", getCurrentThreadID());

    self->m_workerThread = std::thread(WorkerHost::run, data);
    stopTask.wait();

    // Running JavaScript must return before its thread and GC can be retired.
    // Native cancellation can bypass C++ and engine cleanup; even a slow or
    // non-yielding script must keep its runtime and containing SO alive.

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

    if (m_workerMessageLoop) {
        auto* loop = m_workerMessageLoop.value();
        // Every worker kind stops on its owning loop after running JS returns.
        // A queued request also survives the loop not having started yet.
        loop->addIdlerWithNoGCRootingInOtherThread(
            nullptr,
            [](size_t, void* data) {
                static_cast<MessageLoop*>(data)->runLoop()->stop();
            },
            loop);
    }
    m_mainThread->stop();
}

void WorkerThread::setWorkerMessageLoop(Optional<MessageLoop*> messageLoop)
{
    Locker<Mutex> locker(*m_mutex);
    m_workerMessageLoop = messageLoop;
}

void WorkerThread::join()
{
    m_mainThread->joinIfNeeds();
}

void WorkerThread::setOnTerminatedCallback(
    WorkerThreadClient::ThreadFinishedCallback callback, void* data)
{
    m_mainThreadClient->setThreadFinishedCallback(callback, data);
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
