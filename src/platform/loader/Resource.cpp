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

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLFormElement.h"
#include "platform/loader/Resource.h"
#include "platform/loader/ResourceLoader.h"
#include "platform/loader/ResourceURL.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/resource_request/ResourceRequest.h"
#include "core/page/WebView.h"
#include "core/page/Window.h"

namespace Starfish {

// TODO : replace ResourceURL* to ReferrerURL*
void Resource::request(RequestData* requestData, bool allowCache)
{
    STARFISH_ASSERT(m_state == BeforeSend);
    STARFISH_ASSERT(requestData->m_referrer);
    auto syncLevel = requestData->m_syncLevel;

    m_isRequested = true;
    if (!loader()->requestResourcePreprocess(this, syncLevel)) {
        // cache miss
        m_resourceRequest =
            new ResourceRequest(loader()->document()->executionContext());
        m_resourceRequest->open(requestData);
        String* entityBody = String::emptyString;

        ResourceURL* url = requestData->m_url;

        // FIXME : move to the suitable place according to the fetch spec.
        if (url->isDocumentURL()) {
            if (url->asDocumentURL()->formSubmitData()) {
                FormSubmitData* formSubmitData =
                    url->asDocumentURL()->formSubmitData();
                if (url->isHTTPFamilyURL()) {
                    m_resourceRequest->setRequestHeader(
                        String::createASCIIString(HTTPHeaderMap::kAccept),
                        String::createASCIIString(
                            "text/html,application/xhtml+xml,application/"
                            "xml;q=0.9,image/webp,image/apng,*/*;q=0.8"));
                    m_resourceRequest->m_requestData->m_method =
                        formSubmitData->m_method;
                    m_resourceRequest->setRequestHeader(
                        String::createASCIIString(HTTPHeaderMap::kContentType),
                        ResourceRequest::encodeType(formSubmitData->m_enctype));
                    m_resourceRequest->setRequestHeader(
                        String::createASCIIString(
                            HTTPHeaderMap::kAcceptCharset),
                        String::createASCIIString("utf-8"));

                    m_resourceRequest->setRequestHeader(
                        String::createASCIIString(HTTPHeaderMap::kOrigin),
                        m_resourceRequest->m_requestData->m_referrer->origin());

                    String* nocache = String::createASCIIString("no-cache");
                    m_resourceRequest->setRequestHeader(
                        String::createASCIIString(HTTPHeaderMap::kPragma),
                        nocache);
                    m_resourceRequest->setRequestHeader(
                        String::createASCIIString(HTTPHeaderMap::kCacheControl),
                        nocache);

                    if (formSubmitData->m_method->equals("GET")) {
                        url = m_resourceRequest->mutateActionURL(
                            url, formSubmitData);
                    } else if (formSubmitData->m_method->equals("POST")) {
                        entityBody = m_resourceRequest->encodeFormDataSet(
                            formSubmitData->m_formDataSet,
                            formSubmitData->m_enctype);
                    }
                } else {
                    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
                }
            }
            m_resourceRequest->m_requestData->m_url = url;
        }

        m_resourceRequest->addResourceRequestClient(
            new ResourceNetworkRequestClient(this));

        prepare();
        m_resourceRequest->send(entityBody, allowCache);
    }
    m_state = Receiving;
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
    const std::unordered_map<std::string, std::string>& headers)
{
    auto iter = m_resourceClients.begin();
    while (iter != m_resourceClients.end()) {
        (*iter)->didHeaderReceived(headers);
        iter++;
    }
}

void Resource::didDataReceived(const char* buf, size_t length)
{
    auto iter = m_resourceClients.begin();
    while (iter != m_resourceClients.end()) {
        (*iter)->didDataReceived(buf, length);
        iter++;
    }
}

void Resource::didLoadFinished()
{
    m_state = Finished;
    if (m_resourceRequest) {
        m_responseMimeType = m_resourceRequest->responseMimeType();
    }
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
        m_loader->webView()->messageLoop()->removeIdler(*iter2);
        iter2++;
    }
    m_requstedIdlers.clear();

    if (!m_isReferencedByAnoterResource && m_resourceRequest) {
        m_resourceRequest->abort(false);
        m_resourceRequest = nullptr;
    }
}
}
