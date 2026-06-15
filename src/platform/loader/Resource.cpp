/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
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
#include "platform/loader/Resource.h"
#include "platform/loader/ResourceLoader.h"
#include "platform/loader/ResourceURL.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/resource_request/ResourceRequest.h"
#include "core/page/WebView.h"
#include "core/xml/FormData.h"
#if defined(STARFISH_ENABLE_CDP)
#include "core/cdp/CDPServer.h"
#include "core/cdp/CDPDispatcher.h"
#include "core/cdp/domains/NetworkDomain.h"
#endif

namespace Starfish {

#if defined(STARFISH_ENABLE_CDP)
// Resolve the live NetworkDomain for this resource's WebView, if CDP is up.
static NetworkDomain* cdpNetwork(Resource* res)
{
    WebView* wv = res->loader() && res->loader()->document()
                      ? res->loader()->document()->webView()
                      : nullptr;
    if (!wv || !wv->cdpServer() || !wv->cdpServer()->dispatcher()) {
        return nullptr;
    }
    return wv->cdpServer()->dispatcher()->network();
}

static WebView* cdpWebView(Resource* res)
{
    return res->loader() && res->loader()->document()
               ? res->loader()->document()->webView()
               : nullptr;
}
#endif

// TODO : replace ResourceURL* to ReferrerURL*
void Resource::request(RequestData* requestData, bool allowCache)
{
    STARFISH_ASSERT(m_state == BeforeSend);
    STARFISH_ASSERT(requestData->m_referrer);
    auto syncLevel = requestData->m_syncLevel;

    m_isRequested = true;
    m_requestErrorType = RequestErrorType::NoError;

    if (!loader()->requestResourcePreprocess(this, syncLevel)) {
        // cache miss
        m_resourceRequest =
            new ResourceRequest(loader()->document()->executionContext());
        m_resourceRequest->open(requestData, new HeadersData());
        String* entityBody = String::emptyString;

        ResourceURL* url = requestData->m_url;

        // FIXME : move to the suitable place according to the fetch spec.
        if (url->isDocumentURL()) {
            if (url->asDocumentURL()->formSubmitData()) {
                FormSubmitData* formSubmitData =
                    url->asDocumentURL()->formSubmitData();
                if (url->isHTTPFamilyURL() == true) {
                    m_resourceRequest->setRequestHeader(
                        String::createASCIIString(
                            HTTPHeaderMap::kAccept,
                            strlen(HTTPHeaderMap::kAccept)),
                        String::createASCIIString(
                            "text/html,application/xhtml+xml,application/"
                            "xml;q=0.9,image/webp,image/apng,*/*;q=0.8"));
                    m_resourceRequest->m_requestData->m_method =
                        formSubmitData->m_method;
                    m_resourceRequest->setRequestHeader(
                        String::createASCIIString(
                            HTTPHeaderMap::kContentType,
                            strlen(HTTPHeaderMap::kContentType)),
                        ResourceRequest::encodeType(formSubmitData->m_enctype));
                    m_resourceRequest->setRequestHeader(
                        String::createASCIIString(
                            HTTPHeaderMap::kAcceptCharset,
                            strlen(HTTPHeaderMap::kAcceptCharset)),
                        String::createASCIIString("utf-8"));

                    m_resourceRequest->setRequestHeader(
                        String::createASCIIString(
                            HTTPHeaderMap::kOrigin,
                            strlen(HTTPHeaderMap::kOrigin)),
                        m_resourceRequest->m_requestData->m_referrer->origin());

                    String* nocache = String::createASCIIString("no-cache");
                    m_resourceRequest->setRequestHeader(
                        String::createASCIIString(
                            HTTPHeaderMap::kPragma,
                            strlen(HTTPHeaderMap::kPragma)),
                        nocache);
                    m_resourceRequest->setRequestHeader(
                        String::createASCIIString(
                            HTTPHeaderMap::kCacheControl,
                            strlen(HTTPHeaderMap::kCacheControl)),
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
                    STARFISH_UNSUPPORTED("Resource: unsupported url type");
                }
            }
            m_resourceRequest->m_requestData->m_url = url;
        }

        m_resourceRequest->addResourceRequestClient(
            new ResourceNetworkRequestClient(this));

        prepare();
#if defined(STARFISH_ENABLE_CDP)
        // CDP Network hook: inject setExtraHTTPHeaders, then emit a real
        // Network.requestWillBeSent for this outgoing request (document or
        // subresource). Captured requestId is reused for the later phases.
        if (NetworkDomain* net = cdpNetwork(this)) {
            net->applyExtraHTTPHeaders(cdpWebView(this), m_resourceRequest);
            // entityBody carries the encoded POST body for form submits (empty
            // otherwise); pass it so Network.getRequestPostData can serve it.
            std::string cdpPostData;
            if (entityBody && entityBody->length() > 0) {
                cdpPostData = entityBody->toUTF8NonGCString();
            }
            m_cdpRequestId =
                net->onResourceWillBeSent(cdpWebView(this), this, cdpPostData);

            // Network.emulateNetworkConditions(offline) / setBlockedURLs: fail
            // a real http(s) request instead of sending it. Emit the blocked
            // loadingFailed with the proper net:: error, then drive the request
            // into its error state -- ResourceNetworkRequestClient turns that
            // into didLoadFailed (document => page.goto rejects; subresource =>
            // only that resource fails). Clear the captured id first so the
            // generic ERR_FAILED loadingFailed (cdpNotifyNetworkFailed) is not
            // also emitted for the same request.
            std::string blockError;
            if (net->shouldBlockRequest(cdpWebView(this), this, blockError)) {
                net->emitLoadingFailed(cdpWebView(this), m_cdpRequestId,
                                       blockError);
                m_cdpRequestId.clear();
                m_resourceRequest->handleError(ProgressState::InError,
                                               RequestErrorType::ConnectError);
                m_state = Receiving;
                return;
            }
        }
#endif
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
#if defined(STARFISH_ENABLE_CDP)
    if (!m_cdpRequestId.empty() && !m_cdpResponseEmitted && m_resourceRequest) {
        if (NetworkDomain* net = cdpNetwork(this)) {
            net->onResourceResponse(cdpWebView(this), m_cdpRequestId, this,
                                    m_resourceRequest);
            m_cdpResponseEmitted = true;
        }
    }
#endif
    auto iter = m_resourceClients.begin();
    while (iter != m_resourceClients.end()) {
        (*iter)->didHeaderReceived(headers);
        iter++;
    }
}

void Resource::didDataReceived(const char* buf, size_t length)
{
#if defined(STARFISH_ENABLE_CDP)
    if (!m_cdpRequestId.empty()) {
        if (NetworkDomain* net = cdpNetwork(this)) {
            net->onResourceData(cdpWebView(this), m_cdpRequestId, buf, length);
        }
    }
#endif
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
        m_requestErrorType = m_resourceRequest->errorType();
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
    if (m_resourceRequest) {
        m_requestErrorType = m_resourceRequest->errorType();
    }
    auto iter = m_resourceClients.begin();
    while (iter != m_resourceClients.end()) {
        (*iter)->didLoadFailed();
        iter++;
    }
    m_resourceRequest = nullptr;
    m_resourceClients.clear();
}

#if defined(STARFISH_ENABLE_CDP)
void Resource::cdpNotifyNetworkFinished()
{
    if (m_cdpRequestId.empty()) {
        return;
    }
    if (NetworkDomain* net = cdpNetwork(this)) {
        // Emit the response first if a distinct header callback never fired.
        if (!m_cdpResponseEmitted && m_resourceRequest) {
            net->onResourceResponse(cdpWebView(this), m_cdpRequestId, this,
                                    m_resourceRequest);
            m_cdpResponseEmitted = true;
        }
        net->onResourceFinished(cdpWebView(this), m_cdpRequestId);
    }
    m_cdpRequestId.clear();
}

void Resource::cdpNotifyNetworkFailed()
{
    if (m_cdpRequestId.empty()) {
        return;
    }
    if (NetworkDomain* net = cdpNetwork(this)) {
        net->onResourceFailed(cdpWebView(this), m_cdpRequestId);
    }
    m_cdpRequestId.clear();
}
#endif

void Resource::didLoadCanceled()
{
    if (m_isReferencedByAnoterResource) {
        m_isCanceledButContinueLoadingDueToCache = true;
    }
#if defined(STARFISH_ENABLE_CDP)
    // A request canceled before completion: report it as failed (canceled) and
    // drop the captured id so no stale finished event follows.
    if (!m_cdpRequestId.empty()) {
        if (NetworkDomain* net = cdpNetwork(this)) {
            net->onResourceFailed(cdpWebView(this), m_cdpRequestId);
        }
        m_cdpRequestId.clear();
    }
#endif

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
} // namespace Starfish
