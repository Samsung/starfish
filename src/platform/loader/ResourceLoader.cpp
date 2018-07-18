
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

#include "StarFishConfig.h"

#include "platform/loader/ResourceLoader.h"

#include "StarFish.h"
#include "core/dom/Document.h"
#include "core/dom/Event.h"
#include "core/dom/HTMLImageElement.h"
#include "core/dom/HTMLIFrameElement.h"
#include "core/layout/FrameReplacedImage.h"
#include "core/layout/StackingContext.h"
#include "core/modules/resource_request/ResourceRequest.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/profiling/Profiling.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "core/page/WebView.h"
#include "platform/window/PlatformWindow.h"

#ifdef STARFISH_ENABLE_TEST
extern bool g_fireOnloadEvent;
#endif

#ifndef STARFISH_RESOURCE_CACHE_SIZE
#define STARFISH_RESOURCE_CACHE_SIZE 1024 * 1024 * 4
#endif

namespace StarFish {

ResourceLoader::ResourceLoader(Document* document)
    : DocumentHoldable(document)
    , m_isDocumentInOpenState(false)
    , m_documentOpenTime(0)
    , m_pendingResourceCountWhileDocumentOpening(0)
    , m_resourceCacheSize(0)
    , m_downloadedResourceContentSize(0)
    , m_loadProgress(0)
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

ImageResource* ResourceLoader::fetchImage(ResourceURL* url,
                                          bool shouldDecodingInstantly)
{
    ImageResource* res = new ImageResource(url, this, shouldDecodingInstantly);
    return res;
}

FontResource* ResourceLoader::fetchFont(ResourceURL* url)
{
    FontResource* res = new FontResource(url, this);
    return res;
}

HeaderResource* ResourceLoader::fetchHeader(ResourceURL* url)
{
    HeaderResource* res = new HeaderResource(url, this);
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
            ->decreasePendingResourceCountWhileDocumentOpening();
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
        // TODO
        int errorCode = 1;
        resource()->loader()->document()->starFish()->callWebViewHandler(
            std::string("OnReceivedError"), resource()->url()->urlString(),
            errorCode);
    }

    virtual void didLoadFinished()
    {
        ResourceClient::didLoadFinished();
        clearAlive();
        resource()->loader()->updateLoadProgress();
        resource()->loader()->document()->starFish()->callWebViewHandler(
            std::string("OnLoadResource"), resource()->url()->urlString());
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
        m_resource->loader()->m_downloadedResourceContentSize +=
            m_resource->contentSize();
    }

