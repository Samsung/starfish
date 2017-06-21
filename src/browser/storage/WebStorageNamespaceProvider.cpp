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
