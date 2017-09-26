/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#include "binding/DocumentHoldable.h"
#include "platform/loader/Resource.h"
#include "platform/loader/TextResource.h"
#include "platform/loader/ImageResource.h"
#include "core/modules/profiling/Profiling.h"

namespace StarFish {

class ResourceCacheData : public gc {
public:
    friend class ResourceLoader;
    Resource* m_resource;
    uint64_t m_lastUsedTime; // stores tick count
};

class ResourceLoader : public gc, public DocumentHoldable {
    friend class Resource;
    friend class ImageResource;
    friend class DocumentOnLoadChecker;
    friend class ResourceAliveChecker;
    friend class ResourceLoaderTracer;

public:
    ResourceLoader(Document* doc);

    // TODO these fetch methods are just copying `pointer of url`
    // but, url is mutable not likes string
    // we should copy url inside of method
    Resource* fetch(ResourceURL* url);
    TextResource* fetchText(ResourceURL* url,
                            String* preferredEncoding = String::emptyString);
    ImageResource* fetchImage(ResourceURL* url);

    void markDocumentOpenState()
    {
        m_isDocumentInOpenState = true;
        m_documentOpenTime = timestamp();
        increasePendingResourceCountWhileDocumentOpening();
    }

    void notifyEndParseDocument()
    {
        decreasePendingResourceCountWhileDocumentOpening();
    }

    void clear()
    {
        cancelAllOfPendingRequests();
        m_imageResourceCache.clear();
        m_imageResourceCacheLRUList.clear();
        m_isDocumentInOpenState = false;
    }

    void cachePruning();
    void notifyImageResourceActiveState(ImageResource* res);

    bool isDocumentInOpenState()
    {
        return m_isDocumentInOpenState;
    }

    uint64_t documentOpenTime()
    {
        return m_documentOpenTime;
    }

    void increasePendingResourceCountWhileDocumentOpening();
    void decreasePendingResourceCountWhileDocumentOpening();

private:
    void cancelAllOfPendingRequests();
    void cacheHit(Resource* org, Resource* now,
                  Resource::ResourceRequestSyncLevel syncLevel);
    // return value means cache hit
    bool requestResourcePreprocess(
        Resource* res, Resource::ResourceRequestSyncLevel syncLevel);
    void fireDocumentOnLoadEventIfNeeded();
    bool m_isDocumentInOpenState;
    uint64_t m_documentOpenTime;
    size_t m_pendingResourceCountWhileDocumentOpening;
    GCVector<Resource*> m_currentLoadingResources;
    GCUnorderedMap<ASCIIString, ResourceCacheData> m_imageResourceCache;
    GCVector<Resource*> m_imageResourceCacheLRUList;
    size_t m_resourceCacheSize;
    uint64_t m_lastCachePruneTime;
};
}

#endif
