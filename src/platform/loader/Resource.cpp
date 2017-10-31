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
#include "core/dom/HTMLFormElement.h"
#include "platform/loader/Resource.h"
#include "platform/loader/ResourceLoader.h"
#include "platform/loader/ResourceURL.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/resource_request/ResourceRequest.h"
#include "core/page/Window.h"
#include "platform/network/http/HTTPHeaderMap.h"

namespace StarFish {

void Resource::request(ResourceRequestSyncLevel syncLevel,
                       ResourceURL* referrerURL)
{
    if (!loader()->requestResourcePreprocess(this, syncLevel)) {
        // cache miss
        m_resourceRequest = new ResourceRequest(loader()->document());

        ResourceRequest::MethodType method = ResourceRequest::GET_METHOD;
        String* entityBody = String::emptyString;

        if (isImageResource()) {
            m_resourceRequest->setRequestHeader(
                String::createASCIIString(HTTPHeaderMap::kAccept),
                String::createASCIIString("image/*"));
        } else {
            // The current implementation has no difference between
            // text resource and default resource.
            m_resourceRequest->setRequestHeader(
                String::createASCIIString(HTTPHeaderMap::kAccept),
                String::createASCIIString("text/html,text/plain,text/*"));
        }

        String* urlToOpen = url()->urlString();
        if (m_url->isDocumentURL()) {
            DocumentURL* url = m_url->asDocumentURL();
            if (url->formSubmitData()) {
                FormSubmitData* formSubmitData = url->formSubmitData();
                if (url->isNetworkURL()) {
                    m_resourceRequest->setRequestHeader(
                        String::createASCIIString(HTTPHeaderMap::kAccept),
                        String::createASCIIString(
                            "text/html,application/xhtml+xml,application/"
                            "xml;q=0.9,image/webp,image/apng,*/*;q=0.8"));
                    method = formSubmitData->m_method;
                    m_resourceRequest->setRequestHeader(
                        String::createASCIIString(HTTPHeaderMap::kContentType),
                        ResourceRequest::encodeType(formSubmitData->m_enctype));
                    m_resourceRequest->setRequestHeader(
                        String::createASCIIString(
                            HTTPHeaderMap::kAcceptCharset),
                        String::createASCIIString("utf-8"));

                    m_resourceRequest->setRequestHeader(
                        String::createASCIIString(HTTPHeaderMap::kOrigin),
                        referrerURL->origin());

                    String* nocache = String::createASCIIString("no-cache");
                    m_resourceRequest->setRequestHeader(
                        String::createASCIIString(HTTPHeaderMap::kPragma),
                        nocache);
                    m_resourceRequest->setRequestHeader(
                        String::createASCIIString(HTTPHeaderMap::kCacheControl),
                        nocache);

                    if (formSubmitData->m_method ==
                        ResourceRequest::GET_METHOD) {
                        urlToOpen = m_resourceRequest->mutateActionURL(
                            url, formSubmitData);
                    } else if (formSubmitData->m_method ==
                               ResourceRequest::POST_METHOD) {
                        entityBody = m_resourceRequest->encodeFormDataSet(
                            formSubmitData->m_formDataSet,
                            formSubmitData->m_enctype);
                    }
                } else {
                    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
                }
            }
        }

        m_resourceRequest->addResourceRequestClient(
            new ResourceNetworkRequestClient(this));

        m_resourceRequest->open(
            method, urlToOpen,
            !(syncLevel == Resource::ResourceRequestSyncLevel::AlwaysSync),
            referrerURL);

        m_resourceRequest->send(entityBody);
    }
}

void Resource::cancel()
{
    STARFISH_ASSERT(m_state <= Receiving);

    if (m_isReferencedByAnoterResource) {
        m_isCanceledButContinueLoadingDueToCache = true;
    }

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
