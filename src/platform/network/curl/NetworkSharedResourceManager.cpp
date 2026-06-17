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

#include "StarfishConfig.h"
#include "core/dom/ExecutionContext.h"
#include "core/modules/profiling/Profiling.h"
#include "binding/ScriptWrappable.h"
#include "NetworkSharedResourceManager.h"
#include "core/modules/threading/Locker.h"
#include "core/modules/threading/Mutex.h"
#include "core/modules/threading/Thread.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "platform/loader/ResourceURL.h"

#if !defined(OS_WINDOWS)
#include <unistd.h>
#endif

#if !(defined(OS_WINDOWS) || defined(STARFISH_ANDROID))
#include <openssl/crypto.h>
#endif

#define CURLHANDLE_CACHE_PRUNE_MINIMUM_SIZE 12
#define CURLHANDLE_CACHE_PRUNE_MINIMUM_INTERVAL_S 0.5
#define CURLHANDLE_CACHE_IDLE_TIME_LIMIT_S 0.25

namespace Starfish {

#ifdef STARFISH_ENABLE_TEST
static void dumpCookies(CURL* curl, const char* message)
{
    CURLcode res;
    struct curl_slist* cookies;
    struct curl_slist* nc;
    int i;
    STARFISH_LOG_INFO("=========Dump cookie : %s=========", message);
    STARFISH_LOG_INFO("Cookies, curl knows:");
    res = curl_easy_getinfo(curl, CURLINFO_COOKIELIST, &cookies);
    if (res != CURLE_OK) {
        STARFISH_LOG_INFO("Curl curl_easy_getinfo failed: %s",
                          curl_easy_strerror(res));
        STARFISH_LOG_INFO("===================================");
        return;
    }
    nc = cookies;
    i = 1;
    while (nc) {
        STARFISH_LOG_INFO("[%d]: %s", i, nc->data);
        nc = nc->next;
        i++;
    }
    if (i == 1) {
        STARFISH_LOG_INFO("(none)");
    }
    curl_slist_free_all(cookies);
    STARFISH_LOG_INFO("===================================");
}
#endif

#if !(defined(OS_WINDOWS) || defined(STARFISH_ANDROID))
static pthread_mutex_t* sslLockarray;

static void sslLockCallback(int mode, int type, const char* file, int line)
{
    (void)file;
    (void)line;
    if (mode & CRYPTO_LOCK) {
        pthread_mutex_lock(&(sslLockarray[type]));
    } else {
        pthread_mutex_unlock(&(sslLockarray[type]));
    }
}

static unsigned long getThreadID(void)
{
    return (unsigned long)pthread_self();
}

static void initSSLLocks(void)
{
    sslLockarray = (pthread_mutex_t*)OPENSSL_malloc(CRYPTO_num_locks() *
                                                    sizeof(pthread_mutex_t));
    for (int i = 0; i < CRYPTO_num_locks(); i++) {
        pthread_mutex_init(&(sslLockarray[i]), NULL);
    }

    CRYPTO_set_id_callback(getThreadID);
    CRYPTO_set_locking_callback(sslLockCallback);
}

static void removeSSLLocks(void)
{
    int i;
    CRYPTO_set_locking_callback(NULL);
    CRYPTO_set_id_callback(NULL);

    for (i = 0; i < CRYPTO_num_locks(); i++)
        pthread_mutex_destroy(&(sslLockarray[i]));

    OPENSSL_free(sslLockarray);
}
#endif
static NetworkSharedResourceManager* g_networkSharedResourceMangerInstance =
    nullptr;
static Mutex* g_mutexes[curl_lock_data::CURL_LOCK_DATA_LAST + 1];

static bool domainMatch(String* cookieDomain, String* host)
{
    size_t index = host->find(cookieDomain);
    bool flag =
        (index != SIZE_MAX && index + cookieDomain->length() == host->length());

    if (flag && !index) {
        return true;
    }

    if (flag && index > 0 && host->charAt(index) == '.') {
        return true;
    }

    if (cookieDomain->charAt(0) == '.' && cookieDomain->find(host) == 1) {
        return true;
    }
    return false;
}

static void appendMatchingCookie(String* cookie, String* domain, String* path,
                                 StringBuilder& cookiesBuilder)
{
    // Netscape cookie file format
    // * domain\tflag\tpath\tsecure\texpiration\tname\tvalue
    if (!cookie) {
        return;
    }

    GCVector<StringView> tokens;
    StringUtils::tokenize(cookie, "\t", 1, tokens);
    STARFISH_ASSERT(tokens.size() == 7);

    if (tokens[0].startsWith("#HttpOnly_")) {
        return;
    }
    if (!domainMatch(&tokens[0], domain)) {
        return;
    }
    GCVector<StringView> pathTokens;
    StringUtils::tokenize(path, "/", 1, pathTokens);
    GCVector<StringView> pathTokensFromCookie;
    StringUtils::tokenize(tokens[2].substring(), "/", 1, pathTokensFromCookie);
    if (pathTokens.size() < pathTokensFromCookie.size()) {
        return;
    }

    for (size_t i = 0;
         i < pathTokensFromCookie.size() && pathTokensFromCookie.size() != 1;
         ++i) {
        String* p1 = pathTokens[i].substring();
        String* p2 = pathTokensFromCookie[i].substring();
        if (i == pathTokensFromCookie.size() - 1 && p2->isEmpty()) {
            break;
        }
        if (!p1->equals(p2)) {
            return;
        }
    }

    time_t now = 0;
    time(&now);
    // Use int64_t to explicitly specify the width of bits.
    int64_t expires = String::parseInt64(&tokens[4]);
    if (expires && now > expires) {
        return;
    }
    if (cookiesBuilder.contentLength() > 0) {
        cookiesBuilder.appendString("; ");
    }
    cookiesBuilder.appendString(new StringView(tokens[5]));
    cookiesBuilder.appendString("=");
    cookiesBuilder.appendString(new StringView(tokens[6]));
    return;
}

static String* transformetoNetscapeCookieFormat(
    ExecutionContext* executionContext, ResourceURL* url, String* value)
{
    if (value->isEmpty()) {
        return String::emptyString;
    }
    if (!value->containsOnlyASCIIChars()) {
        return String::emptyString;
    }
    // RFC6265: cookie-octet excludes control characters. A cookie-string that
    // contains a control character (e.g. NUL) is not a valid cookie and must be
    // ignored rather than silently truncated at the control character.
    for (size_t i = 0; i < value->length(); ++i) {
        const char32_t c = value->charAt(i);
        if (c <= 0x1F || c == 0x7F) {
            return String::emptyString;
        }
    }
    GCVector<StringView> tokens;
    StringUtils::tokenize(value, ";", 1, tokens);
    String* cookieName = String::emptyString;
    String* cookieValue = String::emptyString;

    // First attr should be 'cookiename=value'
    if (tokens[0].contains("=")) {
        GCVector<StringView> pair;
        StringUtils::tokenize(&tokens[0], "=", 1, pair);
        cookieName = new StringView(pair[0]);
        cookieValue = new StringView(pair[1]);
    } else {
        // According to RFC6265, it should be ignored
        // but modern browsers appear to treat this as <cookiename>=<empty>"
        cookieName = new StringView(tokens[0]);
    }
    int64_t expires = 0;
    bool hasExpiry = false;
    String* domain = url->hostname();
    String* path = url->pathname();
    size_t idx = path->lastIndexOf('/');
    if (idx != SIZE_MAX) {
        path = path->substring(0, idx);
    }
    const char* secure = "FALSE";

    for (size_t i = 1; i < tokens.size(); ++i) {
        if (tokens[i].contains("=")) {
            GCVector<StringView> pair;
            StringUtils::tokenize(&tokens[i], "=", 1, pair);
            String* key = pair[0].trim()->toASCIILower();
            if (key->equals("expires")) {
                String* value = pair[1].trim();
                double parsedDate =
                    parseDate(executionContext->scriptBindingInstance(), value);
                // RFC6265 say : If the attribute-value failed to parse as a
                //               cookie date, ignore it
                if (!std::isnan(parsedDate)) {
                    expires = parsedDate / 1000.0;
                    hasExpiry = true;
                }
            } else if (key->equals("max-age")) {
                String* value = pair[1].trim();
                int64_t parsedValue = String::parseInt64(value);
                time_t current = time(0);
                if (parsedValue > 0 &&
                    (USER_AGENT_MAXIMUM_DATE_VALUE - current) >= parsedValue) {
                    expires = current + parsedValue;
                    hasExpiry = true;
                }
            } else if (key->equals("domain")) {
                String* value = (new StringView(pair[1]))->trim();
                domain = value;
            } else if (key->equals("path")) {
                String* value = (new StringView(pair[1]))->trim();
                path = value;
            }
        } else {
            String* key = tokens[i].trim()->toASCIILower();
            if (key->equals("secure")) {
                secure = "TRUE";
            }
        }
    }
    // A cookie with an explicit expiry in the past must be deleted, not kept.
    // The Netscape format reserves an expiry of 0 for session cookies, so a
    // past/epoch expiry is clamped to a non-zero past timestamp; this makes the
    // cookie expired on both write and read (see appendMatchingCookie) instead
    // of being misinterpreted as a never-expiring session cookie.
    if (hasExpiry && expires <= 0) {
        expires = 1;
    }
    const char* allowSubDomain = domain->startsWith(".") ? "TRUE" : "FALSE";
    String* expiresStr = String::fromInt64(expires);
    StringBuilder builder;

    builder.appendString(domain);
    builder.appendString("\t");
    STARFISH_ASSERT(allowSubDomain != nullptr);
    builder.appendString(allowSubDomain, strlen(allowSubDomain));
    builder.appendString("\t");
    builder.appendString(path);
    builder.appendString("\t");
    STARFISH_ASSERT(secure != nullptr);
    builder.appendString(secure, strlen(secure));
    builder.appendString("\t");
    builder.appendString(expiresStr);
    builder.appendString("\t");
    builder.appendString(cookieName);
    builder.appendString("\t");
    builder.appendString(cookieValue);

    return builder.finalize();
}

static void curlLockCallback(CURL* handle, curl_lock_data data,
                             curl_lock_access access, void* userPtr)
{
    NetworkSharedResourceManager* manager =
        static_cast<NetworkSharedResourceManager*>(userPtr);
    manager->resourceMutex(data)->lock();
}

static void curlUnlockCallback(CURL* handle, curl_lock_data data, void* userPtr)
{
    NetworkSharedResourceManager* manager =
        static_cast<NetworkSharedResourceManager*>(userPtr);
    manager->resourceMutex(data)->unlock();
}

NetworkSharedResourceManager* NetworkSharedResourceManager::getInstance()
{
    if (!g_networkSharedResourceMangerInstance) {
        g_networkSharedResourceMangerInstance =
            new NetworkSharedResourceManager();
    }
    return g_networkSharedResourceMangerInstance;
}

void NetworkSharedResourceManager::destroy()
{
    STARFISH_ASSERT(g_networkSharedResourceMangerInstance);
    STARFISH_LOG_INFO("NetworkSharedResourceManager::destroy()");
    delete g_networkSharedResourceMangerInstance;
    g_networkSharedResourceMangerInstance = nullptr;
}

NetworkSharedResourceManager::NetworkSharedResourceManager()
    : m_curlShareHandle(nullptr)
    , m_curlNonCookieShareHandle(nullptr)
    , m_curlHandleDataCache()
    , m_lastCachePruneTime(0)
    , m_cacheClearTimerID(SIZE_MAX)
    , m_cookieStoreFilePath("")
{
    initMutexes();
#if !(defined(OS_WINDOWS) || defined(STARFISH_ANDROID))
    initSSLLocks();
#endif

    curl_global_init(CURL_GLOBAL_ALL);
    m_curlShareHandle = curl_share_init();
    curl_share_setopt(m_curlShareHandle, CURLSHOPT_SHARE,
                      CURL_LOCK_DATA_COOKIE);
    curl_share_setopt(m_curlShareHandle, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS);
    curl_share_setopt(m_curlShareHandle, CURLSHOPT_SHARE,
                      CURL_LOCK_DATA_SSL_SESSION);
    curl_share_setopt(m_curlShareHandle, CURLSHOPT_LOCKFUNC, curlLockCallback);
    curl_share_setopt(m_curlShareHandle, CURLSHOPT_UNLOCKFUNC,
                      curlUnlockCallback);
    curl_share_setopt(m_curlShareHandle, CURLSHOPT_USERDATA, this);

    m_curlNonCookieShareHandle = curl_share_init();
    curl_share_setopt(m_curlNonCookieShareHandle, CURLSHOPT_SHARE,
                      CURL_LOCK_DATA_DNS);
    curl_share_setopt(m_curlNonCookieShareHandle, CURLSHOPT_SHARE,
                      CURL_LOCK_DATA_SSL_SESSION);
    curl_share_setopt(m_curlNonCookieShareHandle, CURLSHOPT_LOCKFUNC,
                      curlLockCallback);
    curl_share_setopt(m_curlNonCookieShareHandle, CURLSHOPT_UNLOCKFUNC,
                      curlUnlockCallback);
    curl_share_setopt(m_curlNonCookieShareHandle, CURLSHOPT_USERDATA, this);
}

NetworkSharedResourceManager::~NetworkSharedResourceManager()
{
    CurlMultiRequestDataMap multiData;
    {
        Locker<Mutex> l(*m_curlMultiRequestDataMutex);
        multiData = std::move(m_curlMultiRequestData);
    }
    auto iter = multiData.begin();
    while (iter != multiData.end()) {
        auto d = iter->second;
        d->m_running = false;
        d->m_thread->joinIfNeeds();
        delete d;
        iter++;
    }

    multiData.clear();

    clearAllCurlHandleDataCache();
    curl_share_cleanup(m_curlShareHandle);
    curl_share_cleanup(m_curlNonCookieShareHandle);
    curl_global_cleanup();

    for (int i = 0; i < curl_lock_data::CURL_LOCK_DATA_LAST + 1; ++i) {
        if (g_mutexes[i]) {
            delete g_mutexes[i];
            g_mutexes[i] = nullptr;
        }
    }

#if !(defined(OS_WINDOWS) || defined(STARFISH_ANDROID))
    removeSSLLocks();
#endif
}

void NetworkSharedResourceManager::initMutexes()
{
    for (int i = 0; i < curl_lock_data::CURL_LOCK_DATA_LAST + 1; ++i) {
        if (g_mutexes[i] == nullptr) {
            g_mutexes[i] = new (NoGC) Mutex();
        }
    }

    m_curlMultiRequestDataMutex = new (NoGC) Mutex();
}

CURLSH* NetworkSharedResourceManager::curlShareHandle() const
{
    return m_curlShareHandle;
}

CURLSH* NetworkSharedResourceManager::curlNonCookieShareHandle() const
{
    return m_curlNonCookieShareHandle;
}

std::string NetworkSharedResourceManager::cookieStoreFilePath() const
{
    return m_cookieStoreFilePath;
}

void NetworkSharedResourceManager::setCookieStoreFilePath(
    const std::string& name)
{
    m_cookieStoreFilePath = name;
}

void NetworkSharedResourceManager::initCookieSession()
{
    CURL* curl = curl_easy_init();

    if (!curl) {
        STARFISH_ASSERT_NOT_REACHED();
        return;
    }

    curl_easy_setopt(curl, CURLOPT_SHARE, m_curlShareHandle);
    if (m_cookieStoreFilePath.compare("") != 0) {
        curl_easy_setopt(curl, CURLOPT_COOKIEFILE,
                         m_cookieStoreFilePath.data());
        curl_easy_setopt(curl, CURLOPT_COOKIEJAR, m_cookieStoreFilePath.data());
    }
    curl_easy_setopt(curl, CURLOPT_COOKIESESSION, 1);
    curl_easy_cleanup(curl);
}

Mutex* NetworkSharedResourceManager::resourceMutex(curl_lock_data data)
{
    return g_mutexes[data];
}

CurlHandleData NetworkSharedResourceManager::getCurlHandleData(
    const std::string& host)
{
    Locker<Mutex> locker(*g_mutexes[curl_lock_data::CURL_LOCK_DATA_LAST]);
    CurlHandleData ret = { nullptr, 0 };
    auto iter = m_curlHandleDataCache.find(host);

    if (iter != m_curlHandleDataCache.end()) {
        // cache hit
        ret = iter->second;
        m_curlHandleDataCache.erase(iter);
        curl_easy_reset(ret.curl);
    } else {
        ret.curl = curl_easy_init();
    }
    STARFISH_ASSERT(ret.curl);
    return ret;
}

void NetworkSharedResourceManager::cachingCurlHandleData(
    const std::string& host, CurlHandleData& cd)
{
    Locker<Mutex> locker(*g_mutexes[curl_lock_data::CURL_LOCK_DATA_LAST]);
    pruningIfNeed();
    cd.lastUsedTime = tickCount();

    m_curlHandleDataCache.insert(
        std::pair<std::string, CurlHandleData>(host, cd));
}

void NetworkSharedResourceManager::pruningIfNeed()
{
    if ((m_curlHandleDataCache.size() > CURLHANDLE_CACHE_PRUNE_MINIMUM_SIZE) ||
        (tickCount() - m_lastCachePruneTime) >
            (CURLHANDLE_CACHE_PRUNE_MINIMUM_INTERVAL_S * 1000)) {
        uint64_t current = tickCount();
        auto iter = m_curlHandleDataCache.begin();
#ifdef STARFISH_ENABLE_TEST
        size_t old = m_curlHandleDataCache.size();
#endif
        while (iter != m_curlHandleDataCache.end()) {
            if ((current - iter->second.lastUsedTime) >
                (CURLHANDLE_CACHE_IDLE_TIME_LIMIT_S * 1000)) {
                curl_easy_cleanup(iter->second.curl);
                iter = m_curlHandleDataCache.erase(iter);
            } else {
                iter++;
            }
        }
#ifdef STARFISH_ENABLE_TEST
        STARFISH_LOG_INFO("prunning cached handles %zd => %zd ", old,
                          m_curlHandleDataCache.size());
#endif
        m_lastCachePruneTime = tickCount();
    }
}

void NetworkSharedResourceManager::clearAllCurlHandleDataCache()
{
#ifdef STARFISH_ENABLE_TEST
    STARFISH_LOG_INFO(
        "NetworkSharedResourceManager::clearAllCurlHandleDataCache(size:%d)",
        (int)m_curlHandleDataCache.size());
#endif
    Locker<Mutex> locker(*g_mutexes[curl_lock_data::CURL_LOCK_DATA_LAST]);
    while (m_curlHandleDataCache.size()) {
        CurlHandleData cd = m_curlHandleDataCache.begin()->second;
        curl_easy_cleanup(cd.curl);
        m_curlHandleDataCache.erase(m_curlHandleDataCache.begin());
    }
}

String* NetworkSharedResourceManager::cookies(ResourceURL* url)
{
    String* cookies = String::emptyString;
    CURL* curl = curl_easy_init();

    if (!curl) {
        return cookies;
    }
    curl_easy_setopt(curl, CURLOPT_SHARE, m_curlShareHandle);

    struct curl_slist* cookieList = nullptr;
    curl_easy_getinfo(curl, CURLINFO_COOKIELIST, &cookieList);

    if (cookieList) {
        String* domain = url->hostname();
        String* path = url->pathname();
        StringBuilder cookiesBuilder;
        for (struct curl_slist* p = cookieList; p; p = p->next) {
            STARFISH_ASSERT(p->data != nullptr);
            String* cookie = String::fromUTF8(p->data, strlen(p->data));
            appendMatchingCookie(cookie, domain, path, cookiesBuilder);
        }
        cookies = cookiesBuilder.finalize();
        curl_slist_free_all(cookieList);
    }
    curl_easy_cleanup(curl);
    return cookies;
}

bool NetworkSharedResourceManager::hasCookies()
{
    CURL* curl = curl_easy_init();

    if (!curl) {
        return false;
    }
    curl_easy_setopt(curl, CURLOPT_SHARE, m_curlShareHandle);

    struct curl_slist* cookieList = nullptr;
    curl_easy_getinfo(curl, CURLINFO_COOKIELIST, &cookieList);

    bool hasCookies = false;
    if (cookieList) {
        hasCookies = true;
        curl_slist_free_all(cookieList);
    }
    curl_easy_cleanup(curl);
    return hasCookies;
}

void NetworkSharedResourceManager::setCookies(
    ExecutionContext* executionContext, ResourceURL* url, String* value)
{
    CURL* curl = curl_easy_init();

    if (!curl) {
        return;
    }
    curl_easy_setopt(curl, CURLOPT_SHARE, m_curlShareHandle);
    if (m_cookieStoreFilePath.compare("") != 0) {
        curl_easy_setopt(curl, CURLOPT_COOKIEJAR, m_cookieStoreFilePath.data());
    }
#ifdef STARFISH_ENABLE_TEST
// dumpCookies(curl, "Before setCookie");
#endif
    String* cookie =
        transformetoNetscapeCookieFormat(executionContext, url, value);
    STARFISH_ASSERT(cookie->containsOnlyASCIIChars());
    STARFISH_ASSERT(cookie->bufferAccessData().isNullTerminated);
    curl_easy_setopt(curl, CURLOPT_COOKIELIST,
                     cookie->bufferAccessData().asciiData());
#ifdef STARFISH_ENABLE_TEST
// dumpCookies(curl, "Affter setCookie");
#endif
    curl_easy_cleanup(curl);
}

void NetworkSharedResourceManager::clearCookies()
{
    CURL* curl = curl_easy_init();

    if (!curl) {
        return;
    }

    curl_easy_setopt(curl, CURLOPT_SHARE, m_curlShareHandle);
    curl_easy_setopt(curl, CURLOPT_COOKIELIST, "ALL");
    curl_easy_cleanup(curl);
}

void* NetworkSharedResourceManager::curlMultiWorker(void* data)
{
    CurlMultiData* d = (CurlMultiData*)data;
    std::vector<Mutex*> remainedRequest;

    CURLM* curlMultiHandle = curl_multi_init();
#ifndef CURLPIPE_MULTIPLEX
#define CURLPIPE_MULTIPLEX 0
#endif
    curl_multi_setopt(curlMultiHandle, CURLMOPT_PIPELINING, CURLPIPE_MULTIPLEX);
    curl_multi_setopt(curlMultiHandle, CURLMOPT_MAX_HOST_CONNECTIONS, 1L);

    int waitCount = 0;
    while (d->m_running.load()) {
        {
            Locker<Mutex> l(*d->m_globalDataMutex);
            while (d->m_pendingRequests.size()) {
                auto error =
                    curl_easy_setopt(d->m_pendingRequests[0]->m_curl,
                                     CURLOPT_PRIVATE, d->m_pendingRequests[0]);
                STARFISH_ASSERT(error == CURLE_OK);
                curl_multi_add_handle(curlMultiHandle,
                                      d->m_pendingRequests[0]->m_curl);
                remainedRequest.push_back(d->m_pendingRequests[0]->m_mutex);
                d->m_pendingRequests.erase(d->m_pendingRequests.begin());
            }
        }

        int numfds = 0;
        const int waitTime =
            1000; // waiting time for curl_multi_wait when numfds != 0
        const int sleepTime =
            1000 * 50; // if numfds == 0, curl_multi_wait function returns
                       // instantly. so we need to sleep 50ms

        curl_multi_wait(curlMultiHandle, NULL, 0, waitTime, &numfds);

        int stillRunning = 0;
        curl_multi_perform(curlMultiHandle, &stillRunning);
        if (stillRunning == 0) {
            waitCount++;
            std::this_thread::sleep_for(std::chrono::microseconds(sleepTime));
        } else {
            waitCount = 0;
        }

        CURLMsg* msg;
        int msgs_left;
        while ((msg = curl_multi_info_read(curlMultiHandle, &msgs_left))) {
            if (msg->msg == CURLMSG_DONE) {
                CurlMultiRequestData* r;
                curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, &r);
                r->m_result = msg->data.result;
                Mutex* m = r->m_mutex;
                auto iter = std::find(remainedRequest.begin(),
                                      remainedRequest.end(), m);
                remainedRequest.erase(iter);
                curl_multi_remove_handle(curlMultiHandle, msg->easy_handle);
                m->unlock();
            }
        }

        // wait for 3 sec for next request
        if (waitCount > (3 * 1000 * 1000 / sleepTime)) {
            Locker<Mutex> l(*d->m_globalDataMutex);
            if (!d->m_pendingRequests.size()) {
                d->m_finishing = true;
                break;
            }
        }
    }

