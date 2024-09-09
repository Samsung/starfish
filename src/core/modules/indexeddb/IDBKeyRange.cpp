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
#include "core/dom/ExecutionContext.h"
#include "core/dom/DOMException.h"
#include "core/modules/indexeddb/IDBKey.h"

#include "core/modules/indexeddb/IDBKeyRange.h"

namespace Starfish {

IDBKeyRange* IDBKeyRange::convertValueToKeyRange(
    ExecutionContext* executionContext, ScriptValue value, bool nullDisallowed)
{
    // https://w3c.github.io/IndexedDB/#convert-a-value-to-a-key-range

    // TODO: 1. If value is a key range, return value.

    if (isNullOrUndefinedScriptValue(value)) {
        if (nullDisallowed) {
            throw new DOMException(
                executionContext,
                String::createASCIIString("The key is undefined or null."),
                String::createASCIIString("DataError"));
        } else {
            IDBKey* key = new IDBKey(IDBKey::Type::Null);
            new IDBKeyRange(executionContext, key, key, false, false);
        }
    }

    IDBKey* key = IDBKey::convertValueToKey(
        executionContext->scriptBindingInstance(), value);
    if (key->isInvalid()) {
        throw new DOMException(executionContext,
                               String::createASCIIString("The key is invalid."),
                               String::createASCIIString("DataError"));
    }

    return new IDBKeyRange(executionContext, key, key, false, false);
}

IDBKeyRange::IDBKeyRange(ExecutionContext* executionContext, IDBKey* lower,
                         IDBKey* upper, bool isLowerOpen, bool isUpperOpen)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
    , m_lower(lower)
    , m_upper(upper)
    , m_isLowerOpen(isLowerOpen)
    , m_isUpperOpen(isUpperOpen)
    , m_isOnly(lower == upper)
{
}

ScriptBindingInstance* IDBKeyRange::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

} // namespace Starfish

#endif
