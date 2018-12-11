/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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
#include "core/fetch/FetchUtils.h"
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
#include "core/page/WebView.h"
#include "core/dom/WebOrigin.h"

#ifndef STARFISH_CURL_HANDLE_CACHE_CLEAR_TIMEOUT_IN_MS
#define STARFISH_CURL_HANDLE_CACHE_CLEAR_TIMEOUT_IN_MS 5000
#endif

namespace Starfish {
#ifdef STARFISH_ENABLE_NETWORK_PROFILING
int64_t NetworkURLWorkerData::reqCnt = 0;
int64_t NetworkURLWorkerData::hitCnt = 0;
extern uint64_t g_profilingBaseTime;
#endif

NetworkURLWorkerData::NetworkURLWorkerData(ResourceRequest* orgRequest)
    : isAborted(false)
    , needsToHandleError(false)
    , lastTransactionResponseCode(0)
    , corsFlag(false)
    , corsPreflightFlag(false)
    , hasCorsUnsafeRequestHeaderNames(false)
    , needsToSendPreflightRequest(false)
    , request(orgRequest)
    , helper(nullptr)
    , httpTransaction(HTTPTransaction::create())
#ifdef STARFISH_ENABLE_HTTPCACHE
    , cachedEntry(nullptr)
#endif
#ifdef STARFISH_ENABLE_NETWORK_PROFILING
    , workingTime(0)
    , cachehit(false)
#endif
{
#ifdef STARFISH_ENABLE_NETWORK_PROFILING
    NetworkURLWorkerData::reqCnt++;
#endif
}

NetworkURLWorkerData::~NetworkURLWorkerData()
{
#ifdef STARFISH_ENABLE_HTTPCACHE
    if (cachedEntry) {
        cachedEntry->deref();
    }
#endif
}

void* NetworkURLResourceRequestJobDelegate::networkWorker(void* data)
{
    NetworkURLWorkerData* nwd = (NetworkURLWorkerData*)data;
    if (nwd->needsToSendPreflightRequest) {
        nwd->httpTransaction->startPreFlightRequest();
        if (nwd->httpTransaction->httpResponse().isSuccessfulResponseStatus() &&
            nwd->needsToHandleError) {
            nwd->helper->abortHandlerWrapper(nwd);
            return nullptr;
        }
        nwd->needsToSendPreflightRequest = false;
    }

    nwd->httpTransaction->start();

    if (!(nwd->httpTransaction->res() == CURLE_ABORTED_BY_CALLBACK ||
          nwd->httpTransaction->res() == CURLE_WRITE_ERROR)) {
#ifdef STARFISH_ENABLE_HTTPCACHE
        if (nwd->cachedEntry) {
            nwd->cachedEntry->setNeedsPropertiesUpdate(true);
            if (nwd->httpTransaction->httpResponse().responseCode() ==
                HTTPStatusCode::HTTP_STATUS_NOT_MODIFIED) {
                NetworkURLResourceRequestJobDelegate::httpCacheWorker(nwd);
            } else {
                nwd->cachedEntry->setNeedsRawDataUpdate(true);
                nwd->helper->responseHandlerWrapper(nwd);
            }
        } else {
            nwd->helper->responseHandlerWrapper(nwd);
        }
#else
        nwd->helper->responseHandlerWrapper(nwd);
#endif
    } else {
        nwd->helper->abortHandlerWrapper(nwd);
    }
    return nullptr;
}

#ifdef STARFISH_ENABLE_HTTPCACHE
void* NetworkURLResourceRequestJobDelegate::httpCacheWorker(void* data)
{
#ifdef STARFISH_ENABLE_NETWORK_PROFILING
    uint64_t start = longTickCount();
#endif
    NetworkURLWorkerData* nwd = (NetworkURLWorkerData*)data;
    ResourceRequest* request = (ResourceRequest*)nwd->request;

    bool ret;
    {
        Locker<Mutex> locker(*request->m_mutex);
        ret = nwd->cachedEntry->readRawDataFromEntryFile(
            nwd->request->response());
        nwd->cachedEntry->readEntryHeaders(
            nwd->request->m_responseHeaders->httpHeaderMap()->headerMap());
#ifdef STARFISH_ENABLE_NETWORK_PROFILING
        nwd->cachehit = true;
        uint64_t end = longTickCount();
        nwd->workingTime += end - start;
        STARFISH_LOG_INFO(
            "[NETWORK_PROFILING] Http disk cache hit! %s at %dms\n",
            nwd->httpTransaction->httpRequest().url().data(),
            (int)(timestamp() - g_profilingBaseTime));
#endif
    }
    if (ret) {
        request->m_responseData->m_status = 200;
        nwd->helper->responseHandlerWrapper(nwd);
    } else {
        nwd->helper->abortHandlerWrapper(nwd);
    }
    return nullptr;
}
#endif

void NetworkURLWorkerHelper::abortHandeler(size_t handle, void* data)
{
    NetworkURLWorkerData* nwd = (NetworkURLWorkerData*)data;

    {
        Locker<Mutex> locker(*nwd->request->m_mutex);
        if (nwd == nwd->request->m_activeNetworkURLWorkerData) {
            if (nwd->needsToHandleError) {
                nwd->request->handleError(ProgressState::InError);
            }
            nwd->request->m_activeNetworkURLWorkerData = nullptr;
        }
    }

    nwd->~NetworkURLWorkerData();
    GC_FREE(nwd);
}

void NetworkURLWorkerHelper::responseHandler(size_t handle, void* data)
{
    STARFISH_ASSERT(isMainThread());
    NetworkURLWorkerData* nwd = (NetworkURLWorkerData*)data;
    STARFISH_ASSERT(nwd->httpTransaction->res() != CURLE_ABORTED_BY_CALLBACK);
    STARFISH_ASSERT(nwd->httpTransaction->res() != CURLE_WRITE_ERROR);

    if (nwd->isAborted) {
        abortHandeler(handle, data);
        return;
    }

    Locker<Mutex> locker(*nwd->request->m_mutex);

    if (nwd->httpTransaction->res() == 0) {
        if (nwd->request->m_pendingOnProgressEventIdlerHandle != SIZE_MAX) {
            ResourceRequest* request = nwd->request;
            if (!request->isSync()) {
                request->response().insert(request->response().end(),
                                           nwd->pendingResponseData.begin(),
                                           nwd->pendingResponseData.end());
                nwd->pendingResponseData.clear();
            }
            request->changeReadyState(ReadyState::Loading, true);
            request->changeProgress(ProgressState::Progress, true);

            request->webView()->messageLoop()->removeIdlerWithNoGCRooting(
                request->m_pendingOnProgressEventIdlerHandle);
            request->m_pendingOnProgressEventIdlerHandle = SIZE_MAX;
        }
#ifdef STARFISH_ENABLE_HTTPCACHE
        HTTPCache* cache = nwd->request->starfish()->httpCache();
        if (cache) {
            // FIXME : remove '!nwd->corsPreflightFlag'
            // When 'network-or-cache-fetch' is implemented, the response for
            // request that has useCorsPreflightFlag can be cached.
            if (!nwd->cachedEntry && !nwd->corsPreflightFlag) {
                cache->put(nwd);
            } else if (nwd->cachedEntry) { // TODO : If the entry is before it
                                           // expires but is no longer fresh
                cache->update(nwd, nwd->cachedEntry);
            }
        }
#endif
        nwd->request->handleResponseEOF();
    } else if (nwd->httpTransaction->res() == CURLE_OPERATION_TIMEDOUT) {
        auto s = nwd->request->url()->urlString()->toUTF8NonGCString();
        STARFISH_LOG_INFO(
            "got timeout %s[%d]\n", s.data(),
            (int)nwd->httpTransaction->httpResponse().responseCode());
        nwd->request->handleError(ProgressState::TimeOut);
    } else {
        auto s = nwd->request->url()->urlString()->toUTF8NonGCString();
        STARFISH_LOG_INFO("failed to open %s\n", s.data());
        nwd->request->handleError(ProgressState::InError);
    }

    if (NetworkSharedResourceManager::getInstance()->cacheClearTimerID() !=
        SIZE_MAX) {
        if (nwd->request->webView()->timer()) {
            nwd->request->webView()->timer()->removeTimer(
                NetworkSharedResourceManager::getInstance()
                    ->cacheClearTimerID());
        }
    }

    if (nwd->request->webView()->timer()) {
        size_t timerID = nwd->request->webView()->timer()->addTimer(
            STARFISH_CURL_HANDLE_CACHE_CLEAR_TIMEOUT_IN_MS, nullptr,
            [](Window* wnd, void* data) {
                NetworkSharedResourceManager::getInstance()
                    ->clearAllCurlHandleDataCache();
                NetworkSharedResourceManager::getInstance()
                    ->setCacheClearTimerID(SIZE_MAX);
            },
            nullptr, false);

        NetworkSharedResourceManager::getInstance()->setCacheClearTimerID(
            timerID);
    }

    nwd->request->m_activeNetworkURLWorkerData = nullptr;
    nwd->~NetworkURLWorkerData();
    GC_FREE(nwd);
}

void SyncNetworkWorkHelper::responseHandlerWrapper(NetworkURLWorkerData* nwd)
{
    responseHandler(0, nwd);
}

void SyncNetworkWorkHelper::abortHandlerWrapper(NetworkURLWorkerData* nwd)
{
    abortHandeler(0, nwd);
}

void AsyncNetworkWorkHelper::responseHandlerWrapper(NetworkURLWorkerData* nwd)
{
    Locker<Mutex> locker(*nwd->request->m_mutex);
#ifdef STARFISH_ENABLE_NETWORK_PROFILING
    if (nwd->cachehit) {
        NetworkURLWorkerData::hitCnt++;
    }
    STARFISH_LOG_INFO(
        "[NETWORK_PROFILING] Resource Raw data(%zu byte) Load in %f ms, "
        "diskcache: "
        "%s, HitRate: %lf\n",
        nwd->request->response().size(), (float)((nwd->workingTime) / 1000.f),
        (nwd->cachehit) ? "hit" : "miss",
        (NetworkURLWorkerData::hitCnt)
            ? (double)NetworkURLWorkerData::hitCnt /
                  NetworkURLWorkerData::reqCnt
            : 0);
#endif
    nwd->request->webView()
        ->messageLoop()
        ->addIdlerWithNoGCRootingInOtherThread(nullptr, this->responseHandler,
                                               nwd);
}

void AsyncNetworkWorkHelper::abortHandlerWrapper(NetworkURLWorkerData* nwd)
{
    Locker<Mutex> locker(*nwd->request->m_mutex);
    nwd->request->webView()
        ->messageLoop()
        ->addIdlerWithNoGCRootingInOtherThread(nullptr, this->abortHandeler,
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
    STARFISH_ASSERT(m_orgProxy->url()->isHTTPFamilyURL());

    NetworkURLWorkerData* nwd = new (NoGC) NetworkURLWorkerData(m_orgProxy);

    m_orgProxy->m_activeNetworkURLWorkerData = nwd;
    HTTPHeaderMap headers = *(m_orgProxy->m_requestHeaders->httpHeaderMap());
    if (m_orgProxy->method()->equals("GET")) {
#ifdef STARFISH_ENABLE_NETWORK_PROFILING
        uint64_t start = longTickCount();
#endif
#ifdef STARFISH_ENABLE_HTTPCACHE
        if (allowCache && m_orgProxy->starfish()->httpCache() &&
            m_orgProxy->requestDestination() != RequestDestination::Document) {
            auto it =
                m_orgProxy->starfish()->httpCache()->get(m_orgProxy->url());

            if (it != m_orgProxy->starfish()->httpCache()->end()) {
                nwd->cachedEntry = it->second.get();
                nwd->cachedEntry->ref();
                fillHeadersWithCachedEntry(headers, nwd->cachedEntry);
            }
        }
#endif
#ifdef STARFISH_ENABLE_NETWORK_PROFILING
        uint64_t end = longTickCount();
        nwd->workingTime += end - start;
#endif
    }
    bool includeCredentials = false;
    switch (m_orgProxy->requestCredentials()) {
    case RequestCredentials::SameOrigin:
        if (m_orgProxy->isSameOriginRequest()) {
            includeCredentials = true;
        }
        break;
    case RequestCredentials::Include:
        includeCredentials = true;
        break;
    default:
        includeCredentials = false;
        break;
    }

    if (m_orgProxy->requestMode() == RequestMode::Navigate) {
        includeCredentials = true;
    }
    auto unsafeHeaders = FetchUtils::corsUnsafeRequestHeaderNames(headers);
    nwd->hasCorsUnsafeRequestHeaderNames = unsafeHeaders.size() != 0;
    // https://fetch.spec.whatwg.org/#ref-for-use-cors-preflight-flag%E2%91%A1
    if (m_orgProxy->useCorsPreflightFlag() ||
        (m_orgProxy->unsafeRequestFlag() &&
         (!FetchUtils::isCorsSafelistedMethod(m_orgProxy->method()) ||
          nwd->hasCorsUnsafeRequestHeaderNames))) {
        m_orgProxy->setResponseTainting(ResponseTainting::Cors);
        nwd->corsFlag = true;
        nwd->corsPreflightFlag = true;
    }

    if (m_orgProxy->isSameOriginRequest()) {
        nwd->corsFlag = false;
    }

    fillHeadersWithClientHeaders(headers);
    fillHeadersWithGeneralHeaders(headers);
    nwd->httpTransaction->setHTTPRequest(
        HTTPRequest::create(m_orgProxy->url()->urlString()->toUTF8NonGCString(),
                            m_orgProxy->url()->host()->toUTF8NonGCString(),
                            m_orgProxy->method()->toUTF8NonGCString(), headers,
                            body->toUTF8NonGCString(), includeCredentials));
    nwd->httpTransaction->httpRequest().setUnsafeRequestHeaderNames(
        unsafeHeaders);
    nwd->httpTransaction->setTimeout(
        static_cast<unsigned long>(m_orgProxy->m_timeout));
    nwd->httpTransaction->setProxyURL(m_orgProxy->webView()->proxyURL());
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
            Thread* t = new Thread(m_orgProxy->webView());
            t->run(m_orgProxy->webView()->messageLoop(),
                   [](void* data) -> void* {
                       NetworkURLWorkerData* d = (NetworkURLWorkerData*)data;
                       NetworkURLResourceRequestJobDelegate::worker(d);
                       return nullptr;
                   },
                   nwd);
        } else {
            nwd->helper = new AsyncNetworkWorkHelper();
            m_orgProxy->webView()->threadPool()->addWork(
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
    headers.append(HTTPHeaderMap::kConnection, "keep-alive");
}

void NetworkURLResourceRequestJobDelegate::fillHeadersWithClientHeaders(
    HTTPHeaderMap& headers)
{
    // Set Client Request header
    //  * Accept, Accept-Charset, Accept-Encoding, Accept-Language,
    //  * Authorization, Cookie, Expect, From, Host, If-Match,
    //  * Max-Forwards, Origin, Proxy-Authorization, Range, Referer, TE,
    //  * User-Agent ...

    auto it = headers.headerMap().find(HTTPHeaderMap::kAcceptLanguage);
    if (it == headers.headerMap().end()) {
        std::string tmpStr;
        tmpStr = m_orgProxy->webView()->locale().getName();
        std::replace(tmpStr.begin(), tmpStr.end(), '_', '-');
        tmpStr = tmpStr + " , en-US , en";
        headers.append(HTTPHeaderMap::kAcceptLanguage, tmpStr);
    }

    headers.append(HTTPHeaderMap::kUserAgent,
                   m_orgProxy->webView()->userAgent()->toUTF8NonGCString());

    headers.append(HTTPHeaderMap::kHost,
                   m_orgProxy->url()->host()->toUTF8NonGCString());

    if (!m_orgProxy->isSameOriginRequest()) {
        headers.append(HTTPHeaderMap::kOrigin, m_orgProxy->document()
                                                   ->webOrigin()
                                                   ->serialize()
                                                   ->toUTF8NonGCString());
    }

    auto it2 = headers.find(HTTPHeaderMap::kReferer);
    if (it2 == headers.headerMap().end() && m_orgProxy->referrer()) {
        ResourceURL* rUrl = m_orgProxy->referrer();
        String* rString;
        if (rUrl->isReferrerURL()) {
            rString = rUrl->asReferrerURL()->referrerString(m_orgProxy->url());
        } else {
            rString = rUrl->urlString();
        }
        if (!rString->isEmpty()) {
            headers.append(HTTPHeaderMap::kReferer,
                           rString->toUTF8NonGCString());
        }
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
        headers.append(HTTPHeaderMap::kIfModifiedSince, value);
    } else if (info.date) {
        std::string value =
            timeToUTCString(m_orgProxy->document()->scriptBindingInstance(),
                            info.date * 1000)
                ->toUTF8NonGCString();
        headers.append(HTTPHeaderMap::kIfModifiedSince, value);
    }

    if (info.etag.size()) {
        headers.append(HTTPHeaderMap::kIfNoneMatch, info.etag);
    }
}
#endif

#ifdef STARFISH_ENABLE_NETWORK_PROFILING
class NetworkProifileRAIILogger {
public:
    NetworkProifileRAIILogger(NetworkURLWorkerData* nwd)
        : nwd(nwd)
    {
        if (nwd->httpTransaction) {
            STARFISH_LOG_INFO(
                "[NETWORK_PROFILING] Start network request %s at %dms\n",
                nwd->httpTransaction->httpRequest().url().data(),
                (int)(timestamp() - g_profilingBaseTime));
        }
    }

    ~NetworkProifileRAIILogger()
    {
        if (nwd->httpTransaction) {
            STARFISH_LOG_INFO(
                "[NETWORK_PROFILING] End network request %s at %dms\n",
                nwd->httpTransaction->httpRequest().url().data(),
                (int)(timestamp() - g_profilingBaseTime));
        }
    }

    NetworkURLWorkerData* nwd;
};

#endif

void* NetworkURLResourceRequestJobDelegate::worker(void* data)
{
    NetworkURLWorkerData* nwd = (NetworkURLWorkerData*)data;
#ifdef STARFISH_ENABLE_NETWORK_PROFILING
    NetworkProifileRAIILogger logger(nwd);
#endif

    if (nwd->corsPreflightFlag) {
        ResourceRequest* request = nwd->request;
        Locker<Mutex> locker(*request->m_mutex);
        // https://fetch.spec.whatwg.org/#concept-http-fetch
        if ((!FetchUtils::isCorsSafelistedMethod(request->method()) ||
             request->useCorsPreflightFlag()) ||
            nwd->hasCorsUnsafeRequestHeaderNames) {
            nwd->needsToSendPreflightRequest = true;

            // https://fetch.spec.whatwg.org/#cors-preflight-fetch-0
            request->m_preflightRequestData->m_url =
                request->m_requestData->m_url;
            request->m_preflightRequestData->m_destination =
                request->m_requestData->m_destination;
            request->m_preflightRequestData->m_referrer =
                request->m_requestData->m_referrer;
        }
    }

#ifdef STARFISH_ENABLE_HTTPCACHE
    if (!nwd->cachedEntry || (nwd->cachedEntry->shouldReValidate())) {
        return networkWorker(data);
    } else {
        return httpCacheWorker(data);
    }
#else
    return networkWorker(data);
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
    if (nwd->isAborted) {
        return 0;
    }

    size_t realSize = size * nmemb;
    const char* memPtr = (const char*)ptr;

    if (request->isSync()) {
        auto& entityBody = request->response();
        entityBody.insert(entityBody.end(), memPtr, memPtr + realSize);
    } else {
        nwd->pendingResponseData.insert(nwd->pendingResponseData.end(), memPtr,
                                        memPtr + realSize);
    }

    if (request->m_pendingOnProgressEventIdlerHandle == SIZE_MAX) {
        if (request->isSync()) {
            request->changeReadyState(ReadyState::Loading, true);
            request->changeProgress(ProgressState::Progress, true);
            if (!request->checkProgressAllowanceWithContentSecurityPolicy()) {
                nwd->isAborted = true;
                nwd->needsToHandleError = true;
                request->m_responseData->m_status = 0;
                return 0;
            }
        } else {
            request->m_pendingOnProgressEventIdlerHandle =
                request->webView()->messageLoop()->addIdlerWithNoGCRootingInOtherThread(
                    nullptr,
                    [](size_t handle, void* data) {
                        NetworkURLWorkerData* nwd = (NetworkURLWorkerData*)data;
                        ResourceRequest* request = nwd->request;
                        Locker<Mutex> locker(*request->m_mutex);
                        {
                            STARFISH_ASSERT(
                                handle ==
                                request->m_pendingOnProgressEventIdlerHandle);
                            request->m_pendingOnProgressEventIdlerHandle =
                                SIZE_MAX;
                        }
                        if (!request->isSync()) {
                            request->response().insert(
                                request->response().end(),
                                nwd->pendingResponseData.begin(),
                                nwd->pendingResponseData.end());
                            nwd->pendingResponseData.clear();
                        }

                        request->changeReadyState(ReadyState::Loading, true);
                        request->changeProgress(ProgressState::Progress, true);

                        if (!request
                                 ->checkProgressAllowanceWithContentSecurityPolicy()) {
                            nwd->isAborted = true;
                            nwd->needsToHandleError = true;
                            request->m_responseData->m_status = 0;
                        }

                    },
                    nwd);
        }
    }

    return realSize;
}

// https://fetch.spec.whatwg.org/#cors-preflight-fetch-0
// Caution : Use in curlWriteHeaderCallback only
static bool checkCORSPreflight(NetworkURLWorkerData* nwd)
{
    // TODO: apply `Access-Control-Allow-Headers`,`Access-Control-Max-Age` and
    // CORS-preflight cache
    const auto request = nwd->request;
    const auto& resHeaders = nwd->httpTransaction->httpResponse().headers();

    std::vector<std::string> methods;
    bool ret1 = resHeaders.extractHeaderListValues(
        methods, HTTPHeaderMap::kAccessControlAllowMethods);

    std::vector<std::string> headerNames;
    bool ret2 = resHeaders.extractHeaderListValues(
        headerNames, HTTPHeaderMap::kAccessControlAllowHeaders);

    if (ret1 == false || ret2 == false) {
        return false;
    }
    std::string requestMethod = request->method()->toUTF8NonGCString();
    if (methods.size() == 0 && request->useCorsPreflightFlag()) {
        methods.push_back(requestMethod);
    }
    bool hasRequestMethod = false;
    bool hasWildCard = false;
    bool isCorsSafelistedMethod =
        StringUtils::equalsIgnoreCase("GET", requestMethod) ||
        StringUtils::equalsIgnoreCase("POST", requestMethod) ||
        StringUtils::equalsIgnoreCase("HEAD", requestMethod);

    const std::string whildCard = "*";
    for (const auto& method : methods) {
        if (StringUtils::equalsIgnoreCase(requestMethod, method)) {
            hasRequestMethod = true;
        }
        if (StringUtils::equalsIgnoreCase(whildCard, method)) {
            hasWildCard = true;
        }
    }
    if ((!hasRequestMethod && !isCorsSafelistedMethod) &&
        ((request->requestCredentials() == RequestCredentials::Include) ||
         !hasWildCard)) {
        STARFISH_LOG_WARN(
            "Failed to load %s : Request doesn't pass CORS Preflight request, "
            "please check %s header in reponse\n",
            nwd->httpTransaction->httpRequest().url().data(),
            HTTPHeaderMap::kAccessControlAllowMethods);
        return false;
    }

    auto unsafeNames =
        nwd->httpTransaction->httpRequest().unsafeRequestHeaderNames();

    bool hasUnsafeName = unsafeNames.size() == 0;
    for (const auto& unsafeName : unsafeNames) {
        hasUnsafeName = false;
        for (const auto& name : headerNames) {
            if (StringUtils::equalsIgnoreCase(unsafeName, name)) {
                hasUnsafeName = true;
                break;
            }
        }
        if (!hasUnsafeName) {
            break;
        }
    }

    bool hasWildCardHeaderValue = unsafeNames.size() == 0;
    for (const auto& name : headerNames) {
        if (StringUtils::equalsIgnoreCase(whildCard, name)) {
            hasWildCardHeaderValue = true;
            break;
        }
    }

    if (!hasUnsafeName &&
        ((request->requestCredentials() == RequestCredentials::Include) ||
         !hasWildCardHeaderValue)) {
        STARFISH_LOG_WARN(
            "Failed to load %s : Request doesn't pass CORS Preflight request, "
            "please check %s header in reponse\n",
            nwd->httpTransaction->httpRequest().url().data(),
            HTTPHeaderMap::kAccessControlAllowHeaders);
        return false;
    }

    return true;
}

// https://fetch.spec.whatwg.org/#concept-cors-check
// Caution : Use in curlWriteHeaderCallback only
static bool checkCors(NetworkURLWorkerData* nwd)
{
    const auto request = nwd->request;
    const auto& reqHeaders =
        nwd->httpTransaction->httpRequest().headers().headerMap();
    const auto& resHeaders =
        nwd->httpTransaction->httpResponse().headers().headerMap();

    auto origin = resHeaders.find(HTTPHeaderMap::kAccessControlAllowOrigin);
    if (origin == resHeaders.end()) {
        STARFISH_LOG_WARN(
            "Failed to load %s : request doesn't pass CORS check, please "
            "check %s header in reponse\n",
            nwd->httpTransaction->httpRequest().url().data(),
            HTTPHeaderMap::kAccessControlAllowOrigin);
        return false;
    }

    if (request->requestCredentials() != RequestCredentials::Include &&
        origin->second == "*") {
        return true;
    }

    // The origin in request header was serialized
    auto serializedRequestOrigin = reqHeaders.find(HTTPHeaderMap::kOrigin);
    if (serializedRequestOrigin == reqHeaders.end() ||
        serializedRequestOrigin->second != origin->second) {
        STARFISH_LOG_WARN(
            "Failed to load %s : request doesn't pass CORS check, please "
            "check %s header in reponse\n",
            nwd->httpTransaction->httpRequest().url().data(),
            HTTPHeaderMap::kAccessControlAllowOrigin);
        return false;
    }

    if (request->requestCredentials() != RequestCredentials::Include) {
        return true;
    }

    auto credentials =
        resHeaders.find(HTTPHeaderMap::kAccessControlAllowCredentials);
    if (credentials != resHeaders.end() && credentials->second == "true") {
        return true;
    }
    STARFISH_LOG_WARN(
        "Failed to load %s : request doesn't pass CORS check, please "
        "check %s header in reponse\n",
        nwd->httpTransaction->httpRequest().url().data(),
        HTTPHeaderMap::kAccessControlAllowCredentials);

    return false;
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

    request->m_responseData->m_status =
        nwd->httpTransaction->httpResponse().responseCode();

    if (nwd->httpTransaction->httpResponse().isSuccessfulResponseStatus()) {
        if ((rawHeader.compare("\r\n") == 0) ||
            (rawHeader.compare("\n") == 0)) {
            if (nwd->corsFlag &&
                !(request->isSubresourceRequest() ||
                  request->requestMode() == RequestMode::Navigate)) {
                bool allowed = true;
                if (checkCors(nwd)) {
                    if (nwd->needsToSendPreflightRequest) {
                        if (!checkCORSPreflight(nwd)) {
                            allowed = false;
                        }
                    }
                } else {
                    allowed = false;
                }
                if (!allowed) {
                    nwd->isAborted = true;
                    nwd->needsToHandleError = true;
                    request->m_responseData->m_status = 0;
                }
            }
            request->m_lastEffectiveURL =
                nwd->httpTransaction->httpResponse().lastEffectiveURL();
            auto& headers =
                request->m_responseHeaders->httpHeaderMap()->headerMap();
            // TODO : Refactor HTTPTransction using RequestData, ResponseData,
            // HeadersData
            headers = std::move(
                nwd->httpTransaction->httpResponse().headers().headerMap());

        } else {
            nwd->httpTransaction->didReceiveHeader(rawHeader);
        }
    } else if (nwd->httpTransaction->httpResponse()
                   .isRedirectionResponseStatus()) {
        request->m_responseData->m_redirected = true;
    }

    return realSize;
}
}
