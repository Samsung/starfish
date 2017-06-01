/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "StorageNamespaceImpl.h"

#include "core/storage/StorageType.h"
#include "StorageImpl.h"
#include "StorageManager.h"

namespace StarFish {

StorageNamespaceImpl::StorageNamespaceImpl(StarFish* starfish,
                                           StorageType storageType,
                                           String* localStoragePath)
    : StorageNamespace(starfish)
    , m_storageType(storageType)
    , m_storageManager(nullptr)
{
    if (storageType == StorageType::Local) {
        m_storageManager = new StorageManager(localStoragePath);
    }
}

StorageNamespaceImpl::~StorageNamespaceImpl()
{
}

Storage* StorageNamespaceImpl::storage(SecurityOriginData* securityOriginData)
{
    auto itr = m_originToStorage.find(securityOriginData);
    if (itr == m_originToStorage.end()) {
        StorageImpl* storage = new StorageImpl(
            m_starFish, m_storageType, securityOriginData, m_storageManager);
        m_originToStorage.insert(std::make_pair(securityOriginData, storage));
        return storage;
    }

    return itr->second;
}
}
