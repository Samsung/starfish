/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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

#ifndef __StarFishResourceLoader__
#define __StarFishResourceLoader__

#include "loader/Resource.h"
#include "loader/TextResource.h"
#include "loader/ImageResource.h"

namespace StarFish {

class ResourceCacheData : public gc {
    friend class ResourceLoader;
    Resource* m_resource;
    uint64_t m_lastUsedTime; // stores tick count
};

class ResourceLoader : public gc {
    friend class Resource;
    friend class ImageResource;
    friend class DocumentOnLoadChecker;
    friend class ResourceAliveChecker;
    friend class ResourceSizeTracer;
public:
    ResourceLoader(Document& doc);

    Resource* fetch(URL* url);
    TextResource* fetchText(URL* url, String* preferredEncoding = String::emptyString);
    ImageResource* fetchImage(URL* url);

    void markDocumentOpenState()
    {
        m_inDocumentOpenState = true;
        m_pendingResourceCountWhileDocumentOpening = 1;
    }

    void notifyEndParseDocument()
    {
        STARFISH_ASSERT(m_pendingResourceCountWhileDocumentOpening > 0);
        m_pendingResourceCountWhileDocumentOpening--;
        fireDocumentOnLoadEventIfNeeded();
    }

    Document* document()
    {
        return m_document;
    }

    void clear()
    {
        cancelAllOfPendingRequests();
        m_imageResourceCache.clear();
        m_imageResourceCacheLRUList.clear();
    }

    void cachePruning();
    void notifyImageResourceActiveState(ImageResource* res);
private:
    void cancelAllOfPendingRequests();
    void cacheHit(Resource* org, Resource* now, Resource::ResourceRequestSyncLevel syncLevel);
    // return value means cache hit
    bool requestResourcePreprocess(Resource* res, Resource::ResourceRequestSyncLevel syncLevel);
    void fireDocumentOnLoadEventIfNeeded();
    bool m_inDocumentOpenState;
    size_t m_pendingResourceCountWhileDocumentOpening;
    Document* m_document;
    std::vector<Resource*, gc_allocator<Resource*>> m_currentLoadingResources;
    std::unordered_map<std::string, ResourceCacheData, std::hash<std::string>, std::equal_to<std::string>, gc_allocator<std::pair<std::string, ResourceCacheData>>> m_imageResourceCache;
    std::list<Resource*, gc_allocator<Resource*>> m_imageResourceCacheLRUList;
    size_t m_resourceCacheSize;
    uint64_t m_lastCachePruneTime;
};

}

#endif
