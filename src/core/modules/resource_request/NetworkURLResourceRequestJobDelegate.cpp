/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
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

#include "StarFish.h"
#include "core/dom/Document.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/resource_request/ResourceRequest.h"
#include "core/modules/resource_request/NetworkURLResourceRequestJobDelegate.h"
#include "core/modules/threading/ThreadPool.h"
#include "platform/network/http/HTTPHeaderList.h"
#include "platform/network/http/HTTPRequest.h"
#include "platform/network/http/HTTPTransaction.h"

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
                [](size_t, void* data) {
                    NetworkURLWorkerData* requestData =
                        (NetworkURLWorkerData*)data;
                    if (requestData ==
                        requestData->request->m_activeNetworkURLWorkerData) {
                        requestData->request->m_activeNetworkURLWorkerData =
                            nullptr;
                    }
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
        requestData->request->m_status =
            requestData->httpTransaction->responseCode();
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
        STARFISH_LOG_INFO("got timeout %s[%d]\n",
                          requestData->request->m_url->urlString()->utf8Data(),
                          (int)requestData->httpTransaction->responseCode());
        requestData->request->m_status =
            requestData->httpTransaction->responseCode();
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
        STARFISH_LOG_INFO("failed to open %s\n",
                          requestData->request->m_url->urlString()->utf8Data());
        requestData->request->m_status =
            requestData->httpTransaction->responseCode();
        requestData->request->handleError(ResourceRequest::ERROR);
    }

    requestData->request->m_activeNetworkURLWorkerData = nullptr;
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
            ->addIdlerWithNoGCRootingInOtherThread(this->responseHandler,
                                                   requestData);
}

NetworkURLResourceRequestJobDelegate::NetworkURLResourceRequestJobDelegate(
    ResourceRequest* proxy)
    : m_orgProxy(proxy)
{
}

void NetworkURLResourceRequestJobDelegate::send(String* body)
{
    STARFISH_ASSERT(m_orgProxy->m_url->isNetworkURL());
    NetworkURLWorkerData* data = new NetworkURLWorkerData();
    data->request = m_orgProxy;
    data->isAborted = false;
    m_orgProxy->m_activeNetworkURLWorkerData = data;

    String* method = String::emptyString;
    switch (m_orgProxy->m_method) {
    case ResourceRequest::GET_METHOD: {
        method = String::createASCIIString("GET");
        break;
    }
    case ResourceRequest::POST_METHOD: {
        method = String::createASCIIString("POST");
        break;
    }
    case ResourceRequest::UNKNOWN_METHOD: {
        STARFISH_ASSERT_NOT_REACHED();
        break;
    }
    default:
        STARFISH_ASSERT_NOT_REACHED();
    }

    HTTPHeaderList* headers = HTTPHeaderList::create();
    fillHeadersWithGeneralHeaders(headers);
    fillHeadersWithClientHeaders(headers);
    fillHeadersWithResourceRequestHeader(headers);

    HTTPRequest* httpRequest = HTTPRequest::create(
        m_orgProxy->m_url->urlString(), method, headers, body);

    HTTPTransaction* httpTransaction = HTTPTransaction::create(
        httpRequest, static_cast<unsigned long>(m_orgProxy->m_timeout));
    httpTransaction->setProgressCallbackAndData(curlProgressCallback, data);
    httpTransaction->setWriteHeaderCallbackAndData(curlWriteHeaderCallback,
                                                   data);
    httpTransaction->setWriteCallbackAndData(curlWriteCallback, m_orgProxy);

    data->httpTransaction = httpTransaction;

    if (m_orgProxy->isSync()) {
        data->networkWorker = new SyncNetworkWorkHelper();
        worker(data);
    } else {
        data->networkWorker = new AsyncNetworkWorkHelper();
        m_orgProxy->starFish()->threadPool()->addWork(
            NetworkURLResourceRequestJobDelegate::worker, data);
    }
}

void NetworkURLResourceRequestJobDelegate::fillHeadersWithGeneralHeaders(
    HTTPHeaderList* headers)
{
    STARFISH_ASSERT(headers);

    // Set General header
    //  * Cache-Control, Connection, Date, Pragma, Trailer, Transfer-Encoding,
    //  * Upgrade, Via, Warning ...
    headers->append("Connection:keep-alive");
}

void NetworkURLResourceRequestJobDelegate::fillHeadersWithClientHeaders(
    HTTPHeaderList* headers)
{
    STARFISH_ASSERT(headers);

    // Set Client Request header
    //  * Accept, Accept-Charset, Accept-Encoding, Accept-Language,
    //  * Authorization, Cookie, Expect, From, Host, If-Match,
    //  If-Modified-Since,
    //  * If-None-Match, If-Range, If-Unmodified-Since, Max-Forwards, Origin,
    //  * Proxy-Authorization, Range, Referer, TE, User-Agent ...

    std::string tmpStr;
    headers->append("Accept-Charset:utf-8");
    tmpStr = "Accept-Language:";
    tmpStr += m_orgProxy->starFish()->locale().getName();
    tmpStr.replace(tmpStr.begin(), tmpStr.end(), '_', '-');
    headers->append(tmpStr.data());
    headers->append("User-Agent: " USER_AGENT(APP_CODE_NAME, VERSION));
    if (!m_orgProxy->m_document->documentURI()->isNetworkURL()) {
        headers->append("Origin:null");
    } else {
        tmpStr = "Host:";
        tmpStr += m_orgProxy->m_url->hostname()->utf8Data();
        headers->append(tmpStr.data());
        tmpStr = "Referer:";
        tmpStr += m_orgProxy->m_document->urlString()->utf8Data();
        headers->append(tmpStr.data());
    }
}

void NetworkURLResourceRequestJobDelegate::fillHeadersWithResourceRequestHeader(
    HTTPHeaderList* headers)
{
    std::string tmpStr;
    for (size_t i = 0; i < m_orgProxy->m_requestHeaders.size(); i++) {
        tmpStr =
            std::string(m_orgProxy->m_requestHeaders[i].first->utf8Data()) +
            ":";
        tmpStr += m_orgProxy->m_requestHeaders[i].second->utf8Data();
        headers->append(tmpStr.data());
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
    ResourceRequest* request = (ResourceRequest*)data;
    Locker<Mutex> locker(*request->m_mutex);

    size_t realSize = size * nmemb;
    const char* memPtr = (const char*)ptr;

    request->m_response.insert(request->m_response.end(), memPtr,
                               memPtr + realSize);

    if (request->m_pendingOnProgressEventIdlerHandle == SIZE_MAX) {
        if (request->isSync()) {
            request->changeReadyState(ResourceRequest::LOADING, true);
            request->changeProgress(ResourceRequest::PROGRESS, true);
        } else {
            request->m_pendingOnProgressEventIdlerHandle =
                request->starFish()
                    ->messageLoop()
                    ->addIdlerWithNoGCRootingInOtherThread(
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
    size_t realsize = size * nmemb;
    NetworkURLWorkerData* request = (NetworkURLWorkerData*)data;
    Locker<Mutex> locker(*request->request->m_mutex);

    size_t realSize = size * nmemb;
    const char* memPtr = (const char*)ptr;

    long resCode = request->httpTransaction->responseCode();

    if (request->lastTransactionResponseCode != resCode) {
        request->lastTransactionResponseCode = resCode;
        request->request->m_responseHeaderData.clear();
    }

    request->request->m_responseHeaderData.insert(
        request->request->m_responseHeaderData.end(), memPtr,
        memPtr + realSize);
    return realsize;
}
}
