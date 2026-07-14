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

    // Cookies are kept in an engine-owned master store instead of a CURLSH
    // cookie share: libcurl builds the outgoing Cookie header from shared
    // list nodes after releasing the share lock (fixed upstream only in
    // 8.16.0, c278c508e2), so concurrent transfers on the shared store
    // corrupt it. Each credentialed transfer runs a private cookie engine
    // seeded from the master store and merged back after the transfer.
    //
    // Enables the transfer's private cookie engine and seeds it with a
    // snapshot of the master store. Returns the injected snapshot; pass it
    // to mergeTransferCookies (which frees it) after the transfer.
    struct curl_slist* setupPrivateCookieEngine(CURL* curl);
    void mergeTransferCookies(CURL* curl, struct curl_slist* injected);

    // Netscape-format lines of every cookie in the master store.
    // Caller frees with curl_slist_free_all.
    struct curl_slist* allCookies();
    // Adds/replaces one Netscape-format cookie line in the master store.
    void addCookieLine(const char* line);

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

    // Master cookie store. A CURL easy handle that never performs transfers
    // and is never attached to a share; it only hosts the process-wide
    // cookie engine state. Guarded by resourceMutex(CURL_LOCK_DATA_COOKIE).
    CURL* masterCookieHandleLocked();
    void flushMasterCookiesLocked();

    CurlHandleDataMultiMap m_curlHandleDataCache;
    uint64_t m_lastCachePruneTime;
    size_t m_cacheClearTimerID;
    std::string m_cookieStoreFilePath;
    CURL* m_masterCookieHandle;
};
} // namespace Starfish

#endif
