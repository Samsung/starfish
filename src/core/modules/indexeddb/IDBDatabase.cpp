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

IDBTransaction* IDBDatabase::startVersionChangeTransaction()
{
    STARFISH_ASSERT(!m_versionChangeTransaction);
    STARFISH_ASSERT(m_executionContext->isContextThread());

    m_versionChangeTransaction = new IDBTransaction(m_executionContext, this);
    m_versionChangeTransaction->setMode(IDBTransactionMode::VersionChange);

    return m_versionChangeTransaction;
}

} // namespace Starfish

#endif
