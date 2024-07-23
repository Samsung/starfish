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
#include "core/modules/indexeddb/IDBKey.h"
#include "core/modules/indexeddb/IDBKeyPath.h"

namespace Starfish {

IDBKeyPath::IDBKeyPath(ScriptBindingInstance* instance)
    : m_instance(instance)
    , m_type(Type::Null)

{
}

IDBKeyPath::IDBKeyPath(ScriptBindingInstance* instance,
                       const DOMStringOrSequenceOfDOMString& keyPath)
    : m_instance(instance)
{
    if (keyPath.isDOMStringValue()) {
        m_type = Type::String;
        m_strings.push_back(keyPath.getDOMStringValue());
    } else {
        m_type = Type::Null;
        STARFISH_UNIMPLEMENTED();
    }
}

bool IDBKeyPath::isValid()
{
    return m_type != Type::Null;
}

IDBKey* IDBKeyPath::extractKey(ScriptValue value)
{
    // TODO:
    // https://w3c.github.io/IndexedDB/#extract-a-key-from-a-value-using-a-key-path

    if (!isObjectScriptValue(value)) {
        new IDBKey(IDBKey::Type::Invalid);
    }

    for (auto& key : m_strings) {
        ScriptValue property = getScriptObjectOwnProperty(
            m_instance, scriptValueAsObject(value),
            createScriptValue(createScriptString(key)));

        if (!isNullOrUndefinedScriptValue(property)) {
            return IDBKey::convertValueToKey(m_instance, property);
        }
    }

    return new IDBKey(IDBKey::Type::Invalid);
}

} // namespace Starfish

#endif
