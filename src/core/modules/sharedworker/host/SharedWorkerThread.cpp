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

#if defined(STARFISH_ENABLE_SHARED_WORKER) && defined(STARFISH_WEBWORKER_HOST)

#include "StarfishConfig.h"
#include "core/modules/worker/host/WebWorker.h"
#include "core/modules/sharedworker/host/SharedWorkerGlobalScope.h"
#include "core/modules/sharedworker/host/SharedWorkerThread.h"

namespace Starfish {

SharedWorkerThread::SharedWorkerThread(
    Starfish* starfish, MessageLoop* messageLoop, const std::string& name,
    const WorkerHostInitData& workerHostInitData)
    : WorkerThread(starfish, messageLoop, workerHostInitData)
    , m_globalScope(nullptr)
    , m_name(name)
{
    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void* cd) {
            SharedWorkerThread* self = castTo<SharedWorkerThread*>(obj);
            self->~SharedWorkerThread();
        },
        NULL, NULL, NULL);
}

SharedWorkerThread::~SharedWorkerThread() = default;

WorkerGlobalScope* SharedWorkerThread::createWorkerGlobalScope(
    WebWorker* webWorker, WorkerHost* workerHost)
{
    STARFISH_ASSERT(!m_globalScope);

    m_globalScope = webWorker->createGlobalScope<SharedWorkerGlobalScope>(
        createScriptURL());

    m_globalScope->initialize(m_name);

    return m_globalScope;
}

} // namespace Starfish

#endif /* STARFISH_ENABLE_SHARED_WORKER */
