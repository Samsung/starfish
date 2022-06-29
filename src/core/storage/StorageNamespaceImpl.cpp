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
#include "StorageNamespaceImpl.h"

#include "core/storage/StorageType.h"
#include "core/storage/StorageInternal.h"
#include "core/storage/StoragePersistent.h"

namespace Starfish {

StorageNamespaceImpl::StorageNamespaceImpl(StorageType storageType,
                                           Nullable<String*> localStoragePath)
    : m_storageType(storageType)
    , m_localStoragePath(localStoragePath)
{
}

StorageInternal* StorageNamespaceImpl::storageInternal(WebOrigin* webOrigin)
{
    StorageInternal* storageInternal = nullptr;
    auto itr = m_originToStorage.find(webOrigin);
    if (itr == m_originToStorage.end()) {
        if (m_storageType == StorageType::Local &&
            m_localStoragePath.hasValue()) {
            storageInternal = new StoragePersistent(m_storageType, webOrigin,
                                                    m_localStoragePath.value());
        } else {
            storageInternal = new StorageMemory(m_storageType, webOrigin);
        }
        m_originToStorage.insert(std::make_pair(webOrigin, storageInternal));
    } else {
        storageInternal = itr->second;
    }

    return storageInternal;
}
} // namespace Starfish
