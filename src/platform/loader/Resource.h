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

#ifndef __StarFishResource__
#define __StarFishResource__

#include "core/util/URL.h"
#include "platform/loader/ResourceClient.h"
#include "core/modules/resource_request/ResourceRequest.h"
#include "platform/network/http/HTTPHeaderMap.h"

namespace StarFish {

class TextResource;
class ImageResource;
class FontResource;
class ResourceLoader;
class ResourceRequest;

class Resource : public gc {
    friend class ResourceLoader;
    friend class ResourceWatcher;
    friend class ResourceLoaderTracer;

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
        FontResourceType,
    };

    Resource(ResourceURL* url, ResourceLoader* loader)
        : m_isIncludedInComputingWindowOnLoadEvent(true)
        , m_isReferencedByAnoterResource(false)
        , m_isCanceledButContinueLoadingDueToCache(false)
        , m_isRequested(false)
        , m_state(BeforeSend)
        , m_url(url)
        , m_loader(loader)
        , m_resourceRequest(nullptr)
        , m_responseMimeType(String::emptyString)
    {
    }

    virtual ~Resource()
    {
    }

    virtual void prepare()
    {
        if (m_resourceRequest) {
            m_resourceRequest->setRequestHeader(
                String::createASCIIString(HTTPHeaderMap::kAccept),
                String::createASCIIString("*/*"));
        }
    }

    virtual bool isTextResource()
    {
        return false;
    }

    virtual bool isImageResource()
    {
        return false;
    }

    virtual bool isFontResource()
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

    FontResource* asFontResource()
    {
        STARFISH_ASSERT(isFontResource());
        return (FontResource*)this;
    }

    void addResourceClient(ResourceClient* rc)
    {
        m_resourceClients.push_back(rc);
    }

    void addResourceClient(const GCVector<ResourceClient*>& rc)
    {
        m_resourceClients.insert(m_resourceClients.end(), rc.begin(), rc.end());
    }

    void removeResourceClient(ResourceClient* rc)
    {
        m_resourceClients.erase(
            std::find(m_resourceClients.begin(), m_resourceClients.end(), rc));
    }

    ResourceRequest* resourceRequest()
    {
        return m_resourceRequest;
    }

    ResourceURL* url()
    {
        return m_url;
    }

    String* responseMimeType()
    {
        return m_responseMimeType;
    }

    enum ResourceRequestSyncLevel {
        NeverSync,
        SyncIfAlreadyLoaded,
        AlwaysSync
    };
    virtual void request(ResourceRequestSyncLevel syncLevel,
                         ResourceURL* referrerURL, bool allowCache = false);
    virtual void cancel();
    virtual void didHeaderReceived(
        const std::unordered_map<std::string, std::string>& headrs);
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
        STARFISH_ASSERT(std::find(m_requstedIdlers.begin(),
                                  m_requstedIdlers.end(),
                                  handle) != m_requstedIdlers.end());
        m_requstedIdlers.erase(std::find(m_requstedIdlers.begin(),
                                         m_requstedIdlers.end(), handle));
    }

    void markThisResourceIsDoesNotAffectWindowOnLoad()
    {
        m_isIncludedInComputingWindowOnLoadEvent = false;
    }

    bool isThisResourceDoesAffectWindowOnLoad()
    {
        return m_isIncludedInComputingWindowOnLoadEvent;
    }

    bool isFaildToFetchResource()
    {
        return m_state == State::Failed;
    }

    bool isRequested()
    {
        return m_isRequested;
    }

    bool isFinished()
    {
        return m_state == State::Finished;
    }

    bool isReceiving()
    {
        return m_state == State::Receiving;
    }

    State state()
    {
        return m_state;
    }

protected:
    bool m_isIncludedInComputingWindowOnLoadEvent : 1;
    bool m_isReferencedByAnoterResource : 1;
    bool m_isCanceledButContinueLoadingDueToCache : 1;
    bool m_isRequested : 1;
    State m_state;
    ResourceURL* m_url;
    ResourceLoader* m_loader;
    ResourceRequest* m_resourceRequest;
    String* m_responseMimeType;
    GCVector<ResourceClient*> m_resourceClients;
    GCVector<size_t> m_requstedIdlers;
};

class ResourceNetworkRequestClient : public ResourceRequestClient {
public:
    ResourceNetworkRequestClient(Resource* resource)
        : m_resource(resource)
    {
    }

    virtual void onReadyStateChange(ResourceRequest* request,
                                    bool isExplicitAction) override
    {
        if (request->readyState() == ResourceRequest::HEADERS_RECEIVED) {
            m_resource->didHeaderReceived(request->responseHeaderMap());
        }
    }

    virtual void onProgressEvent(ResourceRequest* request,
                                 bool isExplicitAction) override
    {
        if (request->progressState() == ResourceRequest::LOAD) {
            m_resource->didDataReceived(request->response().data(),
                                        request->response().size());
            m_resource->didLoadFinished();
        } else if (request->progressState() == ResourceRequest::IN_ERROR) {
            m_resource->didLoadFailed();
        } else if (request->progressState() == ResourceRequest::TIMEOUT) {
            m_resource->didLoadFailed();
        }
    }

protected:
    Resource* m_resource;
};
}

#endif
