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
#include "core/serialize/MemorySerializer.h"
#include "core/modules/sharedworker/IPCConnection.h"
#include "core/modules/sharedworker/IPCMessagePort.h"

namespace Starfish {

IPCMessagePort::IPCMessagePort(ExecutionContext* executionContext,
                               IPCConnection* connection)
    : MessagePort(executionContext)
    , m_connection(connection)
{
    m_serializer = MemorySerializer::serializeWithTransfer;
}

IPCMessagePort::IPCMessagePort(ExecutionContext* executionContext)
    : IPCMessagePort(executionContext, nullptr)
{
}

void IPCMessagePort::registerDispatchMessageTask(
    SerializeWithTransferResult* serializedMessage)
{
    TRACE(SHAREDWORKER);
    if (!m_connection || !m_connection->isRunning()) {
        TRACE(SHAREDWORKER, "cannot connect message port");
        return;
    }

    STARFISH_ASSERT(serializedMessage->m_serialized);

    SerializedRawScriptValueData* serializedData =
        serializedMessage->m_serialized->data()
            ->asSerializedRawScriptValueData();

    m_connection->send(serializedData->internal()->data(),
                       serializedData->internal()->size());
}

} // namespace Starfish
#endif // #ifdef STARFISH_ENABLE_SHARED_WORKER
