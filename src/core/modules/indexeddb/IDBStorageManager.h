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

#ifndef __StarfishIDBStorageManager__
#define __StarfishIDBStorageManager__

#include "core/modules/indexeddb/IDBDatabaseIdentifier.h"

namespace Starfish {

class IDBTaskQueue;
class IDBConnection;

class IDBStorageManager {
public:
    static IDBStorageManager& instance();

    ~IDBStorageManager();

    void start();

    void dispose();

    IDBTaskQueue* taskQueue();

    String* getLocalStoragePath();
    String* getIDBLocalStoragePath();

    IDBConnection* createConnection(const std::string& name,
                                    IDBDatabaseIdentifier identifier);

private:
    IDBStorageManager();

    bool m_isStared;
    std::mutex m_mutex;
    std::unique_ptr<IDBTaskQueue> m_taskQueue;
    std::vector<std::unique_ptr<IDBConnection>> m_connections;
};

} // namespace Starfish

#endif
#endif
