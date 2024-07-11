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
#include "core/modules/threading/Thread.h"
#include "core/modules/indexeddb/IDBTaskQueue.h"
#include "core/modules/indexeddb/IDBStorageManager.h"

namespace Starfish {

IDBStorageManager& IDBStorageManager::instance()
{
    static IDBStorageManager instance;
    return instance;
}

IDBStorageManager::IDBStorageManager()
    : m_isStared(false)
{
    STARFISH_ASSERT(isMainThread());
}

void IDBStorageManager::start()
{
    std::unique_lock<std::mutex> lock(m_mutex);

    if (m_isStared) {
        return;
    }
    m_isStared = true;

    m_taskQueue->run();
}

void IDBStorageManager::dispose()
{
    STARFISH_ASSERT(isMainThread());

    m_taskQueue.release();
}

IDBTaskQueue* IDBStorageManager::taskQueue()
{
    return m_taskQueue.get();
}

} // namespace Starfish

#endif
