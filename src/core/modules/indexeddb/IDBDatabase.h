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

#ifndef __StarfishIDBDatabase__
#define __StarfishIDBDatabase__

#include "core/dom/EventTarget.h"
#include "binding/generated/DOMStringOrSequenceOfDOMStringUnion.h"

namespace Starfish {

class String;
class DOMStringList;
class IDBTransaction;
class IDBConnection;

enum class IDBTransactionDurability : uint8_t {
    Default,
    Strict,
    Relaxed,
};

struct IDBTransactionOptions {
    void setDurability(String* string);
    String* durability() const;

    IDBTransactionDurability m_durability{ IDBTransactionDurability::Default };
};

struct IDBObjectStoreParameters {
    DEFINE_GETTER_SETTER(Nullable<DOMStringOrSequenceOfDOMString>, keyPath,
                         KeyPath);
    DEFINE_GETTER_SETTER(bool, autoIncrement, AutoIncrement);

    Nullable<DOMStringOrSequenceOfDOMString> m_keyPath;
    bool m_autoIncrement = false;
};

class IDBDatabase : public EventTarget {
public:
    IDBDatabase(ExecutionContext* executionContext, IDBConnection* connection,
                String* name, unsigned long long version);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isIDBDatabase() const;
    virtual ExecutionContext* executionContext() const override
    {
        return m_executionContext;
    }

    IDBTransaction* transaction(DOMStringOrSequenceOfDOMString storeNames,
                                String* mode,
                                const IDBTransactionOptions& options = {});

    IDBObjectStore* createObjectStore(String* name);
    IDBObjectStore* createObjectStore(String* name,
                                      IDBObjectStoreParameters options);

    IDBTransaction* startVersionChangeTransaction();

    void close();

    DEFINE_GETTER(IDBConnection*, connection);
    DEFINE_GETTER(String*, name);
    DEFINE_GETTER(unsigned long long, version);
    DEFINE_GETTER(DOMStringList*, objectStoreNames);

private:
    ExecutionContext* m_executionContext;
    IDBConnection* m_connection;
    IDBTransaction* m_versionChangeTransaction;
    String* m_name;
    unsigned long long m_version;
    DOMStringList* m_objectStoreNames;
};
} // namespace Starfish

#endif
#endif
