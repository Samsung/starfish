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
#include "core/dom/DOMException.h"
#include "core/modules/indexeddb/IDBStorageManager.h"
#include "core/modules/indexeddb/IDBConnection.h"
#include "core/modules/indexeddb/IDBDatabase.h"
#include "core/modules/indexeddb/IDBOpenDBRequest.h"
#include "core/modules/indexeddb/IDBFactory.h"

namespace Starfish {

IDBFactory::IDBFactory(ExecutionContext* executionContext)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
{
}

ScriptBindingInstance* IDBFactory::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

IDBOpenDBRequest* IDBFactory::open(String* name)
{
    return open(name, Nullable<unsigned long long>());
}

IDBOpenDBRequest* IDBFactory::open(String* name, unsigned long long version)
{
    return open(name, Nullable<unsigned long long>(version));
}

IDBOpenDBRequest* IDBFactory::open(String* name,
                                   Nullable<unsigned long long> version)
{
    // https://w3c.github.io/IndexedDB/#dom-idbfactory-open

    if (version.hasValue() && version.getValue() == 0) {
        throw new DOMException(m_executionContext,
                               DOMException::Code::SCRIPT_TYPE_ERR);
    }

    IDBOpenDBRequest* request = new IDBOpenDBRequest(m_executionContext);

    OpenDBRequestData* data = new (NoGC) OpenDBRequestData();
    data->idbRequest = request;
    data->name = name;
    data->version = version;
    data->webOrigin = m_executionContext->webOrigin();
    data->identifier =
        IDBDatabaseIdentifier(m_executionContext->webOrigin(), name);

    auto task = std::make_unique<IDBTaskQueueItem>(
        [](IDBConnectionData* connectionData, IDBTaskQueueItemData* data) {
            OpenDBRequestData* requestData =
                reinterpret_cast<OpenDBRequestData*>(data);
            IDBConnection::openDatabase(connectionData, requestData);
        },
        [](IDBTaskQueueItemData* data) {
            OpenDBRequestData* requestData =
                reinterpret_cast<OpenDBRequestData*>(data);
            IDBOpenDBRequest* request =
                reinterpret_cast<IDBOpenDBRequest*>(requestData->idbRequest);
            STARFISH_ASSERT(request->isOpenDBRequest());

            if (requestData->error != OpenDBRequestErrorType::None) {
                request->failOpenRequest(requestData->error);
            } else {
                STARFISH_ASSERT(requestData->version.hasValue());

                IDBDatabase* db = new IDBDatabase(
                    request->executionContext(), requestData->connection,
                    requestData->name, requestData->version.getValue());

                request->setDatabase(db);

                if (requestData->upgradeNeeded) {
                    request->upgradeNeeded();
                }

                request->successOpenRequest();
            }

            GC_FREE(requestData);
        },
        data);

    IDBStorageManager::instance().taskQueue()->addTask(std::move(task));

    return request;
}

} // namespace Starfish

#endif
