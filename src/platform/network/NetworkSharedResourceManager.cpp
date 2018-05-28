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

#include "StarFishConfig.h"
#include "core/dom/Document.h"
#include "core/modules/profiling/Profiling.h"
#include "binding/ScriptWrappable.h"
#include "NetworkSharedResourceManager.h"
#include "core/modules/threading/Locker.h"
#include "core/modules/threading/Mutex.h"

#if !OS(WINDOWS)
#include <openssl/crypto.h>
#endif

#define CURLHANDLE_CACHE_PRUNE_MINIMUM_SIZE 12
#define CURLHANDLE_CACHE_PRUNE_MINIMUM_INTERVAL_S 0.5
#define CURLHANDLE_CACHE_IDLE_TIME_LIMIT_S 0.25

namespace StarFish {
#if !OS(WINDOWS)
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
    for (i = 0; i < CRYPTO_num_locks(); i++)
        pthread_mutex_destroy(&(sslLockarray[i]));

    OPENSSL_free(sslLockarray);
}
#endif
static NetworkSharedResourceManager* g_networkSharedResourceMangerInstance =
    nullptr;

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
    int index = path->find(&tokens[2]);
    if (index) {
        return;
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

static String* transformetoNetscapeCookieFormat(Document* document,
                                                ResourceURL* url, String* value)
{
    if (value->isEmpty()) {
        return String::emptyString;
    }
    if (!value->containsOnlyASCIIChars()) {
        return String::emptyString;
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
    String* domain = url->host();
    String* path = url->pathname();
    const char* secure = "FALSE";

    for (size_t i = 1; i < tokens.size(); ++i) {
        if (tokens[i].contains("=")) {
            GCVector<StringView> pair;
            StringUtils::tokenize(&tokens[i], "=", 1, pair);
            String* key = pair[0].trim()->toASCIILower();
            if (key->equals("expires")) {
                String* value = pair[1].trim();
                double parsedDate =
                    parseDate(document->scriptBindingInstance(), value);
                // RFC6265 say : If the attribute-value failed to parse as a
                //               cookie date, ignore it
                if (!std::isnan(parsedDate)) {
                    expires = parsedDate / 1000.0;
                }
            } else if (key->equals("max-age")) {
                String* value = pair[1].trim();
                int64_t parsedValue = String::parseInt64(value);
                time_t current = time(0);
                if (parsedValue > 0 &&
                    (USER_AGENT_MAXIMUM_DATE_VALUE - current) >= parsedValue) {
                    expires = current + parsedValue;
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
    const char* allowSubDomain = domain->startsWith(".") ? "TRUE" : "FALSE";
    String* expiresStr = String::fromInt64(expires);
    StringBuilder builder;

    builder.appendString(domain);
    builder.appendString("\t");
    builder.appendString(allowSubDomain);
    builder.appendString("\t");
    builder.appendString(path);
    builder.appendString("\t");
    builder.appendString(secure);
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

void NetworkSharedResourceManager::close()
{
    STARFISH_ASSERT(g_networkSharedResourceMangerInstance);
    STARFISH_LOG_INFO("NetworkSharedResourceManager::close()\n");
    delete g_networkSharedResourceMangerInstance;
    g_networkSharedResourceMangerInstance = nullptr;
}

NetworkSharedResourceManager::NetworkSharedResourceManager()
    : m_curlShareHandle(nullptr)
    , m_curlHandleDataCache()
    , m_lastCachePruneTime(0)
    , m_cacheClearTimerID(SIZE_MAX)
    , m_cookieStoreFilePath("")
{
    initMutexes();
#if !OS(WINDOWS)
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
}

NetworkSharedResourceManager::~NetworkSharedResourceManager()
{
    clearAllCurlHandleDataCache();
    curl_share_cleanup(m_curlShareHandle);
    curl_global_cleanup();

#if !OS(WINDOWS)
    removeSSLLocks();
#endif
    removeMutexes();
}

void NetworkSharedResourceManager::initMutexes()
{
    for (int i = 0; i < curl_lock_data::CURL_LOCK_DATA_LAST + 1; ++i) {
        m_mutexes[i] = new (NoGC) Mutex();
    }
}

void NetworkSharedResourceManager::removeMutexes()
{
    for (int i = 0; i < curl_lock_data::CURL_LOCK_DATA_LAST + 1; ++i) {
        delete m_mutexes[i];
    }
}

CURLSH* NetworkSharedResourceManager::curlShareHandle() const
{
    return m_curlShareHandle;
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
    return m_mutexes[data];
}

CurlHandleData NetworkSharedResourceManager::getCurlHandleData(
    const std::string& host)
{
    Locker<Mutex> locker(*m_mutexes[curl_lock_data::CURL_LOCK_DATA_LAST]);
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
    Locker<Mutex> locker(*m_mutexes[curl_lock_data::CURL_LOCK_DATA_LAST]);
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
        STARFISH_LOG_INFO("prunning cached handles %zd => %zd \n", old,
                          m_curlHandleDataCache.size());
#endif
        m_lastCachePruneTime = tickCount();
    }
}

void NetworkSharedResourceManager::clearAllCurlHandleDataCache()
{
#ifdef STARFISH_ENABLE_TEST
    STARFISH_LOG_INFO(
        "NetworkSharedResourceManager::clearAllCurlHandleDataCache(size:%d)\n",
        (int)m_curlHandleDataCache.size());
#endif
    Locker<Mutex> locker(*m_mutexes[curl_lock_data::CURL_LOCK_DATA_LAST]);
    while (m_curlHandleDataCache.size()) {
        CurlHandleData cd = m_curlHandleDataCache.begin()->second;
        curl_easy_cleanup(cd.curl);
        m_curlHandleDataCache.erase(m_curlHandleDataCache.begin());
    }
}

String* NetworkSharedResourceManager::cookeis(ResourceURL* url)
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
        String* domain = url->host();
        String* path = url->pathname();
        StringBuilder cookiesBuilder;
        for (struct curl_slist* p = cookieList; p; p = p->next) {
            String* cookie = String::fromUTF8(p->data);
            appendMatchingCookie(cookie, domain, path, cookiesBuilder);
        }
        cookies = cookiesBuilder.finalize();
        curl_slist_free_all(cookieList);
    }
    curl_easy_cleanup(curl);
    return cookies;
}

void NetworkSharedResourceManager::setCookies(Document* document,
                                              ResourceURL* url, String* value)
{
    CURL* curl = curl_easy_init();

    if (!curl) {
        return;
    }
    curl_easy_setopt(curl, CURLOPT_SHARE, m_curlShareHandle);

    if (m_cookieStoreFilePath.compare("") != 0) {
        curl_easy_setopt(curl, CURLOPT_COOKIEJAR, m_cookieStoreFilePath.data());
    }
    String* cookie = transformetoNetscapeCookieFormat(document, url, value);
    STARFISH_ASSERT(cookie->containsOnlyASCIIChars());
    STARFISH_ASSERT(cookie->bufferAccessData().isNullTerminated);
    curl_easy_setopt(curl, CURLOPT_COOKIELIST,
                     cookie->bufferAccessData().asciiData());
    curl_easy_cleanup(curl);
}
} // namespace StarFish
