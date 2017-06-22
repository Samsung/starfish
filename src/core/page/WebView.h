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

#ifndef __StarFishPage__
#define __StarFishPage__

#include "StarFishConfig.h"

#include "binding/StarFishHoldable.h"

namespace StarFish {

class Document;
class BrowsingContext;
class StorageNamespaceProvider;
class StorageNamespace;

class WebView : public StarFishHoldable, public gc {
public:
    static WebView* create(StarFish* starFish);

    BrowsingContext* mainBrowsingContext()
    {
        return m_mainBrowsingContext;
    }

    StorageNamespace* localStorageNamespace()
    {
        return m_localStorageNamespace;
    }

    StorageNamespace* sessionStorageNamespace()
    {
        return m_sessionStorageNamespace;
    }

    void navigate(ResourceURL* url);

private:
    WebView(StarFish* starFish);
    void initStorage();

    BrowsingContext* m_mainBrowsingContext;
    StorageNamespaceProvider* m_storageNamespaceProvider;

    StorageNamespace* m_localStorageNamespace;
    StorageNamespace* m_sessionStorageNamespace;
};
}

#endif
