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
#include "core/dom/DOMStringList.h"
#include "core/dom/DOMException.h"
#include "core/modules/indexeddb/IDBKeyPath.h"
#include "core/modules/indexeddb/IDBObjectStore.h"
#include "core/modules/indexeddb/IDBConnection.h"
#include "core/modules/indexeddb/IDBUtils.h"
#include "core/modules/indexeddb/IDBTransaction.h"
#include "core/modules/indexeddb/IDBDatabase.h"

namespace Starfish {

void IDBTransactionOptions::setDurability(String* string)
{
    m_durability = IDBUtils::transactionDurabilityToType(string);
}

String* IDBTransactionOptions::durability() const
{
    return IDBUtils::transactionDurabilityToString(m_durability);
}

IDBDatabase::IDBDatabase(ExecutionContext* executionContext,
                         IDBConnection* connection, String* name,
                         unsigned long long version)
    : EventTarget()
    , m_executionContext(executionContext)
    , m_connection(connection)
    , m_versionChangeTransaction(nullptr)
    , m_name(name)
    , m_version(version)
    , m_objectStoreNames(new DOMStringList(executionContext))
{
}

IDBTransaction* IDBDatabase::transaction(
    DOMStringOrSequenceOfDOMString storeNames, String* mode,
    const IDBTransactionOptions& options)
{
    if (storeNames.isSequenceOfDOMStringValue()) {
        for (String* name : storeNames.getSequenceOfDOMStringValue()) {
            m_objectStoreNames->push_back(name);
        }
    } else if (storeNames.isDOMStringValue()) {
        m_objectStoreNames->push_back(storeNames.getDOMStringValue());
    }

    return new IDBTransaction(m_executionContext, this,
                              IDBUtils::transactionModeToType(mode),
                              options.m_durability);
}

IDBObjectStore* IDBDatabase::createObjectStore(String* name)
{
    return createObjectStore(name, IDBObjectStoreParameters());
}

IDBObjectStore* IDBDatabase::createObjectStore(String* name,
                                               IDBObjectStoreParameters options)
{
    // https://w3c.github.io/IndexedDB/#dom-idbdatabase-createobjectstore

    if (!m_versionChangeTransaction) {
        throw new DOMException(m_executionContext,
                               DOMException::Code::INVALID_STATE_ERR);
    }

    if (m_versionChangeTransaction->state() != IDBTransaction::State::Active) {
        throw new DOMException(
            m_executionContext,
            String::createASCIIString("The transaction is not active."),
            String::createASCIIString("TransactionInactiveError"));
    }

    IDBKeyPath* keyPath = nullptr;
    if (options.m_keyPath.hasValue()) {
        keyPath = new IDBKeyPath(scriptBindingInstance(),
                                 options.m_keyPath.getValue());
        if (!keyPath->isValid()) {
            throw new DOMException(m_executionContext,
                                   DOMException::Code::SYNTAX_ERR);
        }
    }

    // TODO: 6. If an object store named name already exists in database throw a
    // "ConstraintError" DOMException.

    // TODO: 8. If autoIncrement is true and keyPath is an empty string or any
    // sequence (empty or otherwise), throw an "InvalidAccessError"
    // DOMException.

    IDBObjectStore* store = m_versionChangeTransaction->objectStore(name);
    m_objectStoreNames->push_back(name);

    // TODO: 9. If autoIncrement is true, then the created object store uses a
    // key generator.

    if (keyPath) {
        store->setKeyPath(Nullable<IDBKeyPath*>(keyPath));
    }

    return store;
}

IDBTransaction* IDBDatabase::startVersionChangeTransaction()
{
    STARFISH_ASSERT(!m_versionChangeTransaction);
    STARFISH_ASSERT(m_executionContext->isContextThread());

    m_versionChangeTransaction = new IDBTransaction(m_executionContext, this);
    m_versionChangeTransaction->setMode(IDBTransactionMode::VersionChange);

    return m_versionChangeTransaction;
}

void IDBDatabase::close()
{
    STARFISH_UNIMPLEMENTED();
}

} // namespace Starfish

#endif
