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

#include "WebView.h"

#include "BrowsingContext.h"
#include "core/dom/Document.h"
#include "core/storage/Storage.h"
#include "core/storage/StorageNamespace.h"
#include "browser/storage/WebStorageNamespaceProvider.h"

namespace StarFish {

WebView* WebView::create(StarFish* starFish)
{
    return new WebView(starFish);
}

WebView::WebView(StarFish* starFish)
    : m_mainBrowsingContext(BrowsingContext::create(starFish, this))
    , m_storageNamespaceProvider(nullptr)
    , m_localStorageNamespace(nullptr)
    , m_sessionStorageNamespace(nullptr)
{
    initStorage();
}

void WebView::initStorage()
{
    // TODO: The name of disk storage file name should be auto-generated
    m_storageNamespaceProvider = WebStorageNamespaceProvider::create(
        String::createASCIIString("./cache/cache.db"));
    m_localStorageNamespace =
        m_storageNamespaceProvider->createLocalStorageNamespace();
    m_sessionStorageNamespace =
        m_storageNamespaceProvider->createSessionStorageNamespace();
}
}
