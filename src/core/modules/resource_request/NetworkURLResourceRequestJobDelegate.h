/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishNetworkURLResourceRequestJobDelegate__
#define __StarfishNetworkURLResourceRequestJobDelegate__

#include "core/modules/resource_request/ResourceRequestJob.h"

namespace Starfish {

class ResourceRequest;
class NetworkURLWorkerHelper;
class HTTPCacheEntry;
class HTTPHeaderMap;
class HTTPTransaction;
class ServiceWorkerFetchTask;

struct CurlMultiRequestData;

struct NetworkURLWorkerData {
    NetworkURLWorkerData(ResourceRequest* orgRequest);
    ~NetworkURLWorkerData();

    std::atomic<bool> isAborted;
    bool needsToHandleError;
    long lastTransactionResponseCode;

    // FIXME : These flags should better be arguments of http-fetch
    // https://fetch.spec.whatwg.org/#http-fetch
    bool corsFlag;
    bool corsPreflightFlag;
    bool hasCorsUnsafeRequestHeaderNames;

    bool needsToSendPreflightRequest;
    bool inPreflightRequest;
    bool isPreflightReqeustDone;
    const char* readptrToUpload;
    size_t sizeleftToUpload;
    ResourceRequest* request;
    NetworkURLWorkerHelper* helper;
    CurlMultiRequestData* curlMultiRequestData;
    std::unique_ptr<HTTPTransaction> httpTransaction;
    std::vector<char> pendingResponseData;
#ifdef STARFISH_ENABLE_HTTPCACHE
    HTTPCacheEntry* cachedEntry;
#endif
    std::string lastEffectiveURL;
#ifdef STARFISH_ENABLE_NETWORK_PROFILING
    uint64_t workingTime;
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

    virtual void responseHandlerWrapper(NetworkURLWorkerData* nwd) = 0;
    virtual void abortHandlerWrapper(NetworkURLWorkerData* nwd) = 0;

protected:
    static void responseHandler(size_t, void* data);
    static void abortHandeler(size_t, void* data);
};

class AsyncNetworkWorkHelper : public NetworkURLWorkerHelper {
protected:
    virtual void responseHandlerWrapper(NetworkURLWorkerData* nwd) override;
    virtual void abortHandlerWrapper(NetworkURLWorkerData* nwd) override;
};

class SyncNetworkWorkHelper : public NetworkURLWorkerHelper {
protected:
    virtual void responseHandlerWrapper(NetworkURLWorkerData* nwd) override;
    virtual void abortHandlerWrapper(NetworkURLWorkerData* nwd) override;
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
    static size_t curlUploadBufferDataCallback(void* ptr, size_t size,
                                               size_t nmemb, void* data);

    NetworkURLResourceRequestJobDelegate(ResourceRequest* proxy);
    virtual void send(String* body = String::emptyString,
                      bool allowCache = false);

private:
    static void* networkWorker(void* data);
#ifdef STARFISH_ENABLE_HTTPCACHE
    static void* httpCacheWorker(void* data);
    void fillHeadersWithCachedEntry(HTTPHeaderMap& headers,
                                    HTTPCacheEntry* cachedEntry);
#endif
    void fillHeadersWithGeneralHeaders(HTTPHeaderMap& headers);
    void fillHeadersWithClientHeaders(HTTPHeaderMap& headers);

    ResourceRequest* m_orgProxy;
#if defined(STARFISH_ENABLE_SERVICE_WORKER) && \
    defined(STARFISH_WEBWORKER_NOT_HOST)
    ServiceWorkerFetchTask* m_serviceWorkerFetchTask;
#endif
};
} // namespace Starfish

#endif
