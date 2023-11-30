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

#include "core/page/WebBase.h"
#include "core/dom/ExecutionContext.h"
#include "core/modules/threading/Locker.h"
#include "core/modules/threading/Mutex.h"
#include "core/modules/threading/Thread.h"
#include "core/modules/worker/util/Trace.h"
#include "core/modules/worker/WorkerThread.h"

namespace Starfish {

WorkerThread::WorkerThread(ExecutionContext* executionContext)
    : m_executionContext(executionContext)
    , m_mainThread(new Thread(nullptr))
{
}

void* WorkerThread::workerMainThreadWork(void* data,
                                         std::future<void>&& stopTask)
{
    auto* self = static_cast<WorkerThread*>(data);

    TRACE(WORKER, "start worker thread", getCurrentThreadID());

    std::promise<void> workerHostThreadSignal;
    auto workerHostThreadFuture = workerHostThreadSignal.get_future();
    self->m_workerThread = std::thread(
        [](std::promise<void> signal, void* data) {
            // TODO: run worker script
            signal.set_value();
        },
        std::move(workerHostThreadSignal), self);
    stopTask.wait();

    if (workerHostThreadFuture.wait_for(std::chrono::seconds(1)) ==
        std::future_status::timeout) {
        self->destroyWorkerThread();
    }

    if (self->m_workerThread.joinable()) {
        self->m_workerThread.join();
    }

    TRACE(WORKER, "finish worker thread", getCurrentThreadID());
    return nullptr;
}

void WorkerThread::start()
{
    STARFISH_ASSERT(m_executionContext->isContextThread());

    m_mainThread->run(m_executionContext->webBase()->messageLoop(),
                      workerMainThreadWork, this);
}

void WorkerThread::destroyWorkerThread()
{
#if defined(OS_POSIX) && !defined(STARFISH_ANDROID)
    pthread_cancel(m_workerThread.native_handle());
#else
    STARFISH_UNIMPLEMENTED();
#endif
}

} // namespace Starfish

#endif
