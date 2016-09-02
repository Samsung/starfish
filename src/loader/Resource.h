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

#ifndef __StarFishResource__
#define __StarFishResource__

#include "loader/Resource.h"
#include "loader/ResourceClient.h"
#include "platform/network/NetworkRequest.h"

namespace StarFish {

class TextResource;
class ImageResource;
class ResourceLoader;
class NetworkRequest;

class Resource : public gc {
    friend class ResourceLoader;
    friend class ResourceWatcher;
public:
    enum State {
        BeforeSend,
        Receiving,
        Finished,
        Failed,
        Canceled,
    };

    enum Type {
        ResourceType,
        ImageResourceType,
        TextResourceType,
    };

    Resource(URL* url, ResourceLoader* loader)
        : m_isIncludedInComputingWindowOnLoadEvent(true)
        , m_isCached(false)
        , m_isCanceledButContinueLoadingDueToCache(false)
        , m_state(BeforeSend)
        , m_url(url)
        , m_loader(loader)
        , m_networkRequest(nullptr)
    {
    }

    virtual ~Resource()
    {

    }

    virtual bool isTextResource()
    {
        return false;
    }

    virtual bool isImageResource()
    {
        return false;
    }

    TextResource* asTextResource()
    {
        STARFISH_ASSERT(isTextResource());
        return (TextResource*)this;
    }

    ImageResource* asImageResource()
    {
        STARFISH_ASSERT(isImageResource());
        return (ImageResource*)this;
    }

    void addResourceClient(ResourceClient* rc)
    {
        m_resourceClients.push_back(rc);
    }

    void addResourceClient(const ResourceClientVector& rc)
    {
        m_resourceClients.insert(m_resourceClients.end(), rc.begin(), rc.end());
    }

    void removeResourceClient(ResourceClient* rc)
    {
        m_resourceClients.erase(std::find(m_resourceClients.begin(), m_resourceClients.end(), rc));
    }

    NetworkRequest* networkRequest()
    {
        return m_networkRequest;
    }

    URL* url()
    {
        return m_url;
    }

    enum ResourceRequestSyncLevel {
        NeverSync,
        SyncIfAlreadyLoaded,
        AlwaysSync
    };
    virtual void request(ResourceRequestSyncLevel syncLevel = NeverSync);
    virtual void cancel();
    virtual void didHeaderReceived(String* header);
    virtual void didDataReceived(const char*, size_t length);
    virtual void didLoadFinished();
    virtual void didLoadFailed();
    virtual void didLoadCanceled();
    virtual void didCacheHit(Resource* cache)
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    virtual size_t contentSize()
    {
        return 0;
    }

    virtual Type type()
    {
        return Type::ResourceType;
    }

    ResourceLoader* loader()
    {
        return m_loader;
    }

    void pushIdlerHandle(size_t handle)
    {
        m_requstedIdlers.push_back(handle);
    }

    void removeIdlerHandle(size_t handle)
    {
        STARFISH_ASSERT(m_requstedIdlers.size());
        STARFISH_ASSERT(std::find(m_requstedIdlers.begin(), m_requstedIdlers.end(), handle) != m_requstedIdlers.end());
        m_requstedIdlers.erase(std::find(m_requstedIdlers.begin(), m_requstedIdlers.end(), handle));
    }

    void markThisResourceIsDoesNotAffectWindowOnLoad()
    {
        m_isIncludedInComputingWindowOnLoadEvent = false;
    }

    bool isThisResourceDoesAffectWindowOnLoad()
    {
        return m_isIncludedInComputingWindowOnLoadEvent;
    }

protected:
    State state()
    {
        return m_state;
    }

    bool m_isIncludedInComputingWindowOnLoadEvent : 1;
    bool m_isCached : 1;
    bool m_isCanceledButContinueLoadingDueToCache : 1;
    State m_state;
    URL* m_url;
    ResourceLoader* m_loader;
    NetworkRequest* m_networkRequest;
    ResourceClientVector m_resourceClients;
    std::vector<size_t, gc_allocator<size_t>> m_requstedIdlers;
};

class ResourceNetworkRequestClient : public NetworkRequestClient {
public:
    ResourceNetworkRequestClient(Resource* resource)
        : m_resource(resource)
    {
    }

    virtual void onReadyStateChange(NetworkRequest* request, bool isExplicitAction)
    {
        if (request->readyState() == NetworkRequest::HEADERS_RECEIVED) {
            m_resource->didHeaderReceived(String::fromUTF8(request->responseHeaderData().data(), request->responseHeaderData().length()));
        }
    }

    virtual void onProgressEvent(NetworkRequest* request, bool isExplicitAction)
    {
        if (request->progressState() == NetworkRequest::LOAD) {
            m_resource->didDataReceived(request->responseData().data(), request->responseData().size());
            m_resource->didLoadFinished();
        } else if (request->progressState() == NetworkRequest::ERROR) {
            m_resource->didLoadFailed();
        } else if (request->progressState() == NetworkRequest::TIMEOUT) {
            m_resource->didLoadFailed();
        }
    }

protected:
    Resource* m_resource;
};

}

#endif
