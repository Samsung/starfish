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
#include "core/dom/ExecutionContext.h"
#include "core/modules/worker/DedicatedWorkerThread.h"
#include "core/modules/worker/WorkerHostProxy.h"
#include "core/modules/worker/Worker.h"

namespace Starfish {

Worker::Worker(ExecutionContext* executionContext, String* scriptURL,
               const WorkerOptions& workerOptions)
    : AbstractWorker(executionContext, scriptURL, workerOptions)
    , m_workerThread(
          new DedicatedWorkerThread(executionContext->webBase(), this))
    , m_workerHostProxy(new WorkerHostProxy(executionContext, m_workerThread))
    , m_wasTerminated(false)
{
    m_workerThread->start();
}

void Worker::postMessage(ScriptValue message,
                         GCAtomicVector<ScriptObject>& transfer)
{
    if (m_wasTerminated) {
        return;
    }

    m_workerHostProxy->postMessage(message, transfer);
}

void Worker::postMessage(ScriptValue message,
                         const StructuredSerializeOptions& options)
{
    if (m_wasTerminated) {
        return;
    }

    m_workerHostProxy->postMessage(message, options.transfer());
}

void Worker::terminate()
{
    if (m_wasTerminated) {
        return;
    }
    m_wasTerminated = true;

    m_workerHostProxy->close();
    m_workerHostProxy->terminateWorkerGlobalScope();
    m_workerHostProxy->terminate();

    m_workerThread->setOnTerminatedCallback(
        [](void* data) {
            Worker* self = static_cast<Worker*>(data);
            self->destroy();
        },
        this);
    m_workerThread->terminate();
}

void Worker::destroy()
{
    m_workerHostProxy = nullptr;
    m_workerThread = nullptr;
}

ScriptBindingInstance* Worker::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

DEFINE_EVENT_LISTENER(Worker, message);
DEFINE_EVENT_LISTENER(Worker, messageerror);

} // namespace Starfish
#endif
