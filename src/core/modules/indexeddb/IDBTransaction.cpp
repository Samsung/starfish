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
#include "core/dom/DOMStringList.h"
#include "core/modules/indexeddb/IDBUtils.h"
#include "core/modules/indexeddb/IDBObjectStore.h"
#include "core/modules/indexeddb/IDBTransaction.h"

namespace Starfish {

IDBTransaction::IDBTransaction(ExecutionContext* executionContext,
                               IDBDatabase* db)
    : EventTarget()
    , m_executionContext(executionContext)
    , m_objectStoreNames(new DOMStringList(executionContext))
    , m_mode(IDBTransactionMode::ReadWrite)
    , m_durability(IDBTransactionDurability::Default)
    , m_db(db)
    , m_error(nullptr)
    , m_state(State::Active)
{
}

IDBObjectStore* IDBTransaction::objectStore(String* name)
{
    return new IDBObjectStore(m_executionContext, this, name);
}

String* IDBTransaction::mode() const
{
    return IDBUtils::transactionModeToString(m_mode);
}

String* IDBTransaction::durability() const
{
    return IDBUtils::transactionDurabilityToString(m_durability);
}

} // namespace Starfish

#endif
