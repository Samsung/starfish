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
#include "core/storage/StorageInternal.h"
#include "core/modules/indexeddb/IDBStorageManager.h"
#include "core/modules/indexeddb/MemoryBackingStore.h"
#include "core/modules/indexeddb/IDBKeyRange.h"
#include "core/modules/indexeddb/IDBOpenDBRequest.h"
#include "core/modules/indexeddb/IDBConnection.h"

namespace Starfish {

IDBConnectionData::IDBConnectionData()
    : m_storageKey(nullptr)
{
}

StorageInternal* IDBConnectionData::storageKey(WebOrigin* origin)
{
    if (m_storageKey == nullptr) {
        // TODO: change to persistent storage
        // resolve the issues when multiple processes modify the same storage
        // key.
        m_storageKey = new StorageMemory(StorageType::Local, origin);
    }

    return m_storageKey;
}

void IDBConnection::openDatabase(IDBConnectionData* connectionData,
                                 OpenDBRequestData* data)
{
    // https://w3c.github.io/IndexedDB/#open-a-database-connection

    StorageInternal* storageKey = connectionData->storageKey(data->webOrigin);

    // key is name, value is version.
    Nullable<String*> db = storageKey->getItem(data->name);
    unsigned long long dbVersion = 1;

    if (db.hasValue()) {
        std::istringstream iss(CSTR(db.getValue()));
        iss >> dbVersion;
    }

    if (!data->version.hasValue()) {
        data->version = Nullable<unsigned long long>(dbVersion);
    }

    if (!db.hasValue()) {
        dbVersion = 0;
    }

    if (dbVersion > data->version.getValue()) {
        data->error = IDBRequestErrorType::VersionError;
        return;
    }

    data->connection = IDBStorageManager::instance().createConnection(
        data->name->toUTF8NonGCString(), data->identifier);
    data->connection->setVersion(data->version.getValue());

    if (dbVersion < data->version.getValue()) {
        // TODO: 10. If db’s version is less than version, then:
        dbVersion = data->version.getValue();
        std::string versionString = std::to_string(dbVersion);
        storageKey->setItem(data->name,
                            String::createASCIIString(versionString.c_str(),
                                                      versionString.length()));

        data->upgradeNeeded = true;
    }

    TRACE(IDB, "dbname:", CSTR(data->name), "version:", dbVersion);
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

IDBRequestErrorType IDBConnection::storeRecode(String* name, const char* data,
                                               size_t dataSize, IDBKey* key,
                                               bool noOverwrite)
{
    // TODO:
    // https://w3c.github.io/IndexedDB/#store-a-record-into-an-object-store
    return m_backingStore->addOrPut(name, data, dataSize, key, noOverwrite);
}

bool IDBConnection::retrieveValue(String* name, IDBKeyRange* range, char*& data,
                                  size_t& dataSize)
{
    // TODO:
    // https://w3c.github.io/IndexedDB/#retrieve-a-value-from-an-object-store
    if (!range->isOnly()) {
        return false;
    }

    return m_backingStore->get(name, range->lower(), data, dataSize);
}

IDBBackingStore* IDBConnection::backingStore()
{
    return m_backingStore.get();
}

} // namespace Starfish

#endif
