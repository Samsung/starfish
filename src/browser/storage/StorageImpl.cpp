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

#include "StarFishConfig.h"
#include "StorageImpl.h"

#include "StorageManager.h"
#include "core/page/SecurityOriginData.h"

namespace StarFish {

StorageImpl::StorageImpl(Window* window)
    : Storage(window)
{
}

StorageImpl::~StorageImpl()
{
}

StorageImpl::StorageImpl(Window* window, StorageType storageType,
                         SecurityOriginData* securityOriginData,
                         StorageManager* storageManager)
    : Storage(window)
    , m_storageType(storageType)
    , m_securityOriginData(securityOriginData)
    , m_storageManager(storageManager)
    , m_map(new (GC) GCUnorderedMap<String*, String*>())
{
}

void StorageImpl::copyDataFrom(StorageImpl* storage)
{
    m_map = storage->m_map;
}

unsigned long StorageImpl::length()
{
    return m_map->size();
}

Nullable<String*> StorageImpl::key(unsigned long index)
{
    if (index >= m_map->size()) {
        return nullptr;
    }
    auto itr = std::next(m_map->begin(), index);
    return itr->first;
}

Nullable<String*> StorageImpl::getItem(String* key)
{
    auto itr = m_map->find(key);
    if (itr == m_map->end()) {
        return nullptr;
    }

    return itr->second;
}

void StorageImpl::setItem(String* key, String* value)
{
    Nullable<String*> val = getItem(key);
    if (val.hasValue()) {
        // Update only if the existing value is different
        if (!(val.getValue()->equals(value))) {
            m_map->insert(std::pair<String*, String*>(key, value));
        }
    } else {
        m_map->insert(std::pair<String*, String*>(key, value));
    }
}

void StorageImpl::removeItem(String* key)
{
    m_map->erase(key);
}

void StorageImpl::clear()
{
    m_map->clear();
}
}
