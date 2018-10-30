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

#ifndef __StarfishNetworkURLResourceRequestJobDelegate__
#define __StarfishNetworkURLResourceRequestJobDelegate__

#include "core/modules/resource_request/ResourceRequestJob.h"

namespace Starfish {

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

    void* networkWorker(void* data);
#ifdef STARFISH_ENABLE_HTTPCACHE
    void* httpCacheWorker(void* data);
#endif

protected:
    virtual void responseHandlerWrapper(int res, NetworkURLWorkerData* nwd)
    {
    }

    virtual void abortHandlerWrapper(int res, NetworkURLWorkerData* nwd)
    {
    }

    static void responseHandler(size_t handle, void* data);
    static void abortHandeler(size_t handle, void* data);
};

class AsyncNetworkWorkHelper : public NetworkURLWorkerHelper {
protected:
    virtual void responseHandlerWrapper(int res,
                                        NetworkURLWorkerData* nwd) override;
    virtual void abortHandlerWrapper(int res,
                                     NetworkURLWorkerData* nwd) override;
};

class SyncNetworkWorkHelper : public NetworkURLWorkerHelper {
protected:
    virtual void responseHandlerWrapper(int res, NetworkURLWorkerData* nwd);
    virtual void abortHandlerWrapper(int res,
                                     NetworkURLWorkerData* nwd) override;
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
