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

void Resource::request(ResourceRequestSyncLevel syncLevel)
{
    if (!loader()->requestResourcePreprocess(this, syncLevel)) {
        // cache miss
        m_networkRequest = new NetworkRequest(loader()->document());
        m_networkRequest->addNetworkRequestClient(new ResourceNetworkRequestClient(this));
        m_networkRequest->open(NetworkRequest::GET_METHOD, url()->urlString(), !(syncLevel == Resource::ResourceRequestSyncLevel::AlwaysSync));
        m_networkRequest->send();
    }
}

void Resource::cancel()
{
    if (m_state == BeforeSend || m_state == Receiving)
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
    m_resourceClients.clear();
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
    m_resourceClients.clear();
}

void Resource::didLoadCanceled()
{
    if (m_isReferencedByAnoterResource) {
        m_isCanceledButContinueLoadingDueToCache = true;
    }

    m_state = Canceled;
    auto b = std::move(m_resourceClients);
    auto iter = b.begin();
    while (iter != b.end()) {
        (*iter)->didLoadCanceled();
        iter++;
    }
    auto iter2 = m_requstedIdlers.begin();
    while (iter2 != m_requstedIdlers.end()) {
        m_loader->m_document->window()->starFish()->messageLoop()->removeIdler(*iter2);
        iter2++;
    }
    m_requstedIdlers.clear();

    if (!m_isReferencedByAnoterResource && m_networkRequest) {
        m_networkRequest->abort(false);
        m_networkRequest = nullptr;
    }
}

}
