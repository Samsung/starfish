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

#if defined(STARFISH_ENABLE_IDB)

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/util/debug/Trace.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/DOMException.h"
#include "core/dom/Event.h"
#include "core/modules/indexeddb/IDBDatabase.h"
#include "core/modules/indexeddb/IDBOpenDBRequest.h"

namespace Starfish {

IDBOpenDBRequest::IDBOpenDBRequest(ExecutionContext* executionContext)
    : IDBRequest(executionContext)
{
}

DOMException* IDBOpenDBRequest::errorCodeToDOMException(
    ExecutionContext* executionContext, OpenDBRequestErrorType error)
{
    if (error == OpenDBRequestErrorType::None) {
        return new DOMException(executionContext, DOMException::DOM_EXCEPTION,
                                "Unknown Error");
    } else if (error == OpenDBRequestErrorType::VersionError) {
        return new DOMException(
            executionContext,
            String::createASCIIString(
                "The requested version is less than the existing version."),
            String::createASCIIString("VersionError"));
    }

    STARFISH_ASSERT_NOT_REACHED();
    return nullptr;
}

void IDBOpenDBRequest::successOpenRequest()
{
    STARFISH_ASSERT(m_executionContext->isContextThread());
    STARFISH_ASSERT(m_database);
    TRACE(IDB);

    m_result = m_database->scriptValue();
    m_done = true;
    dispatchSuccessEvent();
}

void IDBOpenDBRequest::failOpenRequest(OpenDBRequestErrorType error)
{
    STARFISH_ASSERT(m_executionContext->isContextThread());
    TRACE(IDB);

    m_error = errorCodeToDOMException(m_executionContext, error);
    m_result = scriptUndefined();
    m_done = true;
    dispatchErrorEvent();
}

void IDBOpenDBRequest::upgradeNeeded()
{
    STARFISH_ASSERT(m_database);
    TRACE(IDB);

    m_database->startVersionChangeTransaction();

    m_result = m_database->scriptValue();

    Event* event = new Event(m_executionContext,
                             staticStrings()->m_upgradeneeded.localName(),
                             EventInit(true, true));

    EventTarget::dispatchEventIdleTimeByUA(event);
}

DEFINE_EVENT_LISTENER(IDBOpenDBRequest, upgradeneeded);

} // namespace Starfish

#endif
