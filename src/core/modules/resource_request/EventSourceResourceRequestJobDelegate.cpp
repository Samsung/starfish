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
#include "core/modules/resource_request/EventSourceResourceRequestJobDelegate.h"
#include "platform/network/NetworkSharedResourceManager.h"
#include "core/modules/resource_request/ResourceRequest.h"
#include "core/modules/threading/ThreadPool.h"
#include "core/modules/message_loop/Timer.h"

#define CURL_HANDLE_DATA_CLEAR_TIME_S 1

namespace StarFish {

void* EventSourceWorkerHelper::networkWorker(void* data)
{
    EventSourceWorkerData* requestData = (EventSourceWorkerData*)data;
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
                    EventSourceWorkerData* requestData =
                        (EventSourceWorkerData*)data;
                    if (requestData ==
                        requestData->request->m_activeEventSourceWorkerData) {
                        requestData->request->m_activeEventSourceWorkerData =
                            nullptr;
                    }
                    requestData->~EventSourceWorkerData();
                    GC_FREE(requestData);
                },
                requestData);
    }

    return NULL;
}

void SyncEventSourceWorkHelper::responseHandlerWrapper(
    int res, EventSourceWorkerData* requestData)
{
    responseHandler(res, requestData);
}

void AsyncEventSourceWorkHelper::responseHandlerWrapper(
    int res, EventSourceWorkerData* requestData)
{
    Locker<Mutex> locker(*requestData->request->m_mutex);
    requestData->request->m_pendingNetworkWorkerEndIdlerHandle =
        requestData->request->starFish()
            ->messageLoop()
            ->addIdlerWithNoGCRootingInOtherThread(
                requestData->request->document()->browsingContext(),
                this->responseHandler, requestData);
}

void EventSourceWorkerHelper::responseHandler(size_t handle, void* data)
{
    EventSourceWorkerData* requestData = (EventSourceWorkerData*)data;
    STARFISH_ASSERT(isMainThread());

    // TODO : handle close()

    requestData->request->m_activeEventSourceWorkerData = nullptr;
    requestData->~EventSourceWorkerData();
    GC_FREE(requestData);
}

EventSourceResourceRequestJobDelegate::EventSourceResourceRequestJobDelegate(
    ResourceRequest* proxy)
    : m_orgProxy(proxy)
{
}

void EventSourceResourceRequestJobDelegate::send(String* body)
{
    STARFISH_ASSERT(isMainThread());

    EventSourceWorkerData* data = new (NoGC) EventSourceWorkerData();
    data->request = m_orgProxy;
    data->isAborted = false;
    data->isRedirected = false;
    data->lastLocation = "";
    m_orgProxy->m_activeEventSourceWorkerData = data;

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

    data->httpTransaction->setHTTPRequest(
        HTTPRequest::create(m_orgProxy->m_url->urlString()->toUTF8NonGCString(),
                            m_orgProxy->m_url->host()->toUTF8NonGCString(),
                            method, headers, body->toUTF8NonGCString()));

    data->httpTransaction->setTimeout(
        static_cast<unsigned long>(m_orgProxy->m_timeout));
    data->httpTransaction->setProgressCallbackAndData(curlProgressCallback,
                                                      data);
    data->httpTransaction->setWriteHeaderCallbackAndData(
        curlWriteHeaderCallback, data);
    data->httpTransaction->setWriteCallbackAndData(curlWriteCallback, data);

    if (m_orgProxy->isSync()) {
        data->eventSourceNetworkWorker = new SyncEventSourceWorkHelper();
        worker(data);
    } else {
        data->eventSourceNetworkWorker = new AsyncEventSourceWorkHelper();
        m_orgProxy->starFish()->threadPool()->addWork(
            m_orgProxy->document()->browsingContext(),
            EventSourceResourceRequestJobDelegate::worker, data);
    }
}

void EventSourceResourceRequestJobDelegate::fillHeadersWithGeneralHeaders(
    HTTPHeaderMap& headers)
{
    // Set General header
    //  * Cache-Control, Connection, Date, Pragma, Trailer, Transfer-Encoding,
    //  * Upgrade, Via, Warning ...
    headers.setHeader(HTTPHeaderMap::kConnection, "keep-alive");
}

void EventSourceResourceRequestJobDelegate::fillHeadersWithClientHeaders(
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
    tmpStr.replace(tmpStr.begin(), tmpStr.end(), '_', '-');
    headers.setHeader(HTTPHeaderMap::kAcceptLanguage, tmpStr.data());

    headers.setHeader(HTTPHeaderMap::kAcceptCharset, "utf-8");

    headers.setHeader(HTTPHeaderMap::kUserAgent,
                      USER_AGENT(APP_CODE_NAME, VERSION));
    if (!m_orgProxy->m_document->documentURI()->isNetworkURL()) {
        headers.setHeader(HTTPHeaderMap::kOrigin, "null");
    } else {
        headers.setHeader(HTTPHeaderMap::kHost,
                          m_orgProxy->m_url->hostname()->utf8Data());
        headers.setHeader(HTTPHeaderMap::kReferer,
                          m_orgProxy->m_document->urlString()->utf8Data());
    }
}

