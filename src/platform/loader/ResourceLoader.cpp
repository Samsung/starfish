
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

#include "StarFishConfig.h"

#include "platform/loader/ResourceLoader.h"

#include "StarFish.h"
#include "core/dom/Document.h"
#include "core/dom/Event.h"
#include "core/dom/HTMLImageElement.h"
#include "core/layout/FrameReplacedImage.h"
#include "core/modules/resource_request/ResourceRequest.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/profiling/Profiling.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"

#ifdef STARFISH_ENABLE_TEST
extern bool g_fireOnloadEvent;
#endif

#ifndef STARFISH_RESOURCE_CACHE_SIZE
#define STARFISH_RESOURCE_CACHE_SIZE 1024 * 1024 * 2
#endif

#ifndef STARFISH_RESOURCE_CACHE_PRUNE_MINIMUM_INTERVAL
#define STARFISH_RESOURCE_CACHE_PRUNE_MINIMUM_INTERVAL 3
#endif

namespace StarFish {

ResourceLoader::ResourceLoader(Document* document)
    : DocumentHoldable(document)
    , m_isDocumentInOpenState(false)
    , m_pendingResourceCountWhileDocumentOpening(0)
    , m_resourceCacheSize(0)
    , m_lastCachePruneTime(0)
{
}

Resource* ResourceLoader::fetch(ResourceURL* url)
{
    Resource* res = new Resource(url, this);
    return res;
}

TextResource* ResourceLoader::fetchText(ResourceURL* url,
                                        String* preferredEncoding)
{
    TextResource* res = new TextResource(url, this, preferredEncoding);
    return res;
}

ImageResource* ResourceLoader::fetchImage(ResourceURL* url)
{
    ImageResource* res = new ImageResource(url, this);
    return res;
}

class DocumentOnLoadChecker : public ResourceClient {
public:
    DocumentOnLoadChecker(Resource* res)
        : ResourceClient(res)
        , m_didFire(false)
    {
    }
    virtual void didLoadFailed()
    {
        ResourceClient::didLoadFailed();
        checkFire();
    }

    virtual void didLoadFinished()
    {
        ResourceClient::didLoadFinished();
        checkFire();
    }

    virtual void didLoadCanceled()
    {
        ResourceClient::didLoadCanceled();
        checkFire();
    }

    void checkFire()
    {
        if (m_didFire) {
            return;
        }
        m_didFire = true;
        m_resource->loader()
            ->decreasependingResourceCountWhileDocumentOpening();
    }

    bool m_didFire;
};

class ResourceAliveChecker : public ResourceClient {
public:
    ResourceAliveChecker(Resource* res)
        : ResourceClient(res)
    {
        auto& v = m_resource->loader()->m_currentLoadingResources;
        v.push_back(m_resource);
    }

    virtual void didLoadFailed()
    {
        ResourceClient::didLoadFailed();
        clearAlive();
    }

    virtual void didLoadFinished()
    {
        ResourceClient::didLoadFinished();
        clearAlive();
    }

    virtual void didLoadCanceled()
    {
        ResourceClient::didLoadCanceled();
        clearAlive();
    }

    void clearAlive()
    {
        auto& v = m_resource->loader()->m_currentLoadingResources;
        auto iter = std::find(v.begin(), v.end(), m_resource);
        // TODO prevent remove twice
        if (iter != v.end()) {
            v.erase(iter);
        }
    }
};

class ResourceLoaderTracer : public ResourceClient {
public:
    ResourceLoaderTracer(Resource* res)
        : ResourceClient(res)
    {
    }
    virtual void didLoadFailed()
    {
        ResourceClient::didLoadFailed();
    }

    virtual void didLoadFinished()
    {
        ResourceClient::didLoadFinished();
        m_resource->loader()->m_resourceCacheSize += m_resource->contentSize();
    }

    virtual void didLoadCanceled()
    {
        ResourceClient::didLoadCanceled();
        if (!m_resource->m_isReferencedByAnoterResource) {
            auto& l = m_resource->loader()->m_imageResourceCacheLRUList;
            auto iter = std::find(l.begin(), l.end(), m_resource);
            if (iter != l.end()) {
                l.erase(iter);
            }

            auto& cache = m_resource->loader()->m_imageResourceCache;
            auto u8Str = resource()->url()->urlString()->toUTF8NonGCString();
            ASCIIString url(u8Str.data(), u8Str.length());
            auto iter2 = cache.find(url);
            if (iter2 != cache.end() &&
                iter2->second.m_resource == resource()) {
                cache.erase(iter2);
            }
        }
    }
};

class ResourceWatcher : public ResourceClient {
public:
    ResourceWatcher(Resource* res, Resource* watcher)
        : ResourceClient(res)
        , m_watcher(watcher)
    {
    }
    virtual void didLoadFailed()
    {
        ResourceClient::didLoadFailed();
        m_watcher->didLoadFailed();
    }

    virtual void didLoadFinished()
    {
        ResourceClient::didLoadFinished();
        m_watcher->didCacheHit(m_resource);
    }

