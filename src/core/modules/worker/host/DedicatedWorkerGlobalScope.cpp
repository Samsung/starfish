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

#ifdef STARFISH_ENABLE_WORKER

#include "StarfishConfig.h"
#include "Starfish.h"
#include "binding/ScriptBindingWorkerInstance.h"

#include "core/modules/message_loop/MessageLoop.h"
#include "core/dom/ExecutionContext.h"
#include "core/modules/worker/Worker.h"
#include "core/modules/worker/WorkerHostProxy.h"
#include "core/modules/worker/host/WebWorker.h"
#include "core/modules/worker/host/WorkerObjectProxy.h"
#include "core/modules/worker/host/DedicatedWorkerGlobalScope.h"

namespace Starfish {

DedicatedWorkerGlobalScope::DedicatedWorkerGlobalScope(WebWorker* webWorker,
                                                       ResourceURL* url,
                                                       String* charSet)
    : WorkerGlobalScope(webWorker)
    , m_workerObjectProxy(nullptr)
    , m_name(String::emptyString)
    , m_wasTerminated(false)
{
    m_scriptBindingInstance =
        new ScriptBindingWorkerInstance<DedicatedWorkerGlobalScope>(
            webWorker->scriptEngineInstance(), this);
    initGlobalScope(url, charSet);
}

void DedicatedWorkerGlobalScope::initialize(
    WorkerHost* workerHost, WorkerObjectProxy* workerObjectProxy)
{
    STARFISH_ASSERT(!m_workerObjectProxy);
    m_workerObjectProxy = workerObjectProxy;
    m_name = m_workerObjectProxy->workerName();

    WorkerHostProxy* hostProxy =
        m_workerObjectProxy->workerObject()->workerHostProxy();
    hostProxy->workerHostCreated(workerHost);
    hostProxy->entangleTarget(this, workerObjectProxy);

    if (loadMainScript()) {
        m_workerObjectProxy->postTask(
            [](void* data) {
                auto* hostProxy = static_cast<WorkerHostProxy*>(data);
                hostProxy->onScriptLoadFinished();
            },
            hostProxy);
    } else {
        m_workerObjectProxy->terminateWorker();
    }
}

void DedicatedWorkerGlobalScope::postMessage(
    ScriptValue message, const GCAtomicVector<ScriptObject>& transfer)
{
    if (m_wasTerminated) {
        return;
    }

    STARFISH_ASSERT(m_workerObjectProxy);
    m_workerObjectProxy->postMessage(message, transfer);
}

void DedicatedWorkerGlobalScope::postMessage(
    ScriptValue message, const StructuredSerializeOptions& options)
{
    if (m_wasTerminated) {
        return;
    }

    STARFISH_ASSERT(m_workerObjectProxy);
    m_workerObjectProxy->postMessage(message, options.transfer());
}

void DedicatedWorkerGlobalScope::close()
{
    if (!terminate()) {
        return;
    }

    // The closing worker is postponed to the next to process all currently
    // loaded scripts.
    m_webWorker->messageLoop()->addIdler(
        this,
        [](size_t handle, void* data) {
            auto* self = static_cast<DedicatedWorkerGlobalScope*>(data);
            self->workerObjectProxy()->terminateWorker();
        },
        this);
}

void DedicatedWorkerGlobalScope::dispose()
{
    if (m_wasTerminated) {
        return;
    }

    m_wasTerminated = true;

    if (m_workerObjectProxy) {
        m_workerObjectProxy->terminate();
        m_workerObjectProxy = nullptr;
    }

    WorkerGlobalScope::dispose();
}

ScriptBindingInstance* DedicatedWorkerGlobalScope::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}

DEFINE_EVENT_LISTENER(DedicatedWorkerGlobalScope, message);
DEFINE_EVENT_LISTENER(DedicatedWorkerGlobalScope, messageerror);

} // namespace Starfish

#endif /* STARFISH_ENABLE_WORKER */