    virtual void didLoadCanceled()
    {
        ResourceClient::didLoadCanceled();
        if (!m_resource->m_isReferencedByAnoterResource) {
            if (m_resource->isImageResource()) {
                auto& l = m_resource->loader()->m_imageResourceCacheLRUList;
                auto iter = std::find(l.begin(), l.end(), m_resource);
                if (iter != l.end()) {
                    l.erase(iter);
                }

                auto& cache = m_resource->loader()->m_imageResourceCache;
                auto u8Str =
                    resource()->url()->urlString()->toUTF8NonGCString();
                ASCIIString url(u8Str.data(), u8Str.length());
                auto iter2 = cache.find(url);
                if (iter2 != cache.end() &&
                    iter2->second.m_resource == resource()) {
                    cache.erase(iter2);
                }
            } else if (m_resource->isFontResource()) {
                auto& cache = m_resource->loader()->m_fontResourceCache;
                auto u8Str =
                    resource()->url()->urlString()->toUTF8NonGCString();
                ASCIIString url(u8Str.data(), u8Str.length());
                auto iter2 = cache.find(url);
                if (iter2 != cache.end() &&
                    iter2->second.m_resource == resource()) {
                    cache.erase(iter2);
                }
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

static void registerImageURLs(
    Frame* c, std::unordered_set<std::string>& currentUsingResourcePaths)
{
    if (c->isFrameReplaced() && c->asFrameReplaced()->isFrameReplacedImage()) {
        String* u = ResourceURL::mergeDocumentURIWithURIString(
            c->node()->document(), c->node()->asHTMLImageElement()->src());
        auto utf8Data = u->toUTF8NonGCString();
        currentUsingResourcePaths.insert(utf8Data.data());
    }

    ComputedStyle* cs = c->style();

    if (cs) {
        size_t i = 0;
        while (i < cs->backgroundLayerSize()) {
            if (cs->backgroundImage(i) &&
                cs->backgroundImage(i)->type().isURL()) {
                auto utf8Data =
                    cs->backgroundImage(i)->urlValue()->toUTF8NonGCString();
                currentUsingResourcePaths.insert(utf8Data.data());
            }
            i++;
        }
    }
}

void ResourceLoader::cachePruning()
{
    // STARFISH_LOG_INFO("ResourceLoader - CacheSize %dKB\n",
    // (int)m_resourceCacheSize / 1024);

    if (m_resourceCacheSize > (STARFISH_RESOURCE_CACHE_SIZE * 0.75) &&
        m_downloadedResourceContentSize >
            (STARFISH_RESOURCE_CACHE_SIZE * 0.5)) {
        m_downloadedResourceContentSize = 0;
        size_t removedSize = 0;

        std::unordered_set<std::string> currentUsingResourcePaths;

        if (document()->frame()) {
            document()->frame()->asFrameBox()->iterateChildFrameBox(
                [&](FrameBox* box) {
                    registerImageURLs(box, currentUsingResourcePaths);
                });
        }

        // remove non-referenced resources
        auto iter = m_imageResourceCache.begin();
        while (iter != m_imageResourceCache.end()) {
            ResourceCacheData data = iter->second;
            auto utf8Data =
                data.m_resource->url()->urlString()->toUTF8NonGCString();
            if ((currentUsingResourcePaths.find(utf8Data.data()) ==
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
        if (m_resourceCacheSize > STARFISH_RESOURCE_CACHE_SIZE * 0.5) {
            auto iter = m_imageResourceCacheLRUList.begin();
            size_t currentTick = tickCount();
            while (m_imageResourceCacheLRUList.size() &&
                   removedSize < STARFISH_RESOURCE_CACHE_SIZE * 0.25) {
                Resource* res = (*iter);
                auto utf8Data = res->url()->urlString()->toUTF8NonGCString();
                auto iter2 = m_imageResourceCache.find(utf8Data.data());
                if (m_imageResourceCache.end() != iter2 &&
                    res->state() == Resource::State::Finished) {
                    size_t siz = res->contentSize();
                    m_resourceCacheSize -= siz;
                    removedSize += siz;
                    m_imageResourceCache.erase(iter2);
                }
                iter = m_imageResourceCacheLRUList.erase(iter);
            }
        }

#if !defined(OS_WINDOWS)
        auto& globalImages = NativeImageData::everyNativeImageInstances();
        for (size_t i = 0; i < globalImages.size(); i++) {
            globalImages[i]->m_isSeenByGC = false;
        }
        GC_gcollect();
        GC_disable();
        GC_enumerate_reachable_objects_inner(
            [](void* obj, size_t bytes, void* cd) {
                size_t size;
                int kind = GC_get_kind_and_size(obj, &size);
                STARFISH_ASSERT(size == bytes);
                void* ptr = GC_USR_PTR_FROM_BASE(obj);

                int srcKind = (int)(size_t)cd;
                if (kind == srcKind) {
                    ((NativeImageData*)ptr)->m_isSeenByGC = true;
                }
            },
            (void*)(size_t)NativeImageData::nativeImageDataGCKind());
        GC_enable();
        for (size_t i = 0; i < globalImages.size(); i++) {
            if (!globalImages[i]->m_isSeenByGC) {
                if (*(int*)globalImages[i]) {
                    delete globalImages[i];
                } else {
                    globalImages.erase(globalImages.begin() + i);
                }
                i--;
            }
        }
#endif
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

void ResourceLoader::increasePendingResourceCountWhileDocumentOpening()
{
    m_pendingResourceCountWhileDocumentOpening++;

    if (!window()->browsingContext()->isTopLevelBrowsingContext()) {
        BrowsingContext* ctx = window()->browsingContext();
        while (ctx->parentBrowsingContext()) {
            ctx = ctx->parentBrowsingContext();
        }
        ctx->document()
            ->resourceLoader()
            .m_pendingResourceCountWhileDocumentOpening++;
    }
}

void ResourceLoader::decreasePendingResourceCountWhileDocumentOpening()
{
    if (m_pendingResourceCountWhileDocumentOpening > 0) {
        m_pendingResourceCountWhileDocumentOpening--;
        fireDocumentOnLoadEventIfNeeded();
    }

    if (!window()->browsingContext()->isTopLevelBrowsingContext()) {
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
        increasePendingResourceCountWhileDocumentOpening();
        res->addResourceClient(new DocumentOnLoadChecker(res));
        auto it = res->m_resourceClients.begin();
        res->m_resourceClients.insert(it, new ResourceAliveChecker(res));
    }

    // TODO cache text resource
    if (syncLevel != Resource::ResourceRequestSyncLevel::AlwaysSync) {
        if (res->isImageResource()) {
            auto u8Str = res->url()->urlString()->toUTF8NonGCString();
            ASCIIString url(u8Str.data(), u8Str.length());
            auto iter = m_imageResourceCache.find(url);
            if (iter == m_imageResourceCache.end()) {
                ResourceCacheData data;
                data.m_lastUsedTime = 0;
                data.m_resource = res;
                m_imageResourceCache.insert(
                    std::make_pair(std::move(url), data));
            } else {
                ResourceCacheData& data = iter->second;
                Resource* resourceInCache = data.m_resource;
                data.m_lastUsedTime = tickCount();

                auto iter = std::find(m_imageResourceCacheLRUList.begin(),
                                      m_imageResourceCacheLRUList.end(),
                                      resourceInCache);
                if (m_imageResourceCacheLRUList.end() != iter) {
                    m_imageResourceCacheLRUList.erase(iter);
                }

                m_imageResourceCacheLRUList.push_back(res);
                cacheHit(resourceInCache, res, syncLevel);
                return true;
            }

            STARFISH_ASSERT(res->state() == Resource::BeforeSend);
            res->addResourceClient(new ResourceLoaderTracer(res));
        } else if (res->isFontResource()) {
            auto u8Str = res->url()->urlString()->toUTF8NonGCString();
            ASCIIString url(u8Str.data(), u8Str.length());
            auto iter = m_fontResourceCache.find(url);
            if (iter == m_fontResourceCache.end()) {
                ResourceCacheData data;
                data.m_lastUsedTime = 0;
                data.m_resource = res;
                m_fontResourceCache.insert(
                    std::make_pair(std::move(url), data));
            } else {
                ResourceCacheData& data = iter->second;
                Resource* resourceInCache = data.m_resource;
                data.m_lastUsedTime = tickCount();
                cacheHit(resourceInCache, res, syncLevel);
                return true;
            }

            STARFISH_ASSERT(res->state() == Resource::BeforeSend);
            res->addResourceClient(new ResourceLoaderTracer(res));
        }
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
            [](size_t, void* data) {
                Document* doc = (Document*)data;
                doc->webView()->setNeedsRendering();
                doc->starFish()
                    ->platformWindow()
                    ->webView()
                    ->addDidRenderingCallback(
                        doc->browsingContext(),
                        [](void* data) {
                            Document* doc = (Document*)data;
                            doc->setReadyState(DocumentReadyStateComplete);
                            if (!doc->doesParticipateInRendering()) {
                                return;
                            }
                            String* eventType = doc->starFish()
                                                    ->staticStrings()
                                                    ->m_load.localName();
                            Event* e = new Event(doc, eventType,
                                                 EventInit(false, false));
                            doc->window()->dispatchEventByUA(e);
                            if (!doc->browsingContext()
                                     ->isTopLevelBrowsingContext()) {
                                doc->browsingContext()
                                    ->sourceElement()
                                    ->childBrowsingContextLoaded();
                            } else {
                                doc->starFish()->callWebViewHandler(
                                    std::string("OnPageLoaded"),
                                    doc->urlString());
                            }
#ifdef STARFISH_ENABLE_TEST
                            if (doc->browsingContext()
                                    ->isTopLevelBrowsingContext()) {
                                // We need to wait few milliseconds
                                // because some tests has setTimeout(0,...) in
                                // onload
                                // handler
                                doc->window()->setTimeout(
                                    [](Window* window, void* data) {
                                        g_fireOnloadEvent = true;
                                        window->document()->setNeedsPainting();
                                        window->window()->testStart();
                                    },
                                    10, nullptr);
                            }
#endif
                        },
                        doc);
            },
            document());
    }
}

void ResourceLoader::cancelAllOfPendingRequests()
{
    m_pendingResourceCountWhileDocumentOpening = 0;
    auto& v = m_currentLoadingResources;
    while (v.size()) {
        v[0]->cancel();
    }
}

void ResourceLoader::resetLoadProgress()
{
    m_loadProgress = 0;
    m_loadProgressState = LoadProgressState::Normal;
}

void ResourceLoader::startLoadProgressTracking()
{
    resetLoadProgress();
    m_loadProgress = 10;
    starFish()->callWebViewHandler(std::string("OnProgressChanged"), nullptr,
                                   m_loadProgress);
}

void ResourceLoader::setLoadProgressState(LoadProgressState state)
{
    if (m_loadProgress == 0) {
        return;
    }
    m_loadProgressState = state;
    if (m_loadProgressState == LoadProgressState::DomContentLoaded) {
        updateLoadProgress();
    }
}

void ResourceLoader::updateLoadProgress()
{
    if (m_loadProgress == 0) {
        return;
    }

    int loadProgress = 0;

    if (m_loadProgressState == LoadProgressState::DomContentLoaded) {
        loadProgress = 100;
        resetLoadProgress();
    } else {
        int remain = 100 - m_loadProgress;
        int delta = remain / m_pendingResourceCountWhileDocumentOpening;
        m_loadProgress += delta;
        if (m_loadProgressState == LoadProgressState::ParsingEnd) {
            m_loadProgress += delta;
        }
        if (m_loadProgress >= 100) {
            m_loadProgress = 99;
        }
        loadProgress = m_loadProgress;
    }
    starFish()->callWebViewHandler(std::string("OnProgressChanged"), nullptr,
                                   loadProgress);
}

void ResourceLoader::markDocumentOpenState()
{
    if (window()->browsingContext()->isTopLevelBrowsingContext()) {
        startLoadProgressTracking();
    }
    m_isDocumentInOpenState = true;
    m_documentOpenTime = timestamp();
    increasePendingResourceCountWhileDocumentOpening();
}

} // namespace StarFish
