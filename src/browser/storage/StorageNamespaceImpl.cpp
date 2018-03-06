/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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

#include "StarFishConfig.h"
#include "StorageNamespaceImpl.h"

#include "core/storage/Storage.h"
#include "core/storage/StorageType.h"
#include "StorageImpl.h"
#include "StorageManager.h"

namespace StarFish {

StorageNamespaceImpl::StorageNamespaceImpl(StorageType storageType,
                                           String* localStoragePath)
    : m_storageType(storageType)
    , m_storageManager(nullptr)
{
    if (storageType == StorageType::Local &&
        localStoragePath->equals("") == false) {
        m_storageManager = new StorageManager(localStoragePath);
    }
}

StorageNamespaceImpl::~StorageNamespaceImpl()
{
}

Storage* StorageNamespaceImpl::storage(Window* window,
                                       SecurityOriginData* securityOriginData)
{
    StorageImpl* storageImpl = nullptr;
    auto itr = m_originToStorage.find(securityOriginData);
    if (itr == m_originToStorage.end()) {
        storageImpl = new StorageImpl(m_storageType, securityOriginData,
                                      m_storageManager);
        m_originToStorage.insert(
            std::make_pair(securityOriginData, storageImpl));
    } else {
        storageImpl = itr->second;
    }

    Storage* storage = new Storage(window, storageImpl);
    return storage;
}
}
