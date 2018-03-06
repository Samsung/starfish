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

#include "WebStorageNamespaceProvider.h"

#include "StorageNamespaceImpl.h"

namespace StarFish {

WebStorageNamespaceProvider::WebStorageNamespaceProvider(
    String* localStoragePath)
    : m_localStoragePath(localStoragePath)
{
}

WebStorageNamespaceProvider::~WebStorageNamespaceProvider()
{
}

WebStorageNamespaceProvider* WebStorageNamespaceProvider::create(
    String* localStoragePath)
{
    return new WebStorageNamespaceProvider(localStoragePath);
}

StorageNamespace* WebStorageNamespaceProvider::createLocalStorageNamespace()
{
    auto itr = m_localStoragePathToStorageNamespace.find(m_localStoragePath);
    if (itr == m_localStoragePathToStorageNamespace.end()) {
        StorageNamespaceImpl* storageNamespace =
            new StorageNamespaceImpl(StorageType::Local, m_localStoragePath);
        m_localStoragePathToStorageNamespace.insert(
            std::pair<String*, StorageNamespaceImpl*>(m_localStoragePath,
                                                      storageNamespace));
        return storageNamespace;
    }

    return itr->second;
}

StorageNamespace* WebStorageNamespaceProvider::createSessionStorageNamespace()
{
    StorageNamespaceImpl* storageNamespace =
        new StorageNamespaceImpl(StorageType::Session, nullptr);

    return storageNamespace;
}
}
