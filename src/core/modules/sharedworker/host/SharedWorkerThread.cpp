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
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/worker/util/Trace.h"
#include "core/modules/worker/host/WebWorker.h"
#include "core/modules/sharedworker/host/SharedWorkerAgent.h"
#include "core/modules/sharedworker/host/SharedWorkerGlobalScope.h"
#include "core/modules/sharedworker/host/SharedWorkerThread.h"

namespace Starfish {

SharedWorkerThread::SharedWorkerThread(
    Starfish* starfish, MessageLoop* messageLoop, const std::string& name,
    const WorkerHostInitData& workerHostInitData)
    : WorkerThread(starfish, messageLoop, workerHostInitData)
    , m_globalScope(nullptr)
    , m_name(name)
    , m_initialIdentifier(0)
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
    STARFISH_ASSERT(!m_messageLoop->calledOnValidThread());

    SharedWorkerGlobalScope* globalScope =
        webWorker->createGlobalScope<SharedWorkerGlobalScope>(
            createScriptURL());

    globalScope->initialize(m_name);

    Nullable<MessagePortConnectionInfo*> connectionInfo =
        SharedWorkerAgent::instance()->getConnectionInfo(m_initialIdentifier);
    STARFISH_ASSERT(connectionInfo.hasValue());

    globalScope->requestConnection(connectionInfo.getValue());

    connectionInfo->thread->messageLoop()->addIdlerWithNoGCRootingInOtherThread(
        nullptr,
        [](size_t handle, void* data, void* data1) {
            auto* self = static_cast<SharedWorkerThread*>(data);
            auto* globalScope = static_cast<SharedWorkerGlobalScope*>(data1);

            self->createdWorkerGlobalScope(globalScope);
        },
        this, globalScope);

    return globalScope;
}

void SharedWorkerThread::startWithIdentifier(uint32_t identifier)
{
    TRACE(SHAREDWORKER, identifier);
    STARFISH_ASSERT(m_messageLoop->calledOnValidThread());

    m_initialIdentifier = identifier;

    start();
}

static void requestConnectionToGlobalScope(SharedWorkerGlobalScope* globalScope,
                                           void* data)
{
    globalScope->requestConnection(
        static_cast<MessagePortConnectionInfo*>(data));
}

void SharedWorkerThread::requestSharedWorkerConnection(
    MessagePortConnectionInfo* info)
{
    TRACE(SHAREDWORKER, info->identifier);
    STARFISH_ASSERT(m_messageLoop->calledOnValidThread());

    if (!m_globalScope) {
        m_pendingConnectionInfos.push_back(info);
        return;
    }

    m_globalScope->postTask(requestConnectionToGlobalScope, info);
}

void SharedWorkerThread::createdWorkerGlobalScope(
    SharedWorkerGlobalScope* globalScope)
{
    STARFISH_ASSERT(m_messageLoop->calledOnValidThread());

    STARFISH_ASSERT(!m_globalScope);
    m_globalScope = globalScope;

    for (const auto& info : m_pendingConnectionInfos) {
        m_globalScope->postTask(requestConnectionToGlobalScope, info);
    }

    m_pendingConnectionInfos.clear();
    m_pendingConnectionInfos.shrink_to_fit();
}

} // namespace Starfish

#endif /* STARFISH_ENABLE_SHARED_WORKER */
