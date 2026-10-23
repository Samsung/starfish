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
#include "core/util/debug/Trace.h"
#include "core/modules/indexeddb/IDBStorageManager.h"
#include "core/modules/indexeddb/MemoryBackingStore.h"
#include "core/modules/indexeddb/IDBOpenDBRequest.h"
#include "core/modules/indexeddb/IDBConnection.h"

namespace Starfish {

void IDBConnection::openDatabase(IDBConnectionData* connectionData,
                                 OpenDBRequestData* data)
{
    // https://w3c.github.io/IndexedDB/#open-a-database-connection

    auto found = connectionData->m_versions.find(data->name);
    bool exists = found != connectionData->m_versions.end();
    unsigned long long dbVersion = exists ? found->second : 1;

    if (!data->version.hasValue()) {
        data->version = Optional<unsigned long long>(dbVersion);
    }

    if (!exists) {
        dbVersion = 0;
    }

    if (dbVersion > data->version.getValue()) {
        data->error = IDBRequestErrorType::VersionError;
        return;
    }

    data->connection = IDBStorageManager::instance().createConnection(
        data->name, data->identifier);
    data->connection->setVersion(data->version.getValue());

    if (dbVersion < data->version.getValue()) {
        // TODO: 10. If db’s version is less than version, then:
        dbVersion = data->version.getValue();
        connectionData->m_versions[data->name] = dbVersion;

        data->upgradeNeeded = true;
    }

    TRACE(IDB, "dbname:", data->name, "version:", dbVersion);
    data->connection->m_backingStore->open(data->name, dbVersion);
}

IDBConnection::IDBConnection(const std::string& dbName,
                             IDBDatabaseIdentifier identifier)
    : m_backingStore(std::make_unique<MemoryBackingStore>())
    , m_dbName(dbName)
    , m_dbIdentifier(identifier)
    , m_version(0)
{
}

IDBRequestErrorType IDBConnection::storeRecode(const std::string& name,
                                               const char* data,
                                               size_t dataSize, int64_t keyHash,
                                               bool noOverwrite)
{
    // TODO:
    // https://w3c.github.io/IndexedDB/#store-a-record-into-an-object-store
    return m_backingStore->addOrPut(name, data, dataSize, keyHash, noOverwrite);
}

bool IDBConnection::retrieveValue(const std::string& name, int64_t keyHash,
                                  char*& data, size_t& dataSize)
{
    // TODO:
    // https://w3c.github.io/IndexedDB/#retrieve-a-value-from-an-object-store
    return m_backingStore->get(name, keyHash, data, dataSize);
}

IDBBackingStore* IDBConnection::backingStore()
{
    return m_backingStore.get();
}

} // namespace Starfish

#endif
