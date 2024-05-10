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

#if defined(STARFISH_ENABLE_SHARED_WORKER)

#include "StarfishConfig.h"
#include "core/page/GlobalScope.h"
#include "core/page/WebBase.h"
#include "core/dom/MessagePort.h"
#include "core/dom/ExecutionContext.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/worker/util/Trace.h"
#include "core/modules/worker/util/network/SocketNN.h"
#include "core/modules/sharedworker/SharedWorkerMessagePortConnection.h"

namespace Starfish {

SharedWorkerMessagePortConnection::SharedWorkerMessagePortConnection(
    PerProcess* perProcess, MessagePort* messagePort, uint32_t identifier,
    uint32_t clientID, const std::string& ipcAddress)
    : IPCConnection(perProcess, ipcAddress, SocketNN::kPairProtocol)
    , m_messagePort(messagePort)
    , m_identifier(identifier)
    , m_clientID(clientID)
{
    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void* cd) {
            SharedWorkerMessagePortConnection* self =
                castTo<SharedWorkerMessagePortConnection*>(obj);
            self->~SharedWorkerMessagePortConnection();
        },
        NULL, NULL, NULL);
}

SharedWorkerMessagePortConnection::~SharedWorkerMessagePortConnection() =
    default;

void SharedWorkerMessagePortConnection::onReceived(Socket* socket,
                                                   const char* data,
                                                   size_t size)
{
    TRACE(SHAREDWORKER, size);
    STARFISH_ASSERT(m_messagePort->executionContext()->isContextThread());

    // TODO: dispatch message event
}

IMessageLoop* SharedWorkerMessagePortConnection::messageLoop()
{
    return m_messagePort->executionContext()
        ->globalScope()
        ->webBase()
        ->messageLoop();
}

} // namespace Starfish

#endif // #ifdef STARFISH_WEBWORKER_HOST
