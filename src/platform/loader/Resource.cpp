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
#include "StarFish.h"
#include "core/dom/Document.h"
#include "platform/loader/Resource.h"
#include "platform/loader/ResourceLoader.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/resource_request/ResourceRequest.h"
#include "core/page/Window.h"

namespace StarFish {

void Resource::request(ResourceRequestSyncLevel syncLevel)
{
    if (!loader()->requestResourcePreprocess(this, syncLevel)) {
        // cache miss
        m_resourceRequest = new ResourceRequest(loader()->document());

        if (isImageResource()) {
            m_resourceRequest->setRequestHeader(
                String::createASCIIString("Accept"),
                String::createASCIIString("image/*"));
        } else {
            // The current implementation has no difference between
            // text resource and default resource.
            m_resourceRequest->setRequestHeader(
                String::createASCIIString("Accept"),
                String::createASCIIString("text/html,text/plain,text/*"));
        }

        m_resourceRequest->addResourceRequestClient(
            new ResourceNetworkRequestClient(this));
        m_resourceRequest->open(
            ResourceRequest::GET_METHOD, url()->urlString(),
            !(syncLevel == Resource::ResourceRequestSyncLevel::AlwaysSync));
        m_resourceRequest->send();
    }
}

void Resource::cancel()
{
    if (m_state == BeforeSend || m_state == Receiving) {
        didLoadCanceled();
    }
}

void Resource::didHeaderReceived(
    const std::unordered_map<std::string, std::string>& headrs)
{
    auto iter = m_resourceClients.begin();
    while (iter != m_resourceClients.end()) {
        (*iter)->didHeaderReceived(headrs);
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
    m_resourceRequest = nullptr;
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
    m_resourceRequest = nullptr;
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
        m_loader->starFish()->messageLoop()->removeIdler(*iter2);
        iter2++;
    }
    m_requstedIdlers.clear();

    if (!m_isReferencedByAnoterResource && m_resourceRequest) {
        m_resourceRequest->abort(false);
        m_resourceRequest = nullptr;
    }
}
}
