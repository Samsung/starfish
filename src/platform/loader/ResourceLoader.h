/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishResourceLoader__
#define __StarFishResourceLoader__

#include "binding/DocumentHoldable.h"
#include "platform/loader/Resource.h"
#include "platform/loader/TextResource.h"
#include "platform/loader/ImageResource.h"
#include "platform/loader/FontResource.h"
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

    Resource* fetch(ResourceURL* url);
    TextResource* fetchText(ResourceURL* url,
                            String* preferredEncoding = String::emptyString);
    ImageResource* fetchImage(ResourceURL* url);
    FontResource* fetchFont(ResourceURL* url);

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

    void updateDocumentOpenTime()
    {
        m_documentOpenTime = timestamp();
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
    GCUnorderedMap<UTF8String, ResourceCacheData> m_fontResourceCache;
    GCUnorderedMap<UTF8String, ResourceCacheData> m_imageResourceCache;
    GCVector<Resource*> m_imageResourceCacheLRUList;
    size_t m_resourceCacheSize;
    uint64_t m_lastCachePruneTime;
};
}

#endif
