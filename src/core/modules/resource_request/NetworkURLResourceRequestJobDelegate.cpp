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
#if defined(STARFISH_ENABLE_HTTPCACHE)
#include "platform/network/HTTPCache.h"
#include "platform/network/HTTPCacheEntry.h"
#endif
#include "binding/ScriptWrappable.h"
#include "platform/network/http/HTTPHeaderMap.h"
#include "platform/network/http/HTTPRequest.h"
#include "platform/network/http/HTTPResponse.h"
#include "platform/network/http/HTTPStatus.h"
#include "platform/network/http/HTTPTransaction.h"
#include "platform/network/http/HTTPUtil.h"
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
#ifdef STARFISH_ENABLE_PROFILING
int64_t NetworkURLWorkerData::reqCnt = 0;
int64_t NetworkURLWorkerData::hitCnt = 0;
#endif

NetworkURLWorkerData::NetworkURLWorkerData(ResourceRequest* orgRequest)
    : isAborted(false)
    , isRedirected(false)
    , lastTransactionResponseCode(0)
    , request(orgRequest)
    , helper(nullptr)
    , httpTransaction(HTTPTransaction::create())
#ifdef STARFISH_ENABLE_HTTPCACHE
    , cachedEntry(nullptr)
#endif
    , lastLocation("")
#ifdef STARFISH_ENABLE_PROFILING
    , workingTime(0)
    , cachehit(false)
#endif
{
#ifdef STARFISH_ENABLE_PROFILING
    NetworkURLWorkerData::reqCnt++;
#endif
}

NetworkURLWorkerData::~NetworkURLWorkerData()
{
#ifdef STARFISH_ENABLE_HTTPCACHE
    if (cachedEntry) {
        cachedEntry->decreaseUsingCount();
    }
#endif
}

void* NetworkURLWorkerHelper::networkWorker(void* data)
{
    NetworkURLWorkerData* nwd = (NetworkURLWorkerData*)data;
#ifdef STARFISH_ENABLE_PROFILING
    uint64_t start = longTickCount();
#endif
    nwd->httpTransaction->start();
#ifdef STARFISH_ENABLE_PROFILING
    uint64_t end = longTickCount();
    nwd->workingTime += end - start;
#endif

    // TODO : Do not use libur libcurl error codes
    if (nwd->httpTransaction->res() != CURLE_ABORTED_BY_CALLBACK) {
#ifdef STARFISH_ENABLE_HTTPCACHE
        if (nwd->cachedEntry) {
            nwd->cachedEntry->setNeedsPropertiesUpdate(true);
            if (nwd->httpTransaction->httpResponse().responseCode() ==
                HTTPStatusCode::HTTP_STATUS_NOT_MODIFIED) {
                NetworkURLWorkerHelper::httpCacheWorker(nwd);
            } else {
                nwd->cachedEntry->setNeedsRawDataUpdate(true);
                responseHandlerWrapper(nwd->httpTransaction->res(), nwd);
            }
        } else {
            responseHandlerWrapper(nwd->httpTransaction->res(), nwd);
        }
#else
        responseHandlerWrapper(nwd->httpTransaction->res(), nwd);
#endif
    } else {
        workerAbortHandeler(nwd);
    }
    return nullptr;
}

#ifdef STARFISH_ENABLE_HTTPCACHE
void* NetworkURLWorkerHelper::httpCacheWorker(void* data)
{
#ifdef STARFISH_ENABLE_PROFILING
    uint64_t start = longTickCount();
#endif
    NetworkURLWorkerData* nwd = (NetworkURLWorkerData*)data;
    ResourceRequest* request = (ResourceRequest*)nwd->request;

    bool ret;
    {
        // NOTE: may need the headers received when RawData cached, but
        // currently only the entity-body is cached.
        Locker<Mutex> locker(*request->m_mutex);
        ret = nwd->cachedEntry->readRawDataFromEntryFile(
            nwd->request->response());
        nwd->cachedEntry->readEntryHeaders(nwd->request->m_responseHeaderMap);
#ifdef STARFISH_ENABLE_PROFILING
        nwd->cachehit = true;
        uint64_t end = longTickCount();
        nwd->workingTime += end - start;
#endif
    }
    if (ret) {
        nwd->httpTransaction->httpResponse().setResponseCode(200);
        responseHandlerWrapper(nwd->httpTransaction->res(), nwd);
    } else {
        workerAbortHandeler(nwd);
    }
    return nullptr;
}
#endif
void NetworkURLWorkerHelper::workerAbortHandeler(void* data)
{
    NetworkURLWorkerData* nwd = (NetworkURLWorkerData*)data;
    nwd->request->starFish()
        ->messageLoop()
        ->addIdlerWithNoGCRootingInOtherThread(
            nullptr,
            [](size_t, void* data) {
                NetworkURLWorkerData* d = (NetworkURLWorkerData*)data;
                if (d == d->request->m_activeNetworkURLWorkerData) {
                    d->request->m_activeNetworkURLWorkerData = nullptr;
                }
                d->~NetworkURLWorkerData();
                GC_FREE(d);
            },
            nwd);
}

