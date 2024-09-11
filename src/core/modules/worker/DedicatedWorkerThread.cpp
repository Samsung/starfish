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

#if defined(STARFISH_ENABLE_WORKER)

#include "StarfishConfig.h"
#include "Starfish.h"

#include "core/modules/worker/WebWorker.h"
#include "core/modules/worker/DedicatedWorkerGlobalScope.h"
#include "core/modules/worker/WorkerObjectProxy.h"
#include "core/modules/worker/Worker.h"
#include "core/modules/worker/DedicatedWorkerThread.h"

namespace Starfish {

DedicatedWorkerThread::DedicatedWorkerThread(WebBase* webBase, Worker* worker)
    : WorkerThread(webBase, worker->scriptURL())
    , m_worker(worker)
{
    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void* cd) {
            DedicatedWorkerThread* self = castTo<DedicatedWorkerThread*>(obj);
            self->~DedicatedWorkerThread();
        },
        NULL, NULL, NULL);
}

DedicatedWorkerThread::~DedicatedWorkerThread() = default;

WorkerGlobalScope* DedicatedWorkerThread::createWorkerGlobalScope(
    WebWorker* webWorker, WorkerHost* workerHost)
{
    DedicatedWorkerGlobalScope* globalScope =
        webWorker->createGlobalScope<DedicatedWorkerGlobalScope>(
            createScriptURL());

    globalScope->initialize(
        workerHost,
        new WorkerObjectProxy(globalScope->executionContext(), m_worker));

    return globalScope;
}

} // namespace Starfish

#endif /* STARFISH_ENABLE_WORKER */
