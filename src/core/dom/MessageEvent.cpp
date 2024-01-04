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

#include "StarfishConfig.h"
#include "Starfish.h"
#include "binding/ScriptWrappable.h"
#include "core/dom/DOMException.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/MessagePort.h"
#include "core/page/Serializer.h"
#include "core/page/WebBase.h"

#include "core/dom/MessageEvent.h"

namespace Starfish {

MessageEvent::MessageEvent(ExecutionContext* executionContext,
                           SerializeWithTransferResult* serializedRecord)
    : Event(executionContext)
    , m_data(scriptNull())
    , m_origin(String::emptyString)
    , m_lastEventId(String::emptyString)
    , m_source(nullptr)
{
    StaticStrings* staticStrings =
        executionContext->webBase()->starfish()->staticStrings();

    DeserializeWithTransferResult deserializedRecord;
    try {
        Serializer::deserializeWithTransfer(executionContext, *serializedRecord,
                                            deserializedRecord);
    } catch (DOMException* exc) {
        setType(staticStrings->m_messageerror.localName());
        return;
    }

    setType(staticStrings->m_message.localName());
    setData(deserializedRecord.m_deserialized);

    GCVector<MessagePort*> newPorts;
    for (size_t i = 0; i < deserializedRecord.m_deserializedTransfer.size();
         i++) {
        ScriptValue item = deserializedRecord.m_deserializedTransfer[i];
        STARFISH_ASSERT(isObjectScriptValue(item));
        ScriptWrappable* sw = toScriptWrappable(item);
        if (sw && sw->isMessagePort()) {
            newPorts.push_back(sw->asMessagePort());
        }
    }
    setPorts(newPorts);
}

} // namespace Starfish
