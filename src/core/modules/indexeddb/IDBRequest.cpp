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
#include "core/modules/indexeddb/IDBRequest.h"

namespace Starfish {

IDBRequest::IDBRequest(ExecutionContext* executionContext)
    : EventTarget()
    , m_executionContext(executionContext)
    , m_result(scriptUndefined())
    , m_error(nullptr)
    , m_source(scriptUndefined())
    , m_transaction(nullptr)
    , m_readyState(IDBRequestReadyState::Pending)
{
}

String* IDBRequest::readyState() const
{
    if (m_readyState == IDBRequestReadyState::Pending) {
        return String::createASCIIString("pending");
    } else if (m_readyState == IDBRequestReadyState::Done) {
        return String::createASCIIString("done");
    }

    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    return String::emptyString;
}

} // namespace Starfish

#endif
