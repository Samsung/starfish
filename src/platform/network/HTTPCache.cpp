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

#if defined(STARFISH_ENABLE_HTTPCACHE)
#include "StarFishConfig.h"
#include "HTTPCache.h"
#include "platform/network/http/HTTPHeaderMap.h"
#include "platform/file/FileIO.h"
#include "core/modules/resource_request/NetworkURLResourceRequestJobDelegate.h"
#include "core/modules/resource_request/ResourceRequest.h"
#include "platform/network/http/HTTPResponse.h"
#include "platform/network/http/HTTPRequest.h"
#include "platform/network/http/HTTPTransaction.h"
#include "platform/loader/ResourceURL.h"
#include "core/modules/threading/Thread.h"
#include "core/modules/profiling/Profiling.h"
#include "binding/ScriptWrappable.h"
#include "core/dom/Document.h"
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fstream>
#include <iostream>

#define INDEX_FILE_NAME "/index.txt"

namespace StarFish {

static void clearDirectory(const char* path)
{
    STARFISH_ASSERT(isMainThread());
    DIR* dir;
    struct stat statPath, statEntry;
    struct dirent* entry;

    stat(path, &statPath);
    if (S_ISDIR(statPath.st_mode) == 0) {
        STARFISH_LOG_ERROR("Is not directory : %s\n", path);
        STARFISH_ASSERT_NOT_REACHED();
    }
    if ((dir = opendir(path)) == nullptr) {
        STARFISH_LOG_ERROR("Can`t open directory : %s\n", path);
        STARFISH_ASSERT_NOT_REACHED();
    }

    size_t pathLen = strlen(path);

    while ((entry = readdir(dir)) != NULL) {
        // Skip entries "." and ".."
        if (!strncmp(entry->d_name, ".", 1) ||
            !strncmp(entry->d_name, "..", 2)) {
            continue;
        }

        size_t entryLen = strlen(entry->d_name);
        char* fullPath = (char*)calloc(pathLen + entryLen + 1, sizeof(char));

        if (!fullPath) {
            STARFISH_ASSERT_NOT_REACHED();
        }

        strncpy(fullPath, path, pathLen);
        strncat(fullPath, "/", 1);
        strncat(fullPath, entry->d_name, entryLen);

        stat(fullPath, &statEntry);

        // recursively remove a nested directorys
        if (S_ISDIR(statEntry.st_mode) != 0) {
            clearDirectory(fullPath);
            continue;
        }

        // remove a file object
        if (unlink(fullPath) != 0) {
            STARFISH_LOG_ERROR("Can`t remove a file: %s\n", fullPath);
            STARFISH_ASSERT_NOT_REACHED();
        }
    }
    // remove the devastated directory and close the object of it
    if (rmdir(path) != 0) {
        STARFISH_LOG_ERROR("Can`t remove a directory: %s\n", path);
        STARFISH_ASSERT_NOT_REACHED();
    }
    closedir(dir);
}

static HTTPCache* instance = nullptr;

HTTPCache::HTTPCache(String* cacheDirPath)
    : m_cacheEntryTable()
    , m_cacheDirPath(cacheDirPath)
    , m_indexFilePath()
{
    m_indexFilePath = m_cacheDirPath->concat(INDEX_FILE_NAME);
    initCacheDir();
    initFromIndexFileIfPossible();
}

HTTPCache::~HTTPCache()
{
}

void HTTPCache::initFromIndexFileIfPossible()
{
    STARFISH_ASSERT(isMainThread());

    std::ifstream ifs(m_indexFilePath->toUTF8NonGCString().data());

    size_t entryKey;
    std::string urlStr, entryfileName;
    int64_t date, age, requestTime, responseTime, maxAge;
    int noCache, mustRevalidate;

    while (ifs >> entryKey >> urlStr >> date >> age >> requestTime >>
           responseTime >> maxAge >> noCache >> mustRevalidate >>
           entryfileName) {
        ResourceURL* url = new ResourceURL(urlStr.data());

        CacheControl cc;
        cc.maxAge = maxAge;
        cc.noCache = (bool)noCache;
        cc.mustRevalidate = (bool)mustRevalidate;

        EntryFreshnessInfo info;
        info.date = date;
        info.age = age;
        info.requestTime = requestTime;
        info.responseTime = responseTime;

        HTTPCacheEntry* newEntry = new HTTPCacheEntry(
            url, info, cc, String::fromUTF8(entryfileName.data()));

        m_cacheEntryTable.insert(
            std::pair<size_t, HTTPCacheEntry*>(newEntry->entryKey(), newEntry));
    }

    if (m_cacheEntryTable.size() > 0 && ifs.eof()) {
        ifs.eof();
    } else {
        // Failed to load index file
        m_cacheEntryTable.clear();
        ifs.close();
        // Clear and init cache dir
        clearAndRemoveCacheDir();
        initCacheDir();
    }
}

bool HTTPCache::isFresh(HTTPCacheEntry* entry)
{
    // https://tools.ietf.org/html/rfc7234#section-4.2
    // See 4.2. Freshness
    // response_is_fresh = (freshnessLifetime > currentAge)

    EntryFreshnessInfo info = entry->entryFreshnessInfo();
    int64_t responeTime = info.responseTime;
    int64_t freshnessLifetime = entry->cacheControl().maxAge;
    int64_t apparentAge =
        (0 > (responeTime - info.date)) ? 0 : (responeTime - info.date);
    int64_t responseDelay = (responeTime - info.requestTime);
    int64_t correctedAgeValue = (info.age + responseDelay);
    int64_t correctedInitialAge =
        (apparentAge > correctedAgeValue) ? apparentAge : correctedAgeValue;
    int64_t residentTime = ((timestamp() / 1000) - responeTime);
    int64_t currentAge = correctedInitialAge + residentTime;

    return freshnessLifetime > currentAge;
}

HTTPCacheEntryMultiMap::iterator HTTPCache::cacheHit(ResourceURL* url)
{
    // TODO : Check that cached data is fresh enough.

    auto range = m_cacheEntryTable.equal_range(url->urlString()->hashValue());
    for (auto it = range.first; it != range.second; ++it) {
        if (*(it->second->url()) == *url) {
            return it;
        }
    }
    return m_cacheEntryTable.end();
}

void HTTPCache::initCacheDir()
{
    const char* path = m_cacheDirPath->toUTF8NonGCString().data();
    int ret = mkdir(path, 0755);
    if (ret == 0) {
        std::ofstream ofs;
        ofs.open(m_indexFilePath->toUTF8NonGCString().data());
        ofs.close();
    } else if (ret == -1 && errno != EEXIST) {
        STARFISH_LOG_ERROR("%s directory create error : %s\n", path,
                           strerror(errno));
        STARFISH_ASSERT_NOT_REACHED();
    }
}

void HTTPCache::caching(NetworkURLWorkerData* data)
{
    STARFISH_ASSERT(isMainThread());

    if (data->cacheHit) {
        return;
    }

    // TODO : Consider date, max-age, if it is fresh enough, only max-age is
    // updated.

    CacheControl cc;
    auto it =
        data->request->responseHeaderMap().find(HTTPHeaderMap::kCacheControl);
    if (it != data->request->responseHeaderMap().end()) {
        cc = parseCacheControl(it->second);
    }

    if (cc.noStore) {
        return;
    }

    EntryFreshnessInfo info;
    it = data->request->responseHeaderMap().find(HTTPHeaderMap::kDate);
    if (it != data->request->responseHeaderMap().end()) {
        String* value = String::createASCIIString(it->second.data());
        double parsedDate = parseDate(
            data->request->document()->scriptBindingInstance(), value);
        if (!std::isnan(parsedDate)) {
            info.date = parsedDate / 1000.0;
        }
    }

    it = data->request->responseHeaderMap().find(HTTPHeaderMap::kAge);
    if (it != data->request->responseHeaderMap().end()) {
        String* value = String::createASCIIString(it->second.data());
        info.age = String::parseInt64(value);
    }

    info.requestTime = data->httpTransaction->httpRequest().requestTime();
    info.responseTime = data->httpTransaction->httpResponse().responseTime();

    HTTPCacheEntry* newEntry =
        new HTTPCacheEntry(data->request->url(), info, cc);
    newEntry->setEntryFileNameUsingCachePath(m_cacheDirPath);

    bool ret = newEntry->writeRawDataToEntryFile(data->request->response());

    if (ret) {
        m_cacheEntryTable.insert(
            std::pair<size_t, HTTPCacheEntry*>(newEntry->entryKey(), newEntry));
    }
}

bool HTTPCache::flush()
{
    std::ofstream ofs(m_indexFilePath->toUTF8NonGCString().data());

    if (!ofs.good()) {
        return false;
    }

    for (auto it : m_cacheEntryTable) {
        HTTPCacheEntry* entry = it.second;
        ofs << entry->toString()->toUTF8NonGCString().data() << std::endl;
    }

    ofs.flush();
    ofs.close();

    if (!ofs.good()) {
        return false;
    }

    // TODO : Verify data consistency

    return true;
}

void HTTPCache::clearAndRemoveCacheDir()
{
    clearDirectory(m_cacheDirPath->toUTF8NonGCString().data());
}

CacheControl HTTPCache::parseCacheControl(std::string directives)
{
    // https://tools.ietf.org/html/rfc7234#page-21
    // See 5.2, 5.2.1, 5.2.2

    String* str = String::fromUTF8(directives.data());

    GCVector<String*> tokens;
    str->split(',', tokens);

    CacheControl cc;
    for (auto directive : tokens) {
        directive = directive->trim();

        size_t pos = directive->find("=");

        if (pos != SIZE_MAX) {
            String* key = directive->substring(0, pos)->trim();
            String* value =
                directive->substring(pos + 1, directive->length() - pos - 1)
                    ->trim();

            if (key->equals("max-age")) {
                cc.maxAge = String::parseInt64(value);
            }
        } else {
            if (directive->equals("no-cache")) {
                cc.noCache = true;
            } else if (directive->equals("no-store")) {
                cc.noStore = true;
            } else if (directive->equals("must-revalidate")) {
                cc.mustRevalidate = true;
            }
        }
    }
    return cc;
}
}
#endif
