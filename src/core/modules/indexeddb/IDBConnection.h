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

#ifndef __StarfishIDBConnection__
#define __StarfishIDBConnection__

#include "core/modules/indexeddb/IDBDatabaseIdentifier.h"

namespace Starfish {

class StorageInternal;
class WebOrigin;
class IDBBackingStore;
class IDBKey;
struct OpenDBRequestData;
enum class IDBRequestErrorType : uint8_t;

class IDBConnectionData : public gc {
public:
    IDBConnectionData();

    StorageInternal* storageKey(WebOrigin* origin);

private:
    StorageInternal* m_storageKey;
};

// NOTE: The IDBConnection class is created from Window and Worker by
// IDBStorageManager. And This class is not thread-safe. Functions in this class
// must be called only on the main work thread of IDBTaskQueue.

class IDBConnection {
public:
    friend class IDBStorageManager;

    IDBConnection(const std::string& dbName, IDBDatabaseIdentifier identifier);

    static void openDatabase(IDBConnectionData* connectionData,
                             OpenDBRequestData* data);

    IDBRequestErrorType storeRecode(String* name, const char* value,
                                    size_t valueSize, IDBKey* key,
                                    bool noOverwrite);

    IDBBackingStore* backingStore();

    DEFINE_GETTER(IDBDatabaseIdentifier, dbIdentifier);
    DEFINE_GETTER_SETTER(unsigned long long, version, Version);

private:
    std::unique_ptr<IDBBackingStore> m_backingStore;
    const std::string m_dbName;
    IDBDatabaseIdentifier m_dbIdentifier;
    unsigned long long m_version;
};

} // namespace Starfish

#endif
#endif
