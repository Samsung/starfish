/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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
#include "platform/network/http/HTTPHeaderMap.h"
#include "platform/network/http/HTTPRequest.h"
#include "platform/network/http/HTTPResponse.h"
#include "platform/network/http/HTTPTransaction.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/resource_request/NetworkURLResourceRequestJobDelegate.h"
#include "platform/network/NetworkSharedResourceManager.h"
#include "core/modules/resource_request/ResourceRequest.h"
#include "core/modules/threading/ThreadPool.h"
#include "core/modules/message_loop/Timer.h"

#ifndef STARFISH_CURL_HANDLE_CACHE_CLEAR_TIMEOUT_IN_MS
#define STARFISH_CURL_HANDLE_CACHE_CLEAR_TIMEOUT_IN_MS 5000
#endif

namespace StarFish {

void* NetworkURLWorkerHelper::networkWorker(void* data)
{
    NetworkURLWorkerData* requestData = (NetworkURLWorkerData*)data;
    requestData->httpTransaction->start();

    // TODO : Do not use libur libcurl error codes
    if (requestData->httpTransaction->res() != CURLE_ABORTED_BY_CALLBACK) {
        responseHandlerWrapper(requestData->httpTransaction->res(),
                               requestData);
    } else {
        requestData->request->starFish()
            ->messageLoop()
            ->addIdlerWithNoGCRootingInOtherThread(
                requestData->request->document()->browsingContext(),
                [](size_t, void* data) {
                    NetworkURLWorkerData* requestData =
                        (NetworkURLWorkerData*)data;
                    if (requestData ==
                        requestData->request->m_activeNetworkURLWorkerData) {
                        requestData->request->m_activeNetworkURLWorkerData =
                            nullptr;
                    }
                    requestData->~NetworkURLWorkerData();
                    GC_FREE(requestData);
                },
                requestData);
    }

    return NULL;
}

void NetworkURLWorkerHelper::responseHandler(size_t handle, void* data)
{
    NetworkURLWorkerData* requestData = (NetworkURLWorkerData*)data;
    STARFISH_ASSERT(isMainThread());
    // TODO : Do not use libur libcurl error codes
    STARFISH_ASSERT(requestData->httpTransaction->res() !=
                    CURLE_ABORTED_BY_CALLBACK);

    if (requestData->isAborted) {
    } else if (requestData->httpTransaction->res() == 0) {
        if (!requestData->request->isSync()) {
            Locker<Mutex> locker(*requestData->request->m_mutex);
            requestData->request->m_pendingNetworkWorkerEndIdlerHandle =
                SIZE_MAX;
        }
        STARFISH_ASSERT(
            requestData->request->m_pendingNetworkWorkerEndIdlerHandle ==
            SIZE_MAX);

        if (requestData->isRedirected) {
            requestData->request->m_lastLocation =
                String::createASCIIString(
                    requestData->httpTransaction->httpResponse()
                        .lastEffectiveURL()
                        .data())
                    ->trim();
        }

        requestData->request->handleResponseEOF();
    } else if (requestData->httpTransaction->res() ==
               CURLE_OPERATION_TIMEDOUT) {
        if (!requestData->request->isSync()) {
            Locker<Mutex> locker(*requestData->request->m_mutex);
            requestData->request->m_pendingNetworkWorkerEndIdlerHandle =
                SIZE_MAX;
        }
        STARFISH_ASSERT(
            requestData->request->m_pendingNetworkWorkerEndIdlerHandle ==
            SIZE_MAX);
        auto s = requestData->request->m_url->urlString()->toUTF8NonGCString();
        STARFISH_LOG_INFO(
            "got timeout %s[%d]\n", s.data(),
            (int)requestData->httpTransaction->httpResponse().responseCode());
        requestData->request->handleError(ResourceRequest::TIMEOUT);
    } else {
        if (!requestData->request->isSync()) {
            Locker<Mutex> locker(*requestData->request->m_mutex);
            requestData->request->m_pendingNetworkWorkerEndIdlerHandle =
                SIZE_MAX;
        }
        STARFISH_ASSERT(
            requestData->request->m_pendingNetworkWorkerEndIdlerHandle ==
            SIZE_MAX);
        auto s = requestData->request->m_url->urlString()->toUTF8NonGCString();
        STARFISH_LOG_INFO("failed to open %s\n", s.data());
        requestData->request->handleError(ResourceRequest::ERROR);
    }

    if (NetworkSharedResourceManager::getInstance()->cacheClearTimerID() !=
        SIZE_MAX) {
        requestData->request->starFish()->timer()->removeTimer(
            NetworkSharedResourceManager::getInstance()->cacheClearTimerID());
    }

    size_t timerID = requestData->request->starFish()->timer()->addTimer(
        STARFISH_CURL_HANDLE_CACHE_CLEAR_TIMEOUT_IN_MS,
        requestData->request->document()->window(),
        [](Window* wnd, void* data) {
            NetworkSharedResourceManager::getInstance()
                ->clearAllCurlHandleDataCache();
            NetworkSharedResourceManager::getInstance()->setCacheClearTimerID(
                SIZE_MAX);
        },
        nullptr, false);

    NetworkSharedResourceManager::getInstance()->setCacheClearTimerID(timerID);

    requestData->request->m_activeNetworkURLWorkerData = nullptr;
    requestData->~NetworkURLWorkerData();
    GC_FREE(requestData);
}

void SyncNetworkWorkHelper::responseHandlerWrapper(
    int res, NetworkURLWorkerData* requestData)
{
    responseHandler(res, requestData);
}

void AsyncNetworkWorkHelper::responseHandlerWrapper(
    int res, NetworkURLWorkerData* requestData)
{
    Locker<Mutex> locker(*requestData->request->m_mutex);
    requestData->request->m_pendingNetworkWorkerEndIdlerHandle =
        requestData->request->starFish()
            ->messageLoop()
            ->addIdlerWithNoGCRootingInOtherThread(
                requestData->request->document()->browsingContext(),
                this->responseHandler, requestData);
}

NetworkURLResourceRequestJobDelegate::NetworkURLResourceRequestJobDelegate(
    ResourceRequest* proxy)
    : m_orgProxy(proxy)
{
}

void NetworkURLResourceRequestJobDelegate::send(String* body)
{
    STARFISH_ASSERT(isMainThread());
    STARFISH_ASSERT(m_orgProxy->m_url->isNetworkURL());

    NetworkURLWorkerData* data = new (NoGC) NetworkURLWorkerData();
    data->request = m_orgProxy;
    data->isAborted = false;
    data->isRedirected = false;
    data->lastLocation = "";
    m_orgProxy->m_activeNetworkURLWorkerData = data;

    std::string method;
    switch (m_orgProxy->m_method) {
    case ResourceRequest::GET_METHOD: {
        method = "GET";
        break;
    }
    case ResourceRequest::POST_METHOD: {
        method = "POST";
        break;
    }
    case ResourceRequest::UNKNOWN_METHOD: {
        STARFISH_ASSERT_NOT_REACHED();
        break;
    }
    default:
        STARFISH_ASSERT_NOT_REACHED();
    }

    data->httpTransaction = HTTPTransaction::create();

    HTTPHeaderMap headers;
    fillHeadersWithGeneralHeaders(headers);
    fillHeadersWithClientHeaders(headers);
    fillHeadersWithResourceRequestHeader(headers);

    auto urlUTF8Data = m_orgProxy->m_url->urlString()->toUTF8NonGCString();
    auto hostUTF8Data = m_orgProxy->m_url->host()->toUTF8NonGCString();
    auto bodyUTF8Data = body->toUTF8NonGCString();
    data->httpTransaction->setHTTPRequest(HTTPRequest::create(
        urlUTF8Data, hostUTF8Data, method, headers, bodyUTF8Data));
    data->httpTransaction->setTimeout(
        static_cast<unsigned long>(m_orgProxy->m_timeout));

    data->httpTransaction->setProgressCallbackAndData(curlProgressCallback,
                                                      data);
    data->httpTransaction->setWriteHeaderCallbackAndData(
        curlWriteHeaderCallback, data);
    data->httpTransaction->setWriteCallbackAndData(curlWriteCallback, data);

    if (m_orgProxy->isSync()) {
        data->networkWorker = new SyncNetworkWorkHelper();
        worker(data);
    } else {
        auto& header = headers.headerMap();
        auto pos = std::find_if(
            header.begin(), header.end(),
            [](const std::unordered_map<std::string, std::string>::value_type&
                   o) { return o.second == "text/event-stream"; });

        if (pos != header.end()) {
            data->networkWorker = new AsyncNetworkWorkHelper();
            Thread* t = new Thread(m_orgProxy->starFish());
            t->run(m_orgProxy->starFish()->messageLoop(),
                   [](void* data) -> void* {
                       NetworkURLWorkerData* d = (NetworkURLWorkerData*)data;
                       NetworkURLResourceRequestJobDelegate::worker(d);
                       return nullptr;
                   },
                   data);
        } else {
            data->networkWorker = new AsyncNetworkWorkHelper();
            m_orgProxy->starFish()->threadPool()->addWork(
                m_orgProxy->document()->browsingContext(),
                NetworkURLResourceRequestJobDelegate::worker, data);
        }
    }
}

void NetworkURLResourceRequestJobDelegate::fillHeadersWithGeneralHeaders(
    HTTPHeaderMap& headers)
{
    // Set General header
    //  * Cache-Control, Connection, Date, Pragma, Trailer, Transfer-Encoding,
    //  * Upgrade, Via, Warning ...
    headers.setHeader(HTTPHeaderMap::kConnection, "keep-alive");
}

void NetworkURLResourceRequestJobDelegate::fillHeadersWithClientHeaders(
    HTTPHeaderMap& headers)
{
    // Set Client Request header
    //  * Accept, Accept-Charset, Accept-Encoding, Accept-Language,
    //  * Authorization, Cookie, Expect, From, Host, If-Match,
    //  * If-Modified-Since, If-None-Match, If-Range, If-Unmodified-Since,
    //  * Max-Forwards, Origin, Proxy-Authorization, Range, Referer, TE,
    //  * User-Agent ...
    std::string tmpStr;
    tmpStr = m_orgProxy->starFish()->locale().getName();
    std::replace(tmpStr.begin(), tmpStr.end(), '_', '-');
    headers.setHeader(HTTPHeaderMap::kAcceptLanguage, tmpStr.data());
    headers.setHeader(HTTPHeaderMap::kPragma, "no-cache");
    headers.setHeader(HTTPHeaderMap::kCacheControl, "no-cache");
    headers.setHeader(
        HTTPHeaderMap::kUserAgent,
        m_orgProxy->starFish()->userAgent()->toUTF8NonGCString().data());

    headers.setHeader(HTTPHeaderMap::kUpgradeInsecureRequests, "1");

    if (!m_orgProxy->m_document->documentURI()->isNetworkURL()) {
        headers.setHeader(HTTPHeaderMap::kOrigin, "null");
    } else {
        auto it2 = headers.findHeader(HTTPHeaderMap::kReferer);
        if (it2 == headers.headerMap().end() && m_orgProxy->referrer()) {
            auto urlUTF8Data =
                m_orgProxy->referrer()->urlString()->toUTF8NonGCString();
            headers.setHeader(HTTPHeaderMap::kReferer, urlUTF8Data);
        }
    }
}

void NetworkURLResourceRequestJobDelegate::fillHeadersWithResourceRequestHeader(
    HTTPHeaderMap& headers)
{
    for (size_t i = 0; i < m_orgProxy->m_requestHeaders.size(); i++) {
        auto utf8Data1 =
            m_orgProxy->m_requestHeaders[i].first->toUTF8NonGCString();
        auto utf8Data2 =
            m_orgProxy->m_requestHeaders[i].second->toUTF8NonGCString();
        headers.setHeader(utf8Data1, utf8Data2);
    }
}

void* NetworkURLResourceRequestJobDelegate::worker(void* data)
{
    NetworkURLWorkerData* requestData = (NetworkURLWorkerData*)data;
    return requestData->networkWorker->networkWorker(data);
}

int NetworkURLResourceRequestJobDelegate::curlProgressCallback(
    void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal,
    curl_off_t ulnow)
{
    NetworkURLWorkerData* workerData = (NetworkURLWorkerData*)clientp;
    ResourceRequest* request = workerData->request;
    Locker<Mutex> locker(*request->m_mutex);
    // check abort
    if (workerData->isAborted) {
        return 1;
    }

    request->m_loaded = static_cast<uint32_t>(dlnow);
    request->m_total = static_cast<uint32_t>(dltotal);
    return 0;
}

size_t NetworkURLResourceRequestJobDelegate::curlWriteCallback(void* ptr,
                                                               size_t size,
                                                               size_t nmemb,
                                                               void* data)
{
    NetworkURLWorkerData* workerData = (NetworkURLWorkerData*)data;
    ResourceRequest* request = workerData->request;

    Locker<Mutex> locker(*request->m_mutex);

    size_t realSize = size * nmemb;
    const char* memPtr = (const char*)ptr;

    auto& entityBody = request->response();
    entityBody.insert(entityBody.end(), memPtr, memPtr + realSize);

    if (request->m_pendingOnProgressEventIdlerHandle == SIZE_MAX) {
        if (request->isSync()) {
            request->changeReadyState(ResourceRequest::LOADING, true);
            request->changeProgress(ResourceRequest::PROGRESS, true);
        } else {
            request->m_pendingOnProgressEventIdlerHandle =
                request->starFish()
                    ->messageLoop()
                    ->addIdlerWithNoGCRootingInOtherThread(
                        request->document()->browsingContext(),
                        [](size_t handle, void* data) {
                            ResourceRequest* request = (ResourceRequest*)data;
                            Locker<Mutex> locker(*request->m_mutex);
                            {
                                STARFISH_ASSERT(
                                    handle ==
                                    request
                                        ->m_pendingOnProgressEventIdlerHandle);
                                request->m_pendingOnProgressEventIdlerHandle =
                                    SIZE_MAX;
                            }
                            request->changeReadyState(ResourceRequest::LOADING,
                                                      true);
                            request->changeProgress(ResourceRequest::PROGRESS,
                                                    true);
                        },
                        request);
        }
    }

    return realSize;
}

size_t NetworkURLResourceRequestJobDelegate::curlWriteHeaderCallback(
    void* ptr, size_t size, size_t nmemb, void* data)
{
    NetworkURLWorkerData* workerData = (NetworkURLWorkerData*)data;
    ResourceRequest* request = workerData->request;

    Locker<Mutex> locker(*request->m_mutex);

    workerData->httpTransaction->updateTransactionStatus();
    size_t realSize = size * nmemb;
    std::string rawHeader(static_cast<const char*>(ptr), realSize);

    request->m_status =
        workerData->httpTransaction->httpResponse().responseCode();

    if (workerData->httpTransaction->httpResponse()
            .isSuccessfulResponseStatus()) {
        if ((rawHeader.compare("\r\n") == 0) ||
            (rawHeader.compare("\n") == 0)) {
            request->m_responseHeaderMap =
                std::move(workerData->httpTransaction->httpResponse()
                              .headers()
                              .headerMap());

            if (request->m_pendingOnHeaderReceivedEventIdlerHandle ==
                SIZE_MAX) {
                if (request->isSync()) {
                    request->changeReadyState(ResourceRequest::HEADERS_RECEIVED,
                                              true);
                } else {
                    request->m_pendingOnHeaderReceivedEventIdlerHandle =
                        request->starFish()
                            ->messageLoop()
                            ->addIdlerWithNoGCRootingInOtherThread(
                                request->document()->browsingContext(),
                                [](size_t handle, void* data) {
                                    ResourceRequest* request =
                                        (ResourceRequest*)data;
                                    Locker<Mutex> locker(*request->m_mutex);
                                    {
                                        STARFISH_ASSERT(
                                            handle ==
                                            request
                                                ->m_pendingOnHeaderReceivedEventIdlerHandle);
                                        request
                                            ->m_pendingOnHeaderReceivedEventIdlerHandle =
                                            SIZE_MAX;
                                    }
                                    request->changeReadyState(
                                        ResourceRequest::HEADERS_RECEIVED,
                                        true);
                                },
                                request);
                }
            }
        } else {
            workerData->httpTransaction->didReceiveHeader(rawHeader);
        }
    } else if (workerData->httpTransaction->httpResponse()
                   .isRedirectionResponseStatus()) {
        if (!workerData->isRedirected) {
            workerData->isRedirected = true;
        }
    }

    return realSize;
}
}
