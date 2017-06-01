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

#ifndef __StarFishWebStorageNamespaceProvider__
#define __StarFishWebStorageNamespaceProvider__

#include "core/storage/StorageNamespaceProvider.h"

namespace StarFish {

class StorageNamespaceImpl;

class WebStorageNamespaceProvider : public StorageNamespaceProvider {
public:
    static WebStorageNamespaceProvider* create(Window* window,
                                               String* localStoragePath);

    virtual ~WebStorageNamespaceProvider();

    StorageNamespace* createLocalStorageNamespace();
    StorageNamespace* createSessionStorageNamespace();

private:
    WebStorageNamespaceProvider(Window* windoe, String* localStoragePath);

    String* m_localStoragePath;
    GCUnorderedMap<String*, StorageNamespaceImpl*>
        m_localStoragePathToStorageNamespace;
};
}

#endif