    while (remainedRequest.size()) {
        remainedRequest.back()->unlock();
        remainedRequest.pop_back();
    }

    curl_multi_cleanup(curlMultiHandle);

    return nullptr;
}

NetworkSharedResourceManager::CurlMultiData::CurlMultiData(
    MessageLoop* ml, Mutex* curlMultiRequestDataMutex)
{
    m_running = true;
    m_finishing = false;
    m_ml = ml;
    m_globalDataMutex = curlMultiRequestDataMutex;
    m_thread = new Thread(nullptr);
    m_thread->run(ml, NetworkSharedResourceManager::curlMultiWorker, this);
}

void NetworkSharedResourceManager::startMultiRequestThreadIfNeeds(
    MessageLoop* ml, const std::string& origin)
{
    Locker<Mutex> l(*m_curlMultiRequestDataMutex);

    auto iter = m_curlMultiRequestData.find(origin);
    if (iter == m_curlMultiRequestData.end()) {
        CurlMultiData* d =
            new (NoGC) CurlMultiData(ml, m_curlMultiRequestDataMutex);
        m_curlMultiRequestData.insert(std::make_pair(origin, d));
    } else {
        if (iter->second->m_finishing) {
            iter->second->m_finishing = false;
            if (iter->second->m_thread->isAlive()) {
                iter->second->m_thread->finishUnjoined();
            }
            iter->second->m_thread->run(ml, curlMultiWorker, iter->second);
        }
    }
}

void NetworkSharedResourceManager::appendPendingMultiRequest(
    const std::string& origin, CurlMultiRequestData* r)
{
    Locker<Mutex> l(*m_curlMultiRequestDataMutex);
    auto iter = m_curlMultiRequestData.find(origin);
    if (iter != m_curlMultiRequestData.end()) {
        iter->second->m_pendingRequests.push_back(r);
        if (iter->second->m_finishing) {
            // restart thread if finished
            iter->second->m_finishing = false;
            iter->second->m_ml->addIdlerWithNoGCRootingInOtherThread(
                nullptr,
                [](size_t, void* data) {
                    CurlMultiData* d = (CurlMultiData*)data;
                    if (d->m_thread->isAlive()) {
                        d->m_thread->finishUnjoined();
                    }
                    d->m_thread->run(d->m_ml, curlMultiWorker, d);
                },
                iter->second);
        }
    } else {
        // thread ended!
    }
}
} // namespace Starfish
