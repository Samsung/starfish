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

#include "StarfishConfig.h"
#include "StorageImpl.h"

#include "StorageManager.h"
#include "core/dom/WebOrigin.h"

namespace Starfish {

StorageImpl::~StorageImpl()
{
}

StorageImpl::StorageImpl(StorageType storageType, WebOrigin* webOrigin,
                         StorageManager* storageManager)
    : m_storageType(storageType)
    , m_webOrigin(webOrigin)
    , m_storageManager(storageManager)
    , m_map()
{
    if (m_storageManager) {
        m_storageManager->load(m_map, m_webOrigin);
    }
}

unsigned long StorageImpl::length()
{
    return m_map.size();
}

Nullable<String*> StorageImpl::key(unsigned long index)
{
    if (index >= m_map.size()) {
        return nullptr;
    }
    auto itr = std::next(m_map.begin(), index);
    return itr->first;
}

Nullable<String*> StorageImpl::getItem(String* key)
{
    auto itr = m_map.find(key);
    if (itr == m_map.end()) {
        if (m_storageManager) {
            return m_storageManager->getItem(m_webOrigin, key);
        }
        return nullptr;
    }

    return itr->second;
}

GCVector<String*> StorageImpl::getKeyNames()
{
    GCVector<String*> ret;

    auto iter = m_map.begin();
    while (iter != m_map.end()) {
        ret.push_back(iter->first);
        iter++;
    }

    return ret;
}

bool StorageImpl::setItem(String* key, String* value)
{
    auto iter = m_map.find(key);
    if (iter == m_map.end()) {
        m_map.insert(std::make_pair(key, value));
    } else {
        iter->second = value;
    }
    if (m_storageManager) {
        m_storageManager->setItem(m_webOrigin, key, value);
    }
    return true;
}

bool StorageImpl::removeItem(String* key)
{
    m_map.erase(key);
    if (m_storageManager) {
        m_storageManager->removeItem(m_webOrigin, key);
    }
    return true;
}

void StorageImpl::clear()
{
    m_map.clear();
    if (m_storageManager) {
        m_storageManager->clear(m_webOrigin);
    }
}
}
