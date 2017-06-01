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

#include "StorageImpl.h"

#include "StorageManager.h"
#include "core/page/SecurityOriginData.h"

namespace StarFish {

StorageImpl::StorageImpl(StarFish* starfish)
    : Storage(starfish)
{
}

StorageImpl::~StorageImpl()
{
}

StorageImpl::StorageImpl(StarFish* starfish, StorageType storageType,
                         SecurityOriginData* securityOriginData,
                         StorageManager* storageManager)
    : Storage(starfish)
    , m_storageType(storageType)
    , m_securityOriginData(securityOriginData)
    , m_storageManager(storageManager)
{
}

unsigned long StorageImpl::length()
{
    return m_map.size();
}

String* StorageImpl::key(unsigned long index)
{
    if (index >= m_map.size()) {
        return nullptr;
    }
    auto itr = std::next(m_map.begin(), index);
    return itr->first;
}

String* StorageImpl::getItem(String* key)
{
    auto itr = m_map.find(key);
    if (itr == m_map.end()) {
        return nullptr;
    }

    return itr->second;
}

void StorageImpl::setItem(String* key, String* value)
{
    String* val = getItem(key);
    if (val == nullptr) {
        m_map.insert(std::pair<String*, String*>(key, value));
    } else {
        // Update only if the existing value is different
        if (!(val->equals(value))) {
            m_map.insert(std::pair<String*, String*>(key, value));
        }
    }
}

void StorageImpl::removeItem(String* key)
{
    m_map.erase(key);
}

void StorageImpl::clear()
{
    m_map.clear();
}
}
