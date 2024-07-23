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

namespace Starfish {

IDBKey* IDBKey::convertValueToKey(ScriptBindingInstance* scriptBindingInstance,
                                  ScriptValue value)
{
    // https://w3c.github.io/IndexedDB/#convert-a-value-to-a-key
    // TODO:
    // 1. If seen was not given, then let seen be a new empty set.
    // 2. If seen contains input, then return invalid.

    if (isNumberScriptValue(value)) {
        if (isInfOrNan(scriptValueAsNumber(value))) {
            return new IDBKey(Type::Invalid);
        } else {
            return new IDBKey(scriptValueAsNumber(value));
        }
    } else if (isStringScriptValue(value)) {
        return new IDBKey(toBrowserString(scriptBindingInstance, value));
    } else if (isNullOrUndefinedScriptValue(value)) {
        return new IDBKey(Type::Null);
    } else {
        STARFISH_UNSUPPORTED(
            "IDB key unsupported type: handle Date, buffer source type, array "
            "exotic object");
        // TODO: handle Date, buffer source type, array exotic object
        return new IDBKey(Type::Invalid);
    }
}

IDBKey::IDBKey(Type type)
    : m_type(type)
{
}

IDBKey::IDBKey(String* stringValue)
{
    m_type = Type::String;
    m_value.m_string = stringValue;
}

IDBKey::IDBKey(double numberValue)
{
    m_type = Type::Number;
    m_value.m_number = numberValue;
}

String* IDBKey::getString()
{
    STARFISH_ASSERT(m_type == Type::String);
    return m_value.m_string;
}

double IDBKey::getNumber()
{
    STARFISH_ASSERT(m_type == Type::Number);
    return m_value.m_number;
}

Nullable<String*> IDBKey::toString()
{
    if (m_type == IDBKey::Type::String) {
        return m_value.m_string;
    } else if (m_type == IDBKey::Type::Number) {
        return String::fromDouble(m_value.m_number);
    } else {
        STARFISH_UNIMPLEMENTED();
        return Nullable<String*>();
    }
}

void IDBKey::checkInvalid(ExecutionContext* executionContext)
{
    if (isInvalid()) {
        throw new DOMException(executionContext,
                               String::createASCIIString("The key is invalid."),
                               String::createASCIIString("DataError"));
    }
}

} // namespace Starfish

#endif
