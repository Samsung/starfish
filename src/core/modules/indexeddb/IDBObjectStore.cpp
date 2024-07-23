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
#include "core/util/debug/Trace.h"
#include "core/serialize/MemorySerializer.h"
#include "core/modules/indexeddb/IDBKey.h"
#include "core/modules/indexeddb/IDBKeyPath.h"
#include "core/modules/indexeddb/IDBRequest.h"
#include "core/modules/indexeddb/IDBConnection.h"
#include "core/modules/indexeddb/IDBTaskQueue.h"
#include "core/modules/indexeddb/IDBTransaction.h"
#include "core/modules/indexeddb/IDBObjectStore.h"

namespace Starfish {

IDBObjectStore::IDBObjectStore(ExecutionContext* executionContext,
                               IDBTransaction* transaction, String* name)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
    , m_name(name)
    , m_indexNames(new DOMStringList(executionContext))
    , m_transaction(transaction)
    , m_autoIncrement(false)
    , m_deleted(false)
{
}

ScriptBindingInstance* IDBObjectStore::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

IDBRequest* IDBObjectStore::put(ScriptValue value, ScriptValue key)
{
    // https://w3c.github.io/IndexedDB/#dom-idbobjectstore-put
    return addOrPut(value, key, false);
}

IDBRequest* IDBObjectStore::put(ScriptValue value)
{
    return addOrPut(value, scriptUndefined(), false);
}

IDBRequest* IDBObjectStore::add(ScriptValue value, ScriptValue key)
{
    // https://w3c.github.io/IndexedDB/#dom-idbobjectstore-add
    return addOrPut(value, key, true);
}

IDBRequest* IDBObjectStore::add(ScriptValue value)
{
    return addOrPut(value, scriptUndefined(), true);
}

IDBRequest* IDBObjectStore::addOrPut(ScriptValue value, ScriptValue key,
                                     bool noOverwrite)
{
    // https://w3c.github.io/IndexedDB/#add-or-put
    STARFISH_ASSERT(m_executionContext->isContextThread());
    TRACE(IDB);

    if (m_deleted) {
        throw new DOMException(m_executionContext,
                               DOMException::Code::INVALID_STATE_ERR);
    }

    if (m_transaction->state() != IDBTransaction::State::Active) {
        throw new DOMException(
            m_executionContext,
            String::createASCIIString("The transaction is not active."),
            String::createASCIIString("TransactionInactiveError"));
    }

    if (m_transaction->modeEnum() == IDBTransactionMode::ReadOnly) {
        throw new DOMException(
            m_executionContext,
            String::createASCIIString("The transaction is read-only."),
            String::createASCIIString("ReadOnlyError"));
    }

    IDBKey* convertedKey = nullptr;

    if (!m_keyPath.hasValue()) {
        // TODO: 7. If store uses out-of-line keys and has no key generator and
        // key was not given, throw a "DataError" DOMException.
        convertedKey = IDBKey::convertValueToKey(scriptBindingInstance(), key);
        convertedKey->checkInvalid(m_executionContext);

    } else {
        // TODO: 11. If store uses in-line keys, then:
        if (!isNullOrUndefinedScriptValue(key)) {
            throw new DOMException(
                m_executionContext,
                String::createASCIIString("The store uses in-line keys."),
                String::createASCIIString("DataError"));
        }

        convertedKey = m_keyPath->extractKey(value);
        convertedKey->checkInvalid(m_executionContext);
    }

    SerializedTypedData* serializedData =
        MemorySerializer::serialize(m_executionContext, value);
    SerializedRawScriptValueData* rawData =
        serializedData->data()->asSerializedRawScriptValueData();

    IDBRequest* request = new IDBRequest(m_executionContext);

    struct Params : public IDBTaskQueueItemData {
        String* name;
        IDBKey* key;
        SerializedRawScriptValueData* value;
        IDBConnection* connection;
        IDBRequest* request;
        bool noOverwrite;
    };

    Params* p = new (NoGC) Params();
    p->idbRequest = request;
    p->name = m_name;
    p->key = convertedKey;
    p->value = rawData;
    p->connection = m_transaction->db()->connection();
    p->noOverwrite = noOverwrite;

    auto operation = std::make_unique<IDBTaskQueueItem>(
        [](IDBConnectionData* connectionData, IDBTaskQueueItemData* data) {
            Params* p = reinterpret_cast<Params*>(data);

            p->error = p->connection->storeRecode(
                p->name, p->value->internal()->data(),
                p->value->internal()->size(), p->key, p->noOverwrite);
        },
        [](IDBTaskQueueItemData* data) {
            Params* p = reinterpret_cast<Params*>(data);
            if (p->error == IDBRequestErrorType::None) {
                p->idbRequest->success(scriptUndefined());
            } else {
                p->idbRequest->fail(IDBRequest::errorCodeToDOMException(
                    p->idbRequest->executionContext(), p->error));
            }

            GC_FREE(p);
        },
        p);

    request->executeRequest(this, std::move(operation));

    return request;
}

} // namespace Starfish

#endif