    virtual void didLoadCanceled()
    {
        ResourceClient::didLoadCanceled();
        STARFISH_ASSERT(m_resource->m_isCanceledButContinueLoadingDueToCache);
        m_resource->addResourceClient(this);
        m_resource->addResourceClient(new ResourceLoaderTracer(m_resource));
    }

    Resource* m_watcher;
};

static void traverseChildFrames(
    Frame* parent, std::unordered_set<std::string>& currentUsingResourcePaths)
{
    Frame* c = parent;

    if (c->isFrameReplaced() && c->asFrameReplaced()->isFrameReplacedImage()) {
        String* u = ResourceURL::mergeDocumentURIWithURIString(
            c->node()->document(), c->node()->asHTMLImageElement()->src());
        auto utf8Data = u->toUTF8NonGCString();
        currentUsingResourcePaths.insert(utf8Data.data());
    }

    size_t i = 0;
    while (i < c->style()->backgroundLayerSize()) {
        if (c->style()->backgroundImage(i)->length()) {
            auto utf8Data = c->style()->backgroundImage(i)->toUTF8NonGCString();
            currentUsingResourcePaths.insert(utf8Data.data());
        }
        i++;
    }

    c = parent->firstChild();
    while (c) {
        traverseChildFrames(c, currentUsingResourcePaths);
        c = c->next();
    }
}

void ResourceLoader::cachePruning()
{
    // STARFISH_LOG_INFO("ResourceLoader - CacheSize %dKB\n",
    // (int)m_resourceCacheSize / 1024);

    if (m_resourceCacheSize > STARFISH_RESOURCE_CACHE_SIZE &&
        ((tickCount() - m_lastCachePruneTime) >
         (STARFISH_RESOURCE_CACHE_PRUNE_MINIMUM_INTERVAL * 1000))) {
        size_t removedSize = 0;

        std::unordered_set<std::string> currentUsingResourcePaths;

        if (document()->frame()) {
            traverseChildFrames(document()->frame(), currentUsingResourcePaths);
        }

        // remove non-referenced resources
        auto iter = m_imageResourceCache.begin();
        while (iter != m_imageResourceCache.end()) {
            ResourceCacheData data = iter->second;
            auto utf8Data =
                data.m_resource->url()->urlString()->toUTF8NonGCString();
            if ((currentUsingResourcePaths.find(utf8Data.data()) !=
                 currentUsingResourcePaths.end()) &&
                !data.m_resource->m_isReferencedByAnoterResource &&
                data.m_resource->state() == Resource::State::Finished) {
                size_t siz = data.m_resource->contentSize();
                m_resourceCacheSize -= siz;
                removedSize += siz;
                iter = m_imageResourceCache.erase(iter);
            } else {
                iter++;
            }
        }

        // remove old resources
        if (m_resourceCacheSize > STARFISH_RESOURCE_CACHE_SIZE * 0.75) {
            auto iter = m_imageResourceCacheLRUList.begin();
            size_t currentTick = tickCount();
            while (m_imageResourceCacheLRUList.size() &&
                   removedSize < STARFISH_RESOURCE_CACHE_SIZE * 0.25) {
                Resource* res = (*iter);
                auto utf8Data = res->url()->urlString()->toUTF8NonGCString();
                auto iter2 = m_imageResourceCache.find(utf8Data.data());
                if (m_imageResourceCache.end() != iter2 &&
                    res->state() == Resource::State::Finished &&
                    ((currentTick - iter2->second.m_lastUsedTime) >
                     (STARFISH_RESOURCE_CACHE_PRUNE_MINIMUM_INTERVAL * 1000))) {
                    size_t siz = res->contentSize();
                    m_resourceCacheSize -= siz;
                    removedSize += siz;
                    m_imageResourceCache.erase(iter2);
                }
                iter = m_imageResourceCacheLRUList.erase(iter);
            }
        }

        m_lastCachePruneTime = tickCount();
        STARFISH_LOG_INFO(
            "ResourceLoader::cachePruning - prune %dKB current cache size is "
            "%dKB\n",
            (int)removedSize / 1024, (int)m_resourceCacheSize / 1024);
    }
}

void ResourceLoader::notifyImageResourceActiveState(ImageResource* res)
{
    auto utf8Data = res->url()->urlString()->toUTF8NonGCString();
    auto iter = m_imageResourceCache.find(utf8Data.data());
    if (iter != m_imageResourceCache.end()) {
        iter->second.m_lastUsedTime = tickCount();
    }
}

void ResourceLoader::increasependingResourceCountWhileDocumentOpening()
{
    m_pendingResourceCountWhileDocumentOpening++;

    if (!window()->browsingContext()->isMainBrowsingContext()) {
        BrowsingContext* ctx = window()->browsingContext();
        while (ctx->parentBrowsingContext()) {
            ctx = ctx->parentBrowsingContext();
        }
        ctx->document()
            ->resourceLoader()
            .m_pendingResourceCountWhileDocumentOpening++;
    }
}

void ResourceLoader::decreasependingResourceCountWhileDocumentOpening()
{
    STARFISH_ASSERT(m_pendingResourceCountWhileDocumentOpening > 0);
    m_pendingResourceCountWhileDocumentOpening--;
    fireDocumentOnLoadEventIfNeeded();

    if (!window()->browsingContext()->isMainBrowsingContext()) {
        BrowsingContext* ctx = window()->browsingContext();
        while (ctx->parentBrowsingContext()) {
            ctx = ctx->parentBrowsingContext();
        }
        ctx->document()
            ->resourceLoader()
            .m_pendingResourceCountWhileDocumentOpening--;

        ctx->document()->resourceLoader().fireDocumentOnLoadEventIfNeeded();
    }
}

bool ResourceLoader::requestResourcePreprocess(
    Resource* res, Resource::ResourceRequestSyncLevel syncLevel)
{
    if (m_isDocumentInOpenState &&
        res->isThisResourceDoesAffectWindowOnLoad()) {
        increasependingResourceCountWhileDocumentOpening();
        res->addResourceClient(new DocumentOnLoadChecker(res));
        auto it = res->m_resourceClients.begin();
        res->m_resourceClients.insert(it, new ResourceAliveChecker(res));
    }

    // TODO cache every resource
    if (res->isImageResource() &&
        syncLevel != Resource::ResourceRequestSyncLevel::AlwaysSync) {
        auto u8Str = res->url()->urlString()->toUTF8NonGCString();
        ASCIIString url(u8Str.data(), u8Str.length());
        auto iter = m_imageResourceCache.find(url);
        if (iter == m_imageResourceCache.end()) {
            ResourceCacheData data;
            data.m_lastUsedTime = 0;
            data.m_resource = res;
            m_imageResourceCache.insert(std::make_pair(std::move(url), data));
        } else {
            ResourceCacheData& data = iter->second;
            Resource* resourceInCache = data.m_resource;
            data.m_lastUsedTime = tickCount();

            auto iter =
                std::find(m_imageResourceCacheLRUList.begin(),
                          m_imageResourceCacheLRUList.end(), resourceInCache);
            if (m_imageResourceCacheLRUList.end() != iter) {
                m_imageResourceCacheLRUList.erase(iter);
            }

            m_imageResourceCacheLRUList.push_back(res);
            cacheHit(resourceInCache, res, syncLevel);
            return true;
        }

        STARFISH_ASSERT(res->state() == Resource::BeforeSend);
        res->addResourceClient(new ResourceLoaderTracer(res));
    }

    cachePruning();
    return false;
}

void ResourceLoader::cacheHit(Resource* org, Resource* now,
                              Resource::ResourceRequestSyncLevel syncLevel)
{
    Resource::State s = org->state();
    org->m_isReferencedByAnoterResource = true;
    // STARFISH_LOG_INFO("cache hit! %s\n",
    // org->url()->urlString()->toUTF8NonGCString().data());
    if (s == Resource::State::Finished) {
        if (syncLevel ==
            Resource::ResourceRequestSyncLevel::SyncIfAlreadyLoaded) {
            now->didCacheHit(org);
        } else {
            STARFISH_ASSERT(syncLevel ==
                            Resource::ResourceRequestSyncLevel::NeverSync);
            starFish()->messageLoop()->addIdler(
                document()->browsingContext(),
                [](size_t, void* data, void* data2) {
                    Resource* org = (Resource*)data;
                    Resource* now = (Resource*)data2;
                    now->didCacheHit(org);
                },
                org, now);
        }
    } else if (s == Resource::State::Failed) {
        starFish()->messageLoop()->addIdler(document()->browsingContext(),
                                            [](size_t, void* data) {
                                                Resource* now = (Resource*)data;
                                                now->didLoadFailed();
                                            },
                                            now);
    } else {
        org->addResourceClient(new ResourceWatcher(org, now));
    }
}

void ResourceLoader::fireDocumentOnLoadEventIfNeeded()
{
    if (m_pendingResourceCountWhileDocumentOpening == 0 &&
        m_isDocumentInOpenState && !m_document->m_onLoadFired) {
        m_isDocumentInOpenState = false;
        m_document->m_onLoadFired = true;
        starFish()->platformWindow()->registerOrUpdateIdleTimeCleaner();
        starFish()->messageLoop()->addIdler(
            document()->browsingContext(),
            [](size_t handle, void* data) {
                Document* doc = (Document*)data;
                doc->setReadyState(DocumentReadyStateComplete);
                String* eventType =
                    doc->starFish()->staticStrings()->m_load.localName();
                Event* e = new Event(doc, eventType, EventInit(false, false));
                doc->window()->EventTarget::dispatchEvent(e);
#ifdef STARFISH_ENABLE_TEST
                g_fireOnloadEvent = true;
                doc->window()->browsingContext()->setNeedsPainting();
                doc->window()->testStart();
#endif
            },
            document());
    }
}

void ResourceLoader::cancelAllOfPendingRequests()
{
    auto& v = m_currentLoadingResources;
    while (v.size()) {
        v[0]->cancel();
    }
}
}
