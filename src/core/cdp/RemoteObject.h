/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_CDP) && !defined(__StarfishCDPRemoteObject__)
#define __StarfishCDPRemoteObject__

#include "rapidjson/document.h"

namespace Escargot {
class ObjectRef;
class ValueRef;
} // namespace Escargot

namespace Starfish {

class ScriptBindingInstance;

// objectId handle table for object-typed RemoteObjects. GC: inherited so the
// held ObjectRef* are tracked. Main-thread only.
class RemoteObjectStore : public gc {
public:
    RemoteObjectStore();

    int store(Escargot::ObjectRef* obj); // issue objectId(n)
    Escargot::ObjectRef* lookup(int id);
    void release(int id);
    void reset(); // navigate

private:
    GCUnorderedMap<int, Escargot::ObjectRef*> m_idToObj;
    int m_next;
};

// ValueRef* -> CDP RemoteObject JSON.
void serializeRemoteObject(ScriptBindingInstance* sbi, RemoteObjectStore* store,
                           Escargot::ValueRef* value, bool returnByValue,
                           rapidjson::Value& out,
                           rapidjson::Document::AllocatorType& alloc);

} // namespace Starfish

#endif