void EventSourceResourceRequestJobDelegate::
    fillHeadersWithResourceRequestHeader(HTTPHeaderMap& headers)
{
    for (size_t i = 0; i < m_orgProxy->m_requestHeaders.size(); i++) {
        headers.setHeader(m_orgProxy->m_requestHeaders[i].first->utf8Data(),
                          m_orgProxy->m_requestHeaders[i].second->utf8Data());
    }
}

void* EventSourceResourceRequestJobDelegate::worker(void* data)
{
    EventSourceWorkerData* requestData = (EventSourceWorkerData*)data;
    return requestData->eventSourceNetworkWorker->networkWorker(data);
}

int EventSourceResourceRequestJobDelegate::curlProgressCallback(
    void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal,
    curl_off_t ulnow)
{
    EventSourceWorkerData* workerData = (EventSourceWorkerData*)clientp;
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

size_t EventSourceResourceRequestJobDelegate::curlWriteCallback(void* ptr,
                                                                size_t size,
                                                                size_t nmemb,
                                                                void* data)
{
    EventSourceWorkerData* workerData = (EventSourceWorkerData*)data;
    ResourceRequest* request = workerData->request;

    Locker<Mutex> locker(*request->m_mutex);

    size_t realSize = size * nmemb;
    const char* memPtr = (const char*)ptr;

    auto& entityBody = workerData->httpTransaction->httpResponse().entityBody();
    entityBody.insert(entityBody.end(), memPtr, memPtr + realSize);

    if (request->m_pendingOnProgressEventIdlerHandle == SIZE_MAX) {
        if (request->isSync()) {
            request->changeReadyStateForEventSource(ResourceRequest::OPEN,
                                                    true);
        } else {
            request->m_pendingOnProgressEventIdlerHandle =
                request->starFish()
                    ->messageLoop()
                    ->addIdlerWithNoGCRootingInOtherThread(
                        request->document()->browsingContext(),
                        [](size_t handle, void* data) {
                            EventSourceWorkerData* eventSourceData =
                                (EventSourceWorkerData*)data;
                            ResourceRequest* request = eventSourceData->request;
                            Locker<Mutex> locker(*request->m_mutex);
                            {
                                STARFISH_ASSERT(
                                    handle ==
                                    request
                                        ->m_pendingOnProgressEventIdlerHandle);
                                request->m_pendingOnProgressEventIdlerHandle =
                                    SIZE_MAX;
                            }
                            {
                                request->m_response =
                                    std::move(eventSourceData->httpTransaction
                                                  ->httpResponse()
                                                  .entityBody());
                                if (eventSourceData->isRedirected &&
                                    eventSourceData->lastLocation.compare("") !=
                                        0) {
                                    request->m_lastLocation =
                                        String::createASCIIString(
                                            eventSourceData->lastLocation
                                                .data())
                                            ->trim();
                                }
                            }
                            request->changeReadyStateForEventSource(
                                ResourceRequest::OPEN, true);
                        },
                        workerData);
        }
    }

    return realSize;
}

size_t EventSourceResourceRequestJobDelegate::curlWriteHeaderCallback(
    void* ptr, size_t size, size_t nmemb, void* data)
{
    EventSourceWorkerData* workerData = (EventSourceWorkerData*)data;
    ResourceRequest* request = workerData->request;

    Locker<Mutex> locker(*request->m_mutex);

    workerData->httpTransaction->updateTransactionStatus();
    size_t realSize = size * nmemb;
    std::string rawHeader(static_cast<const char*>(ptr), realSize);

    if (workerData->httpTransaction->httpResponse()
            .isSuccessfulResponseStatus()) {
        if ((rawHeader.compare("\r\n") == 0) ||
            (rawHeader.compare("\n") == 0)) {
            request->m_responseHeaderMap =
                std::move(workerData->httpTransaction->httpResponse()
                              .headers()
                              .headerMap());
            request->m_status =
                workerData->httpTransaction->httpResponse().responseCode();

            if (request->m_pendingOnHeaderReceivedEventIdlerHandle ==
                SIZE_MAX) {
                if (request->isSync()) {
                    request->changeReadyStateForEventSource(
                        ResourceRequest::HEADERS_RECEIVED, true);
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
                                    request->changeReadyStateForEventSource(
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
        size_t pos = rawHeader.find(":");
        if (pos != std::string::npos) {
            std::string key = rawHeader.substr(0, pos);
            std::string value = rawHeader.substr(pos + 1);
            if (StringUtils::equalsWithoutCase(key, HTTPHeaderMap::kLocation)) {
                workerData->lastLocation = value;
            }
        }
    }

    return realSize;
}
}
