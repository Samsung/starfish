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

#ifndef __StarfishIDBObjectStore__
#define __StarfishIDBObjectStore__

#include "binding/ScriptWrappable.h"

namespace Starfish {

class DOMStringList;
class IDBTransaction;

class IDBObjectStore : public ScriptWrappable {
public:
    IDBObjectStore(ExecutionContext* executionContext,
                   IDBTransaction* transaction, String* name);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(IDBObjectStore)

    DEFINE_GETTER_SETTER(String*, name, Name);
    DEFINE_GETTER(ScriptValue, keyPath);
    DEFINE_GETTER(DOMStringList*, indexNames);
    DEFINE_GETTER(IDBTransaction*, transaction);
    DEFINE_GETTER(bool, autoIncrement);

private:
    ExecutionContext* m_executionContext;
    String* m_name;
    ScriptValue m_keyPath;
    DOMStringList* m_indexNames;
    IDBTransaction* m_transaction;
    bool m_autoIncrement;
};
} // namespace Starfish

#endif
#endif
