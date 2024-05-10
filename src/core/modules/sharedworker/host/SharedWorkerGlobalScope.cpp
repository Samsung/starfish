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
#include "binding/ScriptBindingWorkerInstance.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/MessagePort.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/worker/util/Trace.h"
#include "core/modules/worker/host/WebWorker.h"
#include "core/modules/worker/WorkerIPCAddress.h"
#include "core/modules/sharedworker/SharedWorkerMessagePortConnection.h"
#include "core/modules/sharedworker/host/SharedWorkerAgent.h"
#include "core/modules/sharedworker/host/SharedWorkerGlobalScope.h"

namespace Starfish {

SharedWorkerGlobalScope::SharedWorkerGlobalScope(WebWorker* webWorker,
                                                 ResourceURL* url,
                                                 String* charSet)
    : WorkerGlobalScope(webWorker)
    , m_name(String::emptyString)
{
    m_scriptBindingInstance =
        new ScriptBindingWorkerInstance<SharedWorkerGlobalScope>(
            webWorker->scriptEngineInstance(), this);

    initGlobalScope(url, charSet);
}

void SharedWorkerGlobalScope::initialize(const std::string& name)
{
    STARFISH_ASSERT(m_executionContext->isContextThread());

    m_name = String::fromUTF8(name.data(), name.size());

    if (loadMainScript()) {
    } else {
        // TODO: terminate worker
    }
}

void SharedWorkerGlobalScope::dispose()
{
    STARFISH_ASSERT(m_executionContext->isContextThread());

    for (const auto& connection : m_connections) {
        connection.second->close();
    }

    WorkerGlobalScope::dispose();
}

void SharedWorkerGlobalScope::postTask(PostTaskCallback task, void* data)
{
    if (m_closing) {
        return;
    }

    MessageLoop* messageLoop = m_webWorker->messageLoop();

    struct Param {
        SharedWorkerGlobalScope* globalScope;
        PostTaskCallback task;
        void* data;
    };

    Param* p = new Param();
    p->globalScope = this;
    p->task = task;
    p->data = data;

    auto callback = [](size_t, void* data) {
        Param* p = static_cast<Param*>(data);
        if (p->globalScope->isClosing()) {
            delete p;
            return;
        }

        p->task(p->globalScope, p->data);
        delete p;
    };

    if (messageLoop->calledOnValidThread()) {
        messageLoop->addIdler(this, callback, p);
    } else {
        messageLoop->addIdlerWithNoGCRootingInOtherThread(this, callback, p);
    }
}

SharedWorkerMessagePortConnection*
SharedWorkerGlobalScope::createMessagePortConnection(
    MessagePortConnectionInfo* info, MessagePort* messagePort)
{
    auto* agent = SharedWorkerAgent::instance();
    auto* connection = new SharedWorkerMessagePortConnection(
        agent->perProcess(), messagePort, info->identifier, info->clientID,
        agent->ipcAddress()->createIPCAddress(
            std::to_string(info->identifier)));

    m_connections.insert({ info->identifier, connection });

    return connection;
}

void SharedWorkerGlobalScope::requestConnection(MessagePortConnectionInfo* info)
{
    TRACE(SHAREDWORKER, info->identifier);
    STARFISH_ASSERT(m_executionContext->isContextThread());

    MessagePort* messagePort = new MessagePort(m_executionContext);

    // TODO: entangle target MessagePort and emit onConnectMessage

    SharedWorkerMessagePortConnection* connection =
        createMessagePortConnection(info, messagePort);
    connection->bind();

    SharedWorkerAgent::instance()->didGlobalScopeConnected(this, connection);
}

ScriptBindingInstance* SharedWorkerGlobalScope::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}

} // namespace Starfish

#endif /* STARFISH_ENABLE_WORKER */
