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

#ifndef __StarfishIDBTransaction__
#define __StarfishIDBTransaction__

#include "core/dom/EventTarget.h"
#include "core/modules/indexeddb/IDBDatabase.h"

namespace Starfish {

class IDBObjectStore;

enum class IDBTransactionMode : uint8_t { ReadOnly, ReadWrite, VersionChange };

class IDBTransaction : public EventTarget {
public:
    // https://w3c.github.io/IndexedDB/#transaction-state
    enum class State : uint8_t {
        Active,
        Inactive,
        Committing,
        Finished,
    };

    IDBTransaction(ExecutionContext* executionContext, IDBDatabase* db,
                   IDBTransactionMode mode,
                   IDBTransactionDurability durability);
    IDBTransaction(ExecutionContext* executionContext, IDBDatabase* db);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isIDBTransaction() const;
    virtual ExecutionContext* executionContext() const override
    {
        return m_executionContext;
    }

    IDBObjectStore* objectStore(String* name);

    String* mode() const;
    void setMode(IDBTransactionMode mode)
    {
        m_mode = mode;
    }
    IDBTransactionMode modeEnum() const
    {
        return m_mode;
    }

    String* durability() const;

    void addRequest(IDBRequest* request);
    void removeRequest(IDBRequest* request);

    DEFINE_GETTER(DOMStringList*, objectStoreNames);
    DEFINE_GETTER(IDBDatabase*, db);
    DEFINE_GETTER(DOMException*, error);
    DEFINE_GETTER_SETTER(State, state, State);

private:
    ExecutionContext* m_executionContext;
    DOMStringList* m_objectStoreNames;
    IDBTransactionMode m_mode;
    IDBTransactionDurability m_durability;
    IDBDatabase* m_db;
    DOMException* m_error;
    State m_state;
    GCVector<IDBRequest*> m_requestList;
};
} // namespace Starfish

#endif
#endif
