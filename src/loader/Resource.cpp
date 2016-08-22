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
#include "Resource.h"

#include "platform/message_loop/MessageLoop.h"
#include "dom/Document.h"
#include "loader/ResourceLoader.h"
#include "platform/network/NetworkRequest.h"

namespace StarFish {

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

void Resource::request(ResourceRequestSyncLevel syncLevel)
{
    loader()->fetchResourcePreprocess(this);
    m_networkRequest = new NetworkRequest(m_loader->document());
    m_networkRequest->addNetworkRequestClient(new ResourceNetworkRequestClient(this));
    // TODO syncLevel == ResourceSyncLevel::SyncIfAlreadyLoaded
    m_networkRequest->open(NetworkRequest::GET_METHOD, m_url->urlString(), !(syncLevel == ResourceRequestSyncLevel::AlwaysSync));
    m_networkRequest->send();
}

void Resource::cancel()
{
    STARFISH_ASSERT(m_state == BeforeSend || m_state == Receiving);
    didLoadCanceled();
}

void Resource::didHeaderReceived(String* header)
{
    auto iter = m_resourceClients.begin();
    while (iter != m_resourceClients.end()) {
        (*iter)->didHeaderReceived(header);
        iter++;
    }
}

void Resource::didDataReceived(const char* buf, size_t length)
{
    m_state = Receiving;
    auto iter = m_resourceClients.begin();
    while (iter != m_resourceClients.end()) {
        (*iter)->didDataReceived(buf, length);
        iter++;
    }
}

void Resource::didLoadFinished()
{
    m_state = Finished;
    auto iter = m_resourceClients.begin();
    while (iter != m_resourceClients.end()) {
        (*iter)->didLoadFinished();
        iter++;
    }
    m_networkRequest = nullptr;
}

void Resource::didLoadFailed()
{
    m_state = Failed;
    auto iter = m_resourceClients.begin();
    while (iter != m_resourceClients.end()) {
        (*iter)->didLoadFailed();
        iter++;
    }
    m_networkRequest = nullptr;
}

void Resource::didLoadCanceled()
{
    m_state = Canceled;
    auto iter = m_resourceClients.begin();
    while (iter != m_resourceClients.end()) {
        (*iter)->didLoadCanceled();
        iter++;
    }
    auto iter2 = m_requstedIdlers.begin();
    while (iter2 != m_requstedIdlers.end()) {
        m_loader->m_document->window()->starFish()->messageLoop()->removeIdler(*iter2);
        iter2++;
    }
    m_requstedIdlers.clear();
    m_networkRequest->abort(false);
    m_networkRequest = nullptr;
}

}
