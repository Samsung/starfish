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
#include "core/util/debug/Trace.h"
#include "core/page/GlobalScope.h"
#include "core/page/WebBase.h"
#include "core/page/Serializer.h"
#include "core/dom/MessagePort.h"
#include "core/dom/MessageEvent.h"
#include "core/dom/ExecutionContext.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/worker/util/network/SocketNN.h"
#include "core/modules/sharedworker/IPCSerializer.h"
#include "core/modules/sharedworker/SharedWorkerMessagePortConnection.h"

namespace Starfish {

SharedWorkerMessagePortConnection::SharedWorkerMessagePortConnection(
    PerProcess* perProcess, MessagePort* messagePort, uint32_t clientID,
    uint32_t pid, const std::string& ipcAddress)
    : IPCConnection(perProcess, ipcAddress, SocketNN::kPairProtocol)
    , m_messagePort(messagePort)
    , m_clientID(clientID)
    , m_pid(pid)
{
}

SharedWorkerMessagePortConnection::~SharedWorkerMessagePortConnection() =
    default;

void SharedWorkerMessagePortConnection::onReceived(Socket* socket,
                                                   const char* data,
                                                   size_t size)
{
    TRACE(SHAREDWORKER, size);
    STARFISH_ASSERT(m_messagePort->executionContext()->isContextThread());

    if (!isRunning()) {
        return;
    }

    SerializeWithTransferResult* serialized = new SerializeWithTransferResult();
    serialized->m_serialized = new SerializedTypedData(
        SerializedTypedData::Type::RawScriptValue,
        new SerializedRawScriptValueData(new IPCSerializedData(data, size)));
    serialized->m_deserializer = IPCSerializer::deserializeWithTransfer;

    MessageEvent* event =
        new MessageEvent(m_messagePort->executionContext(), serialized);
    m_messagePort->dispatchEventByUA(event);
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
