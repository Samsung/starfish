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
#include "core/storage/StorageInternal.h"
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

} // namespace Starfish

#endif
