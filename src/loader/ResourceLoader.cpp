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

#include "StarFishConfig.h"
#include "dom/Document.h"
#include "ResourceLoader.h"

#include "platform/network/NetworkRequest.h"
#include "platform/message_loop/MessageLoop.h"
#include "platform/profiling/Profiling.h"

#ifdef STARFISH_ENABLE_TEST
extern bool g_fireOnloadEvent;
#endif

#ifndef STARFISH_RESOURCE_CACHE_SIZE
#define STARFISH_RESOURCE_CACHE_SIZE 1024 * 2
#endif

namespace StarFish {

ResourceLoader::ResourceLoader(Document& document)
    : m_inDocumentOpenState(false)
    , m_pendingResourceCountWhileDocumentOpening(0)
    , m_document(&document)
    , m_resourceCacheSize(0)
{
}

Resource* ResourceLoader::fetch(URL* url)
{
    Resource* res = new Resource(url, this);
    return res;
}

TextResource* ResourceLoader::fetchText(URL* url, String* preferredEncoding)
{
    TextResource* res = new TextResource(url, this, preferredEncoding);
    return res;
}

ImageResource* ResourceLoader::fetchImage(URL* url)
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
        if (m_didFire)
            return;
        m_didFire = true;
        STARFISH_ASSERT(m_resource->loader()->m_pendingResourceCountWhileDocumentOpening > 0);
        m_resource->loader()->m_pendingResourceCountWhileDocumentOpening--;
        m_resource->loader()->fireDocumentOnLoadEventIfNeeded();
    }

    bool m_didFire;
};

class ResourceAliveChecker : public ResourceClient {
public:
    ResourceAliveChecker(Resource* res)
        : ResourceClient(res)
    {
        std::vector<Resource*, gc_allocator<Resource*>>& v =  m_resource->loader()->m_currentLoadingResources;
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
        std::vector<Resource*, gc_allocator<Resource*>>& v =  m_resource->loader()->m_currentLoadingResources;
        auto iter = std::find(v.begin(), v.end(), m_resource);
        // TODO prevent remove twice
        if (iter != v.end()) {
            v.erase(iter);
        }
    }
};

class ResourceSizeTracer : public ResourceClient {
public:
    ResourceSizeTracer(Resource* res)
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
        m_resource->loader()->cachePruning();
    }

    virtual void didLoadCanceled()
    {
        ResourceClient::didLoadCanceled();
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
        m_resource->addResourceClient(new ResourceSizeTracer(m_resource));
    }

    Resource* m_watcher;
};

void ResourceLoader::cachePruning()
{
    STARFISH_LOG_INFO("ResourceLoader - CacheSize %dKB\n", (int)m_resourceCacheSize / 1024);
}

bool ResourceLoader::requestResourcePreprocess(Resource* res, Resource::ResourceRequestSyncLevel syncLevel)
{
    if (m_inDocumentOpenState && res->isThisResourceDoesAffectWindowOnLoad()) {
        m_pendingResourceCountWhileDocumentOpening++;
        res->addResourceClient(new DocumentOnLoadChecker(res));
        res->addResourceClient(new ResourceAliveChecker(res));
    }

    // TODO cache every resource
    if (res->isImageResource() && syncLevel != Resource::ResourceRequestSyncLevel::AlwaysSync) {
        std::string url = res->url()->urlString()->utf8Data();
        auto iter = m_resourceCache.find(url);
        if (iter == m_resourceCache.end()) {
            ResourceCacheData* data = new ResourceCacheData;
            data->m_lastUsedTime = 0;
            data->m_data.push_back(std::make_pair(res, res->type()));
            m_resourceCache.insert(std::make_pair(std::move(url), data));
        } else {
            ResourceCacheData* data = iter->second;
            Resource* resourceInCache = nullptr;
            size_t dataSize = data->m_data.size();
            Resource::Type resourceType = res->type();

            for (size_t i = 0; i < dataSize; i ++) {
                if (data->m_data[i].second == resourceType) {
                    resourceInCache = data->m_data[i].first;
                    break;
                }
            }

            if (resourceInCache == nullptr) {
                data->m_data.push_back(std::make_pair(res, resourceType));
            } else {
                data->m_lastUsedTime = tickCount();
                cacheHit(resourceInCache, res, syncLevel);
                return true;
            }
        }

        STARFISH_ASSERT(res->state() == Resource::BeforeSend);
        res->addResourceClient(new ResourceSizeTracer(res));
    }

    return false;
}


void ResourceLoader::cacheHit(Resource* org, Resource* now, Resource::ResourceRequestSyncLevel syncLevel)
{
    Resource::State s = org->state();
    org->m_isCached = true;
    // STARFISH_LOG_INFO("cache hit! %s\n", org->url()->urlString()->utf8Data());
    if (s == Resource::State::Finished) {
        if (syncLevel == Resource::ResourceRequestSyncLevel::SyncIfAlreadyLoaded) {
            now->didCacheHit(org);
        } else {
            STARFISH_ASSERT(syncLevel == Resource::ResourceRequestSyncLevel::NeverSync);
            document()->window()->starFish()->messageLoop()->addIdler([](size_t, void* data, void* data2) {
                Resource* org = (Resource*)data;
                Resource* now = (Resource*)data2;
                now->didCacheHit(org);
            }, org, now);
        }
    } else if (s == Resource::State::Failed) {
        document()->window()->starFish()->messageLoop()->addIdler([](size_t, void* data) {
            Resource* now = (Resource*)data;
            now->didLoadFailed();
        }, now);
    } else {
        org->addResourceClient(new ResourceWatcher(org, now));
    }
}

void ResourceLoader::fireDocumentOnLoadEventIfNeeded()
{
    if (m_pendingResourceCountWhileDocumentOpening == 0 && m_inDocumentOpenState) {
        m_inDocumentOpenState = false;
        m_document->window()->starFish()->messageLoop()->addIdler([](size_t handle, void* data) {
            Window* wnd = (Window*)data;
            String* eventType = wnd->starFish()->staticStrings()->m_load.localName();
            Event* e = new Event(eventType, EventInit(false, false));
            wnd->EventTarget::dispatchEvent(e);
#ifdef STARFISH_ENABLE_TEST
            g_fireOnloadEvent = true;
            wnd->setNeedsPainting();
            wnd->testStart();
#endif
        }, m_document->window());
    }
}

void ResourceLoader::cancelAllOfPendingRequests()
{
    std::vector<Resource*, gc_allocator<Resource*>>& v = m_currentLoadingResources;
    while (v.size()) {
        v[0]->cancel();
    }
}

}