void NetworkURLWorkerHelper::responseHandler(size_t handle, void* data)
{
    NetworkURLWorkerData* nwd = (NetworkURLWorkerData*)data;
    STARFISH_ASSERT(isMainThread());
    // TODO : Do not use libur libcurl error codes
    STARFISH_ASSERT(nwd->httpTransaction->res() != CURLE_ABORTED_BY_CALLBACK);

    if (nwd->isAborted) {
    } else if (nwd->httpTransaction->res() == 0) {
        if (nwd->isRedirected) {
            nwd->request->m_lastLocation =
                String::createASCIIString(nwd->httpTransaction->httpResponse()
                                              .lastEffectiveURL()
                                              .data())
                    ->trim();
        }
#ifdef STARFISH_ENABLE_HTTPCACHE
        HTTPCache* cache = nwd->request->starFish()->httpCache();
        if (cache) {
            if (!nwd->cachedEntry) {
                cache->put(nwd);
            } // TODO : If the entry is before it expires but is no longer fresh
            else if (nwd->cachedEntry) {
                cache->update(nwd, nwd->cachedEntry);
            }
        }
#endif
        nwd->request->handleResponseEOF();
    } else if (nwd->httpTransaction->res() == CURLE_OPERATION_TIMEDOUT) {
        auto s = nwd->request->m_url->urlString()->toUTF8NonGCString();
        STARFISH_LOG_INFO(
            "got timeout %s[%d]\n", s.data(),
            (int)nwd->httpTransaction->httpResponse().responseCode());
        nwd->request->handleError(ResourceRequest::TIMEOUT);
    } else {
        auto s = nwd->request->m_url->urlString()->toUTF8NonGCString();
        STARFISH_LOG_INFO("failed to open %s\n", s.data());
        nwd->request->handleError(ResourceRequest::ERROR);
    }

    if (NetworkSharedResourceManager::getInstance()->cacheClearTimerID() !=
        SIZE_MAX) {
        nwd->request->starFish()->timer()->removeTimer(
            NetworkSharedResourceManager::getInstance()->cacheClearTimerID());
    }

    size_t timerID = nwd->request->starFish()->timer()->addTimer(
        STARFISH_CURL_HANDLE_CACHE_CLEAR_TIMEOUT_IN_MS,
        nwd->request->document()->window(),
        [](Window* wnd, void* data) {
            NetworkSharedResourceManager::getInstance()
                ->clearAllCurlHandleDataCache();
            NetworkSharedResourceManager::getInstance()->setCacheClearTimerID(
                SIZE_MAX);
        },
        nullptr, false);

    NetworkSharedResourceManager::getInstance()->setCacheClearTimerID(timerID);

    nwd->request->m_activeNetworkURLWorkerData = nullptr;
    nwd->~NetworkURLWorkerData();
    GC_FREE(nwd);
}

void SyncNetworkWorkHelper::responseHandlerWrapper(int res,
                                                   NetworkURLWorkerData* nwd)
{
    responseHandler(res, nwd);
}

void AsyncNetworkWorkHelper::responseHandlerWrapper(int res,
                                                    NetworkURLWorkerData* nwd)
{
    Locker<Mutex> locker(*nwd->request->m_mutex);
#ifdef STARFISH_ENABLE_PROFILING
    if (nwd->cachehit) {
        NetworkURLWorkerData::hitCnt++;
    }
    STARFISH_LOG_INFO(
        "[Profiling] Resource Raw data(%zu byte) Load in %f ms, diskcache: "
        "%s, HitRate: %lf\n",
        nwd->request->response().size(), (float)((nwd->workingTime) / 1000.f),
        (nwd->cachehit) ? "hit" : "miss",
        (NetworkURLWorkerData::hitCnt)
            ? (double)NetworkURLWorkerData::hitCnt /
                  NetworkURLWorkerData::reqCnt
            : 0);
#endif
    nwd->request->starFish()
        ->messageLoop()
        ->addIdlerWithNoGCRootingInOtherThread(nullptr, this->responseHandler,
                                               nwd);
}

