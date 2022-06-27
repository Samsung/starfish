/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#include "StarfishConfig.h"
#include "core/storage/StorageInternal.h"

#include "core/dom/WebOrigin.h"

namespace Starfish {

StorageInternal::~StorageInternal()
{
}

StorageInternal::StorageInternal(StorageType storageType, WebOrigin* webOrigin)
    : m_storageType(storageType)
    , m_webOrigin(webOrigin)
{
}

StorageMemory::StorageMemory(StorageType storageType, WebOrigin* webOrigin)
    : StorageInternal(storageType, webOrigin)
{
}

unsigned long StorageMemory::length()
{
    return m_map.size();
}

Nullable<String*> StorageMemory::key(unsigned long index)
{
    if (index >= m_map.size()) {
        return nullptr;
    }
    auto itr = std::next(m_map.begin(), index);
    return itr->first;
}

Nullable<String*> StorageMemory::getItem(String* key)
{
    auto itr = m_map.find(key);
    if (itr == m_map.end()) {
        return nullptr;
    }

    return itr->second;
}

GCVector<String*> StorageMemory::getKeyNames()
{
    GCVector<String*> ret;

    auto iter = m_map.begin();
    while (iter != m_map.end()) {
        ret.push_back(iter->first);
        iter++;
    }

    return ret;
}

bool StorageMemory::setItem(String* key, String* value)
{
    auto iter = m_map.find(key);
    if (iter == m_map.end()) {
        m_map.insert(std::make_pair(key, value));
    } else {
        iter->second = value;
    }

    return true;
}

bool StorageMemory::removeItem(String* key)
{
    m_map.erase(key);

    return true;
}

void StorageMemory::clear()
{
    m_map.clear();
}
} // namespace Starfish
