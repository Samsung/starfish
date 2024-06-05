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
    size_t sharedWorkerKey, const WorkerHostInitData& workerHostInitData)
    : WorkerThread(starfish, messageLoop, workerHostInitData)
    , m_globalScope(nullptr)
    , m_name(name)
    , m_sharedWorkerKey(sharedWorkerKey)
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

    globalScope->initialize(m_name, m_sharedWorkerKey);

    Nullable<MessagePortConnectionInfo*> connectionInfo =
        SharedWorkerAgent::instance()->getConnectionInfo(m_initialIdentifier);
    if (!connectionInfo.hasValue()) {
        TRACE(SHAREDWORKER, "GlobalScope will be closed");
        return globalScope;
    }

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
        for (const auto& pid : m_pendingClosePids) {
            if (info->pid == pid) {
                continue;
            }
        }

        m_globalScope->postTask(requestConnectionToGlobalScope, info);
    }

    m_pendingConnectionInfos.clear();
    m_pendingConnectionInfos.shrink_to_fit();

    for (const auto& pid : m_pendingClosePids) {
        requestCloseToGlobalScope(pid);
    }

    m_pendingClosePids.clear();
    m_pendingClosePids.shrink_to_fit();
}

void SharedWorkerThread::requestCloseToGlobalScope(uint32_t pid)
{
    struct Param {
        size_t pid;
    };
    Param* p = new Param();
    p->pid = pid;

    m_globalScope->postTask(
        [](SharedWorkerGlobalScope* globalScope, void* data) {
            auto* p = static_cast<Param*>(data);
            globalScope->closeConnection(p->pid);

            delete p;
        },
        p);
}

void SharedWorkerThread::closeSharedWorkerConnection(uint32_t pid)
{
    TRACE(SHAREDWORKER, pid);
    STARFISH_ASSERT(m_messageLoop->calledOnValidThread());

    if (!m_globalScope) {
        m_pendingClosePids.push_back(pid);
        return;
    }

    requestCloseToGlobalScope(pid);
}

} // namespace Starfish

#endif /* STARFISH_ENABLE_SHARED_WORKER */
