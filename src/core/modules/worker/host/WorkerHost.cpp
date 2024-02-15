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

#include "core/dom/ExecutionContext.h"
#include "core/dom/DOMException.h"
#include "core/page/WebBase.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/message_loop/RunLoop.h"
#include "core/modules/message_loop/Timer.h"
#include "core/modules/worker/host/DedicatedWorkerGlobalScope.h"
#include "core/modules/worker/host/WebWorker.h"
#include "core/modules/worker/host/WorkerObjectProxy.h"
#include "core/modules/worker/Worker.h"
#include "core/modules/worker/WorkerHostProxy.h"
#include "core/modules/worker/WorkerThread.h"

#include "core/modules/worker/host/WorkerHost.h"

namespace Starfish {

void WorkerHost::run(void* data)
{
    STARFISH_ASSERT(Escargot::Globals::supportsThreading());
    Escargot::Globals::initializeThread();

    RunLoop* runLoop = RunLoop::create();
    Worker* workerObject = static_cast<Worker*>(data);
    WorkerThread* workerThread = workerObject->workerThread();
    WorkerHostProxy* hostProxy = workerObject->workerHostProxy();

    WorkerHost host = WorkerHost(workerObject, runLoop);
    hostProxy->workerHostCreated(&host);

    WorkerObjectProxy* objectProxy = host.globalScope()->workerObjectProxy();

    if (host.loadMainScript()) {
        objectProxy->postTask(
            [](void* data) {
                auto* hostProxy = static_cast<WorkerHostProxy*>(data);
                hostProxy->onScriptLoadFinished();
            },
            hostProxy);
    } else {
        objectProxy->terminateWorker();
    }

    if (!workerThread->wasTerminated()) {
        workerThread->onWorkerRunLoopStarted(runLoop);
        runLoop->run();
    }

    host.dispose();

    delete runLoop;

    Escargot::Globals::finalizeThread();
}

WorkerHost::WorkerHost(Worker* workerObject, RunLoop* runLoop)
    : m_wasDisposed(false)
{
    m_webWorker =
        new WebWorker(workerObject->executionContext()->webBase(), runLoop);

    m_globalScope = m_webWorker->createGlobalScope<DedicatedWorkerGlobalScope>(
        new ResourceURL(workerObject->url()->urlString(),
                        workerObject->url()->baseURI()));

    m_globalScope->initialize(
        new WorkerObjectProxy(m_globalScope->executionContext(), workerObject,
                              workerObject->workerThread()));
}

bool WorkerHost::loadMainScript()
{
    try {
        m_globalScope->importScript(
            m_globalScope->executionContext()->documentURI());
    } catch (DOMException* e) {
        return false;
    }

    return true;
}

void WorkerHost::dispose()
{
    if (m_wasDisposed) {
        return;
    }
    m_wasDisposed = true;

    m_webWorker->timer()->clear(m_globalScope);
    m_webWorker->messageLoop()->clearPendingIdlers(m_globalScope);
    m_globalScope->dispose();
    m_webWorker->destroy();
}

} // namespace Starfish

#endif /* STARFISH_WEBWORKER_HOST */
