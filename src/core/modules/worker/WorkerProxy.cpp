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
#include "core/page/WebBase.h"
#include "core/dom/ExecutionContext.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/worker/WorkerThread.h"
#include "core/modules/worker/WorkerProxy.h"

namespace Starfish {

WorkerProxy::WorkerProxy(ExecutionContext* executionContext,
                         WorkerThread* workerThread)
    : m_ownerExecutionContext(executionContext)
    , m_workerThread(workerThread)
    , m_wasTerminate(false)
{
}

void WorkerProxy::postTask(PostTask task, void* data)
{
    if (m_workerThread->wasWorkerTerminated()) {
        return;
    }

    MessageLoop* messageLoop = targetMessageLoop();

    STARFISH_ASSERT(!messageLoop->calledOnValidThread());
    messageLoop->addIdlerWithNoGCRootingInOtherThread(
        m_workerThread->workerMessageLoopGlobalScope(),
        [](size_t handle, void* data, void* data1) {
            reinterpret_cast<PostTask>(data)(data1);
        },
        reinterpret_cast<void*>(task), data);
}

void WorkerProxy::terminate()
{
    STARFISH_ASSERT(m_ownerExecutionContext->isContextThread());

    if (m_wasTerminate) {
        return;
    }

    m_wasTerminate = true;

    clearPendingPostTask();
}

void WorkerProxy::clearPendingPostTask()
{
    m_ownerExecutionContext->webBase()->messageLoop()->clearPendingIdlers(
        m_workerThread->workerMessageLoopGlobalScope());
}

} // namespace Starfish
#endif
