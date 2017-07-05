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

#include "StarFishConfig.h"
#include "core/dom/Document.h"
#include "binding/ScriptWrappable.h"
#include "NetworkSharedResourceManager.h"
#include "core/modules/threading/Locker.h"
#include "core/modules/threading/Mutex.h"

#ifdef STARFISH_SSLBACKEND_GNU_TLS
#include <gcrypt.h>
#include <errno.h>
#endif

#ifdef STARFISH_SSLBACKEND_OPENSSL
#include <openssl/crypto.h>
#endif

namespace StarFish {

#ifdef STARFISH_SSLBACKEND_GNU_TLS
GCRY_THREAD_OPTION_PTHREAD_IMPL;

void initSSLLocks(void)
{
    gcry_control(GCRYCTL_SET_THREAD_CBS, &gcry_threads_pthread);
}

#define removeSSLLocks() // Do nohting
#endif

#ifdef STARFISH_SSLBACKEND_OPENSSL
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

static NetworkSharedResourceManager* instance = nullptr;

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

    GCVector<String*> tokens;
    cookie->split('\t', tokens);
    STARFISH_ASSERT(tokens.size() == 7);

    if (tokens[0]->startsWith("#HttpOnly_")) {
        return;
    }
    if (!domainMatch(tokens[0], domain)) {
        return;
    }
    int index = path->find(tokens[2]);
    if (index) {
        return;
    }
    time_t now = 0;
    time(&now);
    // Use int64_t to explicitly specify the width of bits.
    int64_t expires = String::parseInt64(tokens[4]);
    if (expires && now > expires) {
        return;
    }
    if (cookiesBuilder.contentLength() > 0) {
        cookiesBuilder.appendString("; ");
    }
    cookiesBuilder.appendString(tokens[5]);
    cookiesBuilder.appendString("=");
    cookiesBuilder.appendString(tokens[6]);
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
    GCVector<String*> tokens;
    value->split(';', tokens);
    String* cookieName = String::emptyString;
    String* cookieValue = String::emptyString;

    // First attr should be 'cookiename=value'
    if (tokens[0]->contains("=")) {
        GCVector<String*> pair;
        tokens[0]->split('=', pair);
        cookieName = pair[0];
        cookieValue = pair[1];
    } else {
        // According to RFC6265, it should be ignored
        // but modern browsers appear to treat this as <cookiename>=<empty>"
        cookieName = tokens[0];
    }
    int64_t expires = 0;
    String* domain = url->host();
    String* path = url->pathname();
    const char* secure = "FALSE";

    for (size_t i = 1; i < tokens.size(); ++i) {
        if (tokens[i]->contains("=")) {
            GCVector<String*> pair;
            tokens[i]->split('=', pair);
            String* key = pair[0]->trim()->toLower();
            String* value = pair[1]->trim();
            if (key->equals("expires")) {
                double parsedDate =
                    parseDate(document->scriptBindingInstance(), value);
                // RFC6265 say : If the attribute-value failed to parse as a
                //               cookie date, ignore it
                if (!std::isnan(parsedDate)) {
                    expires = parsedDate / 1000.0;
                }
            } else if (key->equals("max-age")) {
                int64_t parsedValue = String::parseInt64(value);
                time_t current = time(0);
                if (parsedValue > 0 &&
                    (USER_AGENT_MAXIMUM_DATE_VALUE - current) >= parsedValue) {
                    expires = current + parsedValue;
                }
            } else if (key->equals("domain")) {
                domain = value;
            } else if (key->equals("path")) {
                path = value;
            }
        } else {
            String* key = tokens[i]->trim()->toLower();
            if (key->equals("secure")) {
                secure = "TRUE";
            }
        }
    }
    const char* allowSubDomain = domain->startsWith(".") ? "True" : "FALSE";
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
    if (!instance) {
        instance = new NetworkSharedResourceManager();
    }
    return instance;
}

void NetworkSharedResourceManager::close()
{
    STARFISH_ASSERT(instance);
    delete instance;
}

NetworkSharedResourceManager::NetworkSharedResourceManager()
    : m_curlShareHandle(nullptr)
    , m_cookieJarFileName("/tmp/StarFish_Cookies.txt") // Temporary name
    , m_cookieMutex(new (NoGC) Mutex())
    , m_sslMutex(new (NoGC) Mutex())
    , m_dnsMutex(new (NoGC) Mutex())
    , m_shareMutex(new (NoGC) Mutex())
    , m_storeCookieFile(false)
{
    initSSLLocks();

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
    curl_share_cleanup(m_curlShareHandle);
    curl_global_cleanup();

    removeSSLLocks();

    delete m_cookieMutex;
    delete m_sslMutex;
    delete m_dnsMutex;
    delete m_shareMutex;
}

CURLSH* NetworkSharedResourceManager::curlShareHandle() const
{
    return m_curlShareHandle;
}

std::string NetworkSharedResourceManager::cookieJarFileName() const
{
    return m_cookieJarFileName;
}

void NetworkSharedResourceManager::setCookieJarFileName(const std::string& name)
{
    m_cookieJarFileName = name;
}

void NetworkSharedResourceManager::initCookieSession()
{
    CURL* curl = curl_easy_init();

    if (!curl) {
        STARFISH_ASSERT_NOT_REACHED();
        return;
    }

    curl_easy_setopt(curl, CURLOPT_SHARE, m_curlShareHandle);
    if (m_storeCookieFile) {
        curl_easy_setopt(curl, CURLOPT_COOKIEFILE, m_cookieJarFileName.data());
        curl_easy_setopt(curl, CURLOPT_COOKIEJAR, m_cookieJarFileName.data());
    }
    curl_easy_setopt(curl, CURLOPT_COOKIESESSION, 1);
    curl_easy_cleanup(curl);
}

Mutex* NetworkSharedResourceManager::resourceMutex(curl_lock_data data)
{
    switch (data) {
    case CURL_LOCK_DATA_COOKIE:
        return m_cookieMutex;
    case CURL_LOCK_DATA_SSL_SESSION:
        return m_sslMutex;
    case CURL_LOCK_DATA_DNS:
        return m_dnsMutex;
    case CURL_LOCK_DATA_SHARE:
        return m_shareMutex;
    default:
        STARFISH_ASSERT_NOT_REACHED();
        return nullptr;
    }
}

void NetworkSharedResourceManager::enableToStoreCookiesJarAsFile()
{
    m_storeCookieFile = true;
}

void NetworkSharedResourceManager::disableToStoreCookiesJarAsFile()
{
    m_storeCookieFile = false;
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
    curl_easy_setopt(curl, CURLOPT_COOKIEJAR, m_cookieJarFileName.data());
    curl_easy_setopt(curl, CURLOPT_SHARE, m_curlShareHandle);

    String* cookie = transformetoNetscapeCookieFormat(document, url, value);
    STARFISH_ASSERT(cookie->containsOnlyASCIIChars());
    STARFISH_ASSERT(cookie->bufferAccessData().isNullTerminated);
    curl_easy_setopt(curl, CURLOPT_COOKIELIST,
                     cookie->bufferAccessData().asciiData());
    curl_easy_cleanup(curl);
}
}
