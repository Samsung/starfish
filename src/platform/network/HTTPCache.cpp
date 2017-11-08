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
#include "platform/loader/ResourceURL.h"
#include "core/modules/threading/Thread.h"
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

    // Index is set of cache entry.
    // The initial state is empty, otherwise each rows are conist of
    // hash-key url-string max-age entry-file-name
    std::ifstream ifs(m_indexFilePath->toUTF8NonGCString().data());

    size_t entryKey;
    std::string urlStr, entryfileName;
    time_t date, maxAge;
    int noCache, mustRevalidate;

    while (ifs >> entryKey >> urlStr >> date >> maxAge >> noCache >>
           mustRevalidate >> entryfileName) {
        ResourceURL* url = new ResourceURL(urlStr.data());

        CacheControl cc;
        cc.maxAge = maxAge;
        cc.noCache = (bool)noCache;
        cc.mustRevalidate = (bool)mustRevalidate;

        HTTPCacheEntry* newEntry = new HTTPCacheEntry(
            url, date, cc, String::fromUTF8(entryfileName.data()));

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
    // auto date =
    // data->request->responseHeaderMap().find(HTTPHeaderMap::kDate);

    CacheControl cc;
    auto it =
        data->request->responseHeaderMap().find(HTTPHeaderMap::kCacheControl);
    if (it != data->request->responseHeaderMap().end()) {
        cc = parseCacheControl(it->second);
    }

    if (cc.noStore) {
        return;
    }

    HTTPCacheEntry* newEntry = new HTTPCacheEntry(data->request->url(), 0, cc);
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
