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

#ifndef __StarfishIDBIndex__
#define __StarfishIDBIndex__

#include "binding/ScriptWrappable.h"

namespace Starfish {

class IDBObjectStore;

class IDBIndex : public ScriptWrappable {
public:
    IDBIndex(ExecutionContext* executionContext, String* name,
             IDBObjectStore* objectStore);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(IDBIndex)

    DEFINE_GETTER(ExecutionContext*, executionContext);
    DEFINE_GETTER_SETTER(String*, name, Name);
    DEFINE_GETTER(IDBObjectStore*, objectStore);
    DEFINE_GETTER(ScriptValue, keyPath);
    DEFINE_GETTER(bool, multiEntry);
    DEFINE_GETTER(bool, unique);

private:
    ExecutionContext* m_executionContext;
    String* m_name;
    IDBObjectStore* m_objectStore;
    ScriptValue m_keyPath;
    bool m_multiEntry;
    bool m_unique;
};
} // namespace Starfish

#endif
#endif
