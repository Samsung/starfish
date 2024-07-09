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
#include "core/modules/worker/util/network/Connection.h"
#include "core/modules/sharedworker/IPCSerializer.h"
#include "core/modules/sharedworker/IPCMessageHandler.h"

namespace Starfish {

Nullable<IPCMessageSerializer*> IPCMessageHandler::serialize(
    IPCMessage& message)
{
    IPCMessageSerializer* serializer = message.serialize();
    if (serializer->isError()) {
        TRACE(IPC, "IPCMessageSerializer error");
        return Nullable<IPCMessageSerializer*>();
    }

    return serializer;
}

void IPCMessageHandler::sendMessage(Connection* connection, IPCMessage& message)
{
    Nullable<IPCMessageSerializer*> serializer = serialize(message);
    if (serializer.hasValue()) {
        serializer->writeTerminator();
        connection->send(serializer->data(), serializer->size());
    }
}

void IPCMessageHandler::onReceiveMessage(const char* data, size_t length)
{
    IPCMessageDeserializer deserializer(data, length);
    if (deserializer.isError() || deserializer.messageID().empty()) {
        TRACE(IPC, "IPCMessageDeserializer error");
        return;
    }

    auto it = m_receiveHandlers.find(deserializer.messageID());
    if (it != m_receiveHandlers.end()) {
        it->second(&deserializer);
    }
}

void IPCMessageHandler::setMessageReceiveHandler(const std::string& id,
                                                 MessageReceiveHandler handler)
{
    m_receiveHandlers.insert({ id, handler });
}

} // namespace Starfish

#endif
