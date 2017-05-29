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
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/resource_request/ResourceRequest.h"
#include "core/modules/resource_request/NetworkURLWorkerHelper.h"
#include "core/modules/threading/ThreadPool.h"

namespace StarFish {

void* NetworkURLWorkerHelper::networkWorker(void* data)
{
    NetworkURLWorkerData* requestData = (NetworkURLWorkerData*)data;
    CURL* curl = requestData->curl;
    curl_slist* list = requestData->headerList;
    auto res = curl_easy_perform(curl);
    long code;
    curl_easy_getinfo(requestData->curl, CURLINFO_RESPONSE_CODE, &code);
    requestData->responseCode = code;
    requestData->res = res;

    if (requestData->res != CURLE_ABORTED_BY_CALLBACK) {
        responseHandlerWrapper(res, requestData);
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

    curl_slist_free_all(list);
    curl_easy_cleanup(curl);
    return NULL;
}

void NetworkURLWorkerHelper::responseHandler(size_t handle, void* data)
{
    NetworkURLWorkerData* requestData = (NetworkURLWorkerData*)data;
    STARFISH_ASSERT(isMainThread());
    STARFISH_ASSERT(requestData->res != CURLE_ABORTED_BY_CALLBACK);
    if (requestData->isAborted) {
    } else if (requestData->res == 0) {
        if (!requestData->isSync) {
            Locker<Mutex> locker(*requestData->request->m_mutex);
            requestData->request->m_pendingNetworkWorkerEndIdlerHandle =
                SIZE_MAX;
        }
        STARFISH_ASSERT(
            requestData->request->m_pendingNetworkWorkerEndIdlerHandle ==
            SIZE_MAX);
        requestData->request->m_status = requestData->responseCode;
        requestData->request->handleResponseEOF();
    } else if (requestData->res == CURLE_OPERATION_TIMEDOUT) {
        if (!requestData->isSync) {
            Locker<Mutex> locker(*requestData->request->m_mutex);
            requestData->request->m_pendingNetworkWorkerEndIdlerHandle =
                SIZE_MAX;
        }
        STARFISH_ASSERT(
            requestData->request->m_pendingNetworkWorkerEndIdlerHandle ==
            SIZE_MAX);
        STARFISH_LOG_INFO("got timeout %s[%d]\n",
                          requestData->request->m_url->urlString()->utf8Data(),
                          (int)requestData->responseCode);
        requestData->request->m_status = requestData->responseCode;
        requestData->request->handleError(ResourceRequest::TIMEOUT);
    } else {
        if (!requestData->isSync) {
            Locker<Mutex> locker(*requestData->request->m_mutex);
            requestData->request->m_pendingNetworkWorkerEndIdlerHandle =
                SIZE_MAX;
        }
        STARFISH_ASSERT(
            requestData->request->m_pendingNetworkWorkerEndIdlerHandle ==
            SIZE_MAX);
        STARFISH_LOG_INFO("failed to open %s\n",
                          requestData->request->m_url->urlString()->utf8Data());
        requestData->request->m_status = requestData->responseCode;
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
}
