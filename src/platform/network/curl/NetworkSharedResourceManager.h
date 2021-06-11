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

#ifndef __StarfishNetworkSharedResourceManager__
#define __StarfishNetworkSharedResourceManager__

#include <curl/curl.h>

namespace Starfish {

class Document;
class HTTPTransaction;
class MessageLoop;
class Mutex;
class ResourceURL;
class String;
class Thread;

struct CurlHandleData {
    CURL* curl;
    uint64_t lastUsedTime;
};

struct CurlMultiRequestData : public gc {
    CURL* m_curl;
    Mutex* m_mutex;
    CURLcode m_result;

    CurlMultiRequestData(Mutex* mutex)
        : m_curl(nullptr)
        , m_mutex(mutex)
        , m_result(CURLcode::CURLE_OK)
    {
    }
};

class NetworkSharedResourceManager {
    friend class HTTPTransaction;

public:
    typedef std::multimap<std::string, CurlHandleData> CurlHandleDataMultiMap;
    static NetworkSharedResourceManager* getInstance();
    static void destroy();

    CURLSH* curlShareHandle() const;          // Do not free
    CURLSH* curlNonCookieShareHandle() const; // Do not free
    std::string cookieStoreFilePath() const;
    void setCookieStoreFilePath(const std::string& name);
    Mutex* resourceMutex(curl_lock_data data);

    CurlHandleData getCurlHandleData(const std::string& host);
    void cachingCurlHandleData(const std::string& host, CurlHandleData& cd);
    void clearAllCurlHandleDataCache();
    void pruningIfNeed();
    void initCookieSession();

    // for document.cookie
    String* cookies(ResourceURL* url);
    void setCookies(ExecutionContext* executionContext, ResourceURL* url,
                    String* value);
    void clearCookies();
    bool hasCookies();

    size_t cacheClearTimerID()
    {
        return m_cacheClearTimerID;
    }

    void setCacheClearTimerID(size_t timerID)
    {
        m_cacheClearTimerID = timerID;
    }

    void startMultiRequestThreadIfNeeds(MessageLoop* ml,
                                        const std::string& origin);
    void appendPendingMultiRequest(const std::string& origin,
                                   CurlMultiRequestData* r);

private:
    NetworkSharedResourceManager();
    ~NetworkSharedResourceManager();

    void initMutexes();

    CURLSH* m_curlShareHandle;
    CURLSH* m_curlNonCookieShareHandle;

    struct CurlMultiData : public gc {
        MessageLoop* m_ml;
        CURLM* m_curlMultiHandle;
        Thread* m_thread;
        Mutex* m_globalDataMutex;
        std::atomic<bool> m_running;
        std::atomic<bool> m_finishing;
        std::vector<CurlMultiRequestData*> m_pendingRequests;

        CurlMultiData(MessageLoop* ml, Mutex* curlMultiRequestDataMutex);
    };
    typedef std::unordered_map<std::string, CurlMultiData*>
        CurlMultiRequestDataMap;
    CurlMultiRequestDataMap m_curlMultiRequestData;
    Mutex* m_curlMultiRequestDataMutex;
    static void* curlMultiWorker(void* data);

    CurlHandleDataMultiMap m_curlHandleDataCache;
    uint64_t m_lastCachePruneTime;
    size_t m_cacheClearTimerID;
    std::string m_cookieStoreFilePath;
};
} // namespace Starfish

#endif