NetworkURLResourceRequestJobDelegate::NetworkURLResourceRequestJobDelegate(
    ResourceRequest* proxy)
    : m_orgProxy(proxy)
{
}

void NetworkURLResourceRequestJobDelegate::send(String* body, bool allowCache)
{
    STARFISH_ASSERT(isMainThread());
    STARFISH_ASSERT(m_orgProxy->m_url->isNetworkURL());

    NetworkURLWorkerData* nwd = new (NoGC) NetworkURLWorkerData(m_orgProxy);

    m_orgProxy->m_activeNetworkURLWorkerData = nwd;
    HTTPHeaderMap headers;

    std::string method;
    switch (m_orgProxy->m_method) {
    case ResourceRequest::GET_METHOD: {
        method = "GET";
#ifdef STARFISH_ENABLE_PROFILING
        uint64_t start = longTickCount();
#endif
#ifdef STARFISH_ENABLE_HTTPCACHE
        if (allowCache && m_orgProxy->starFish()->httpCache()) {
            auto it =
                m_orgProxy->starFish()->httpCache()->get(m_orgProxy->m_url);

            if (it != m_orgProxy->starFish()->httpCache()->end()) {
                nwd->cachedEntry = it->second;
                nwd->cachedEntry->increaseUsingCount();
                fillHeadersWithCachedEntry(headers, nwd->cachedEntry);
            }
        }
#endif
#ifdef STARFISH_ENABLE_PROFILING
        uint64_t end = longTickCount();
        nwd->workingTime += end - start;
#endif
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

    fillHeadersWithResourceRequestHeader(headers);
    fillHeadersWithClientHeaders(headers);
    fillHeadersWithGeneralHeaders(headers);

    auto urlUTF8Data = m_orgProxy->m_url->urlString()->toUTF8NonGCString();
    auto hostUTF8Data = m_orgProxy->m_url->host()->toUTF8NonGCString();
    auto bodyUTF8Data = body->toUTF8NonGCString();
    nwd->httpTransaction->setHTTPRequest(HTTPRequest::create(
        urlUTF8Data, hostUTF8Data, method, headers, bodyUTF8Data));
    nwd->httpTransaction->setTimeout(
        static_cast<unsigned long>(m_orgProxy->m_timeout));

    nwd->httpTransaction->setProgressCallbackAndData(curlProgressCallback, nwd);
    nwd->httpTransaction->setWriteHeaderCallbackAndData(curlWriteHeaderCallback,
                                                        nwd);
    nwd->httpTransaction->setWriteCallbackAndData(curlWriteCallback, nwd);

    if (m_orgProxy->isSync()) {
        nwd->helper = new SyncNetworkWorkHelper();
        worker(nwd);
    } else {
        auto& header = headers.headerMap();
        auto pos = std::find_if(
            header.begin(), header.end(),
            [](const std::unordered_map<std::string, std::string>::value_type&
                   o) { return o.second == "text/event-stream"; });

        if (pos != header.end()) {
            nwd->helper = new AsyncNetworkWorkHelper();
            Thread* t = new Thread(m_orgProxy->starFish());
            t->run(m_orgProxy->starFish()->messageLoop(),
                   [](void* data) -> void* {
                       NetworkURLWorkerData* d = (NetworkURLWorkerData*)data;
                       NetworkURLResourceRequestJobDelegate::worker(d);
                       return nullptr;
                   },
                   nwd);
        } else {
            nwd->helper = new AsyncNetworkWorkHelper();
            m_orgProxy->starFish()->threadPool()->addWork(
                m_orgProxy->document()->browsingContext(),
                NetworkURLResourceRequestJobDelegate::worker, nwd);
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
    //  * Max-Forwards, Origin, Proxy-Authorization, Range, Referer, TE,
    //  * User-Agent ...
    std::string tmpStr;
    tmpStr = m_orgProxy->starFish()->locale().getName();
    std::replace(tmpStr.begin(), tmpStr.end(), '_', '-');
    tmpStr = tmpStr + " , en-US , en";
    headers.setHeader(HTTPHeaderMap::kAcceptLanguage, tmpStr.data());
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
#ifdef STARFISH_ENABLE_HTTPCACHE
void NetworkURLResourceRequestJobDelegate::fillHeadersWithCachedEntry(
    HTTPHeaderMap& headers, HTTPCacheEntry* cachedEntry)
{
    // * If-Modified-Since, If-None-Match, If-Range, If-Unmodified-Since
    if (!cachedEntry->shouldReValidate()) {
        return;
    }

    // If-Modified-Since = lastModified or HTTP-date
    // When used for cache updates, a cache will typically use the value of
    // the cached message's Last-Modified field to generate the field value
    // of If-Modified-Since.  This behavior is most interoperable for cases
    // where clocks are poorly synchronized or when the server has chosen to
    // only honor exact timestamp matches (due to a problem with
    // Last-Modified dates that appear to go "back in time" when the origin
    // server's clock is corrected or a representation is restored from an
    // archived backup).  However, caches occasionally generate the field
    // value based on other data, such as the Date header field of the
    // cached message or the local clock time that the message was received,
    // particularly when the cached message does not contain a Last-Modified
    // field.
    std::string value;
    HTTPFreshnessInfo info = cachedEntry->httpFreshnessInfo();

    if (info.lastModified) {
        std::string value =
            timeToUTCString(m_orgProxy->document()->scriptBindingInstance(),
                            info.lastModified * 1000)
                ->toUTF8NonGCString();
        headers.setHeader(HTTPHeaderMap::kIfModifiedSince, value);
    } else if (info.date) {
        std::string value =
            timeToUTCString(m_orgProxy->document()->scriptBindingInstance(),
                            info.date * 1000)
                ->toUTF8NonGCString();
        headers.setHeader(HTTPHeaderMap::kIfModifiedSince, value);
    }

    if (info.etag.size()) {
        headers.setHeader(HTTPHeaderMap::kIfNoneMatch, info.etag);
    }
}
#endif
void* NetworkURLResourceRequestJobDelegate::worker(void* data)
{
    NetworkURLWorkerData* nwd = (NetworkURLWorkerData*)data;
#ifdef STARFISH_ENABLE_HTTPCACHE
    if (!nwd->cachedEntry || (nwd->cachedEntry->shouldReValidate())) {
        return nwd->helper->networkWorker(data);
    } else {
        return nwd->helper->httpCacheWorker(data);
    }
#else
    return nwd->helper->networkWorker(data);
#endif
}

int NetworkURLResourceRequestJobDelegate::curlProgressCallback(
    void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal,
    curl_off_t ulnow)
{
    NetworkURLWorkerData* nwd = (NetworkURLWorkerData*)clientp;
    ResourceRequest* request = nwd->request;
    Locker<Mutex> locker(*request->m_mutex);
    if (nwd->isAborted) {
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
    NetworkURLWorkerData* nwd = (NetworkURLWorkerData*)data;
    ResourceRequest* request = nwd->request;

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
                        nullptr,
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
    NetworkURLWorkerData* nwd = (NetworkURLWorkerData*)data;
    ResourceRequest* request = nwd->request;

    Locker<Mutex> locker(*request->m_mutex);

    nwd->httpTransaction->updateTransactionStatus();
    size_t realSize = size * nmemb;
    std::string rawHeader(static_cast<const char*>(ptr), realSize);

    request->m_status = nwd->httpTransaction->httpResponse().responseCode();

    if (nwd->httpTransaction->httpResponse().isSuccessfulResponseStatus()) {
        if ((rawHeader.compare("\r\n") == 0) ||
            (rawHeader.compare("\n") == 0)) {
            request->m_responseHeaderMap = std::move(
                nwd->httpTransaction->httpResponse().headers().headerMap());

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
                                nullptr,
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
            nwd->httpTransaction->didReceiveHeader(rawHeader);
        }
    } else if (nwd->httpTransaction->httpResponse()
                   .isRedirectionResponseStatus()) {
        if (!nwd->isRedirected) {
            nwd->isRedirected = true;
        }
    }

    return realSize;
}
}
