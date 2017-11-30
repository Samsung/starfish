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

#ifndef __StarFishNetworkURLResourceRequestJobDelegate__
#define __StarFishNetworkURLResourceRequestJobDelegate__

#include "core/modules/resource_request/ResourceRequestJob.h"

namespace StarFish {

class ResourceRequest;
class NetworkURLWorkerHelper;
class HTTPCacheEntry;
class HTTPHeaderMap;
class HTTPTransaction;

struct NetworkURLWorkerData {
    NetworkURLWorkerData(ResourceRequest* orgRequest);
    ~NetworkURLWorkerData();

    bool isAborted;
    bool isRedirected;
    long lastTransactionResponseCode;
    ResourceRequest* request;
    NetworkURLWorkerHelper* helper;
    std::unique_ptr<HTTPTransaction> httpTransaction;
#ifdef STARFISH_ENABLE_HTTPCACHE
    HTTPCacheEntry* cachedEntry;
#endif
    std::string lastLocation;
#ifdef STARFISH_ENABLE_PROFILING
    uint64_t start;
    bool cachehit;
    static int64_t reqCnt;
    static int64_t hitCnt;
#endif
};

class NetworkURLWorkerHelper : public gc {
public:
    NetworkURLWorkerHelper()
    {
    }

    virtual ~NetworkURLWorkerHelper()
    {
    }

    void* networkWorker(void* data);
#ifdef STARFISH_ENABLE_HTTPCACHE
    void* httpCacheWorker(void* data);
#endif
    void workerAbortHandeler(void* data);

protected:
    virtual void responseHandlerWrapper(int res,
                                        NetworkURLWorkerData* requestData)
    {
    }
    static void responseHandler(size_t handle, void* requestData);
};

class AsyncNetworkWorkHelper : public NetworkURLWorkerHelper {
protected:
    virtual void responseHandlerWrapper(int res,
                                        NetworkURLWorkerData* requestData);
};

class SyncNetworkWorkHelper : public NetworkURLWorkerHelper {
protected:
    virtual void responseHandlerWrapper(int res,
                                        NetworkURLWorkerData* requestData);
};

class NetworkURLResourceRequestJobDelegate
    : public gc,
      public ResourceRequestJobInterface {
public:
    static void* worker(void* data);
    static int curlProgressCallback(void* clientp, curl_off_t dltotal,
                                    curl_off_t dlnow, curl_off_t ultotal,
                                    curl_off_t ulnow);
    static size_t curlWriteCallback(void* ptr, size_t size, size_t nmemb,
                                    void* data);
    static size_t curlWriteHeaderCallback(void* ptr, size_t size, size_t nmemb,
                                          void* data);
    NetworkURLResourceRequestJobDelegate(ResourceRequest* proxy);
    virtual void send(String* body = String::emptyString,
                      bool allowCache = false);

private:
    void fillHeadersWithGeneralHeaders(HTTPHeaderMap& headers);
    void fillHeadersWithClientHeaders(HTTPHeaderMap& headers);
    void fillHeadersWithResourceRequestHeader(HTTPHeaderMap& headers);
#ifdef STARFISH_ENABLE_HTTPCACHE
    void fillHeadersWithCachedEntry(HTTPHeaderMap& headers,
                                    HTTPCacheEntry* cachedEntry);
#endif
    ResourceRequest* m_orgProxy;
};
}

#endif
