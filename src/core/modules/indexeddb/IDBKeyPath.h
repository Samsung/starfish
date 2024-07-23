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

#include "binding/generated/DOMStringOrSequenceOfDOMStringUnion.h"

#ifndef __StarfishIDBKeyPath__
#define __StarfishIDBKeyPath__

namespace Starfish {

class IDBKey;

class IDBKeyPath : public gc {
public:
    enum class Type : uint8_t {
        Null,
        String,
    };

    IDBKeyPath(ScriptBindingInstance* instance);
    IDBKeyPath(ScriptBindingInstance* instance,
               const DOMStringOrSequenceOfDOMString& keyPath);

    bool isValid();

    const GCVector<String*>& strings()
    {
        return m_strings;
    }

    IDBKey* extractKey(ScriptValue value);

private:
    ScriptBindingInstance* m_instance;
    GCVector<String*> m_strings;
    Type m_type;
};

} // namespace Starfish

#endif
#endif
