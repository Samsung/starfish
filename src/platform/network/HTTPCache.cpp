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
#include "platform/file/File.h"
#include "HTTPCache.h"
#include "platform/network/http/HTTPHeaderMap.h"
#include "core/modules/resource_request/NetworkURLResourceRequestJobDelegate.h"
#include "core/modules/resource_request/ResourceRequest.h"
#include "platform/network/http/HTTPResponse.h"
#include "platform/network/http/HTTPRequest.h"
#include "platform/network/http/HTTPTransaction.h"
#include "platform/network/http/HTTPUtil.h"
#include "platform/loader/ResourceURL.h"
#include "platform/file/File.h"
#include "core/modules/threading/Thread.h"
#include "core/modules/profiling/Profiling.h"
#include "binding/ScriptWrappable.h"
#include "core/dom/Document.h"
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define INDEX_FILE_NAME "/index.txt"
#define DEFAULT_HTTP_CACHE_SIZE 1024 * 1024 * 10
#define MAX_ENTRY_FILE_SIZE (DEFAULT_HTTP_CACHE_SIZE * 0.04)
#define NUM_OF_COL 15

namespace StarFish {

// TODO : Implement Directory util and move the below method to it
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
    , m_currentCacheSize(0)
    , m_cacheSize(DEFAULT_HTTP_CACHE_SIZE)
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

    File* in = File::create();

    if (!in->open(m_indexFilePath, File::FileMode::Read)) {
        return;
    }

    Nullable<String*> data = in->readAll();
    in->close();

    if (!data.hasValue()) {
        return;
    }

    String* index = data.getValue();
    GCVector<StringView> table;

    StringUtils::tokenize(index, "\n", 1, table);

    // Last line is "\n"
    for (auto row = table.begin(); row != table.end() - 1; row++) {
        GCVector<StringView> columns;
        StringUtils::tokenize(&(*row), HTTPCacheEntry::kSeparator, 1, columns);

        if (columns.size() != NUM_OF_COL) {
            m_cacheEntryTable.clear();
            m_cacheLRUList.clear();
            m_currentCacheSize = 0;
            return;
        }
        // TODO : Check whether each column is valid or not

        // entryKey(UINT) urlString(STRING) no-cache(0|1) mustRevalidate(0|1)
        // maxAge(UINT) contentLanguage(STRING) contentLength(UINT)
        // contentType(STRING) date(UINT) age(UINT) rquestTime(UINT)
        // responeTime(UINT) lastModified(UINT) Etag(STRING)
        // entryFileName(STRING)

        auto tempStr = columns[1].toUTF8NonGCString();
        ResourceURL* url =
            new ResourceURL(String::fromUTF8(tempStr.data(), tempStr.length()));

        CacheControl cc;
        cc.noCache = columns[2].equals("true") ? true : false;
        cc.mustRevalidate = columns[3].equals("true") ? true : false;
        cc.maxAge = String::parseInt64(&columns[4]);

        HTTPContentInfo cinfo;
        if (!columns[5].equals("null")) {
            cinfo.contentLanguage = columns[5].toUTF8NonGCString();
        }
        cinfo.contentLength = String::parseInt64(&columns[6]);
        if (!columns[7].equals("null")) {
            cinfo.contentType = columns[7].toUTF8NonGCString();
        }

        HTTPFreshnessInfo finfo;
        finfo.date = String::parseInt64(&columns[8]);
        finfo.age = String::parseInt64(&columns[9]);
        finfo.requestTime = String::parseInt64(&columns[10]);
        finfo.responseTime = String::parseInt64(&columns[11]);
        finfo.lastModified = String::parseInt64(&columns[12]);
        if (!columns[13].equals("null")) {
            finfo.etag = columns[7].toUTF8NonGCString();
        }

        tempStr = columns[14].toUTF8NonGCString();

        HTTPCacheEntry* newEntry = new HTTPCacheEntry(
            url, cc, cinfo, finfo,
            String::fromUTF8(tempStr.data(), tempStr.length()));

        m_cacheEntryTable.insert(
            std::pair<size_t, HTTPCacheEntry*>(newEntry->entryKey(), newEntry));
        m_cacheLRUList.push_back(columns[1].toUTF8NonGCString());

        m_currentCacheSize += cinfo.contentLength;
    }
    expire();
    return;
}

HTTPCacheEntryMultiMap::iterator HTTPCache::get(ResourceURL* url)
{
    STARFISH_ASSERT(isMainThread());

    auto entryIter = findEntryTableData(url->urlString());
    if (entryIter == m_cacheEntryTable.end()) {
        return m_cacheEntryTable.end();
    }
    auto urlStr = entryIter->second->url()->urlString()->toUTF8NonGCString();
    addCacheLRUListData(urlStr);

    return entryIter;
}

HTTPCacheEntryMultiMap::iterator HTTPCache::findEntryTableData(String* key)
{
    auto range = m_cacheEntryTable.equal_range(key->hashValue());
    for (auto it = range.first; it != range.second; ++it) {
        if (it->second->url()->urlString()->equals(key)) {
            return it;
        }
    }
    return m_cacheEntryTable.end();
}

void HTTPCache::initCacheDir()
{
    STARFISH_ASSERT(isMainThread());

    const char* path = m_cacheDirPath->toUTF8NonGCString().data();
    int ret = mkdir(path, 0755);

    if (ret == 0) {
        File* in = File::create();
        if (!in->open(m_indexFilePath, File::FileMode::Write)) {
            return;
        }
        in->close();
    } else if (ret == -1 && errno != EEXIST) {
        STARFISH_LOG_ERROR("%s directory create error : %s\n", path,
                           strerror(errno));
        STARFISH_ASSERT_NOT_REACHED();
    }
}

void HTTPCache::put(NetworkURLWorkerData* data)
{
    STARFISH_ASSERT(isMainThread());

    if (data->cachedEntry) {
        return;
    }

    const HeaderMap& headerMap = data->request->responseHeaderMap();

    HTTPContentInfo cinfo = HTTPUtil::getHTTPContentInfoFromHeaders(headerMap);
    if (cinfo.contentLength > MAX_ENTRY_FILE_SIZE) {
        return;
    }

    CacheControl cc;
    auto it = headerMap.find(HTTPHeaderMap::kCacheControl);
    if (it != headerMap.end()) {
        cc = HTTPUtil::parseCacheControl(it->second);
    }
    // TODO : Change initial value(ex: -1 or using string) of max-age and then
    // modify freshness calculation algorithm appropriately
    if (cc.noStore || cc.maxAge == 0) {
        return;
    }

    HTTPFreshnessInfo finfo = HTTPUtil::getHTTPFreshnessInfoFromHeaders(
        data->request->document()->scriptBindingInstance(), headerMap);

    finfo.requestTime = data->httpTransaction->httpRequest().requestTime();
    finfo.responseTime = data->httpTransaction->httpResponse().responseTime();

    pruningIfNeeds(cinfo.contentLength);

    HTTPCacheEntry* newEntry =
        new HTTPCacheEntry(data->request->url(), cc, cinfo, finfo);
    newEntry->setEntryFileNameUsingCachePath(m_cacheDirPath);

    bool ret = newEntry->writeRawDataToEntryFile(data->request->response());

    if (!ret) {
        return;
    }

    m_cacheEntryTable.insert(
        std::pair<size_t, HTTPCacheEntry*>(newEntry->entryKey(), newEntry));
    auto urlStr = data->request->url()->urlString()->toUTF8NonGCString();

    // TODO : LRUlist must be flushed to disk and restored from disk.
    addCacheLRUListData(urlStr);

    m_currentCacheSize += cinfo.contentLength;
}

bool HTTPCache::flush()
{
    STARFISH_ASSERT(isMainThread());

    expire();

    File* out = File::create();

    if (!out->open(m_indexFilePath, File::FileMode::Write)) {
        return false;
    }

    for (auto it : m_cacheLRUList) {
        const std::string& urlStr = it;
        auto tableIter = findEntryTableData(String::fromUTF8(urlStr.data()));
        HTTPCacheEntry* entry = tableIter->second;
        if (!out->writeLine(entry->toString())) {
            out->close();
            return false;
        }
    }

    bool ret = (out->flush() == 0) & (out->close() == 0);
    // TODO : Verify data consistency

    return ret;
}

void HTTPCache::pruningIfNeeds(size_t contentLength)
{
    STARFISH_ASSERT(isMainThread());

    expire();
    // Consider indexFile size
    size_t removedSize = 0;
    size_t reserve = contentLength;

    if (m_currentCacheSize + reserve > m_cacheSize) {
        for (auto iter = m_cacheLRUList.begin();
             iter != m_cacheLRUList.end() && removedSize < reserve;) {
            std::string urlStr = (*iter);
            auto tableIter =
                findEntryTableData(String::fromUTF8(urlStr.data()));
            HTTPCacheEntry* cacheEntry = tableIter->second;

            if (cacheEntry->usingCount() > 0) {
                iter++;
                continue;
            }

            File* fio = File::create();
            if (fio->open(cacheEntry->entryFileName(), File::Read)) {
                HTTPContentInfo info = cacheEntry->httpContentInfo();
                m_currentCacheSize -= info.contentLength;
                removedSize += info.contentLength;
                fio->removeFile();
                fio->close();
            }
            m_cacheEntryTable.erase(tableIter);
            iter = m_cacheLRUList.erase(iter);
        }
    }
}

void HTTPCache::expire()
{
    for (auto it = m_cacheEntryTable.begin(); it != m_cacheEntryTable.end();) {
        if (!it->second->isFresh()) {
            File* fio = File::create();
            fio->open(it->second->entryFileName(), File::Read);
            if (fio->isOpen()) {
                fio->removeFile();
            }
            fio->close();

            auto urlStr = it->second->url()->urlString()->toUTF8NonGCString();
            deleteCacheLRUListData(urlStr);
            it = m_cacheEntryTable.erase(it);
        } else {
            it++;
        }
    }
}

void HTTPCache::clearAndRemoveCacheDir()
{
    clearDirectory(m_cacheDirPath->toUTF8NonGCString().data());
}

void HTTPCache::addCacheLRUListData(std::string& url)
{
    auto it = std::find(m_cacheLRUList.begin(), m_cacheLRUList.end(), url);
    if (m_cacheLRUList.end() != it) {
        m_cacheLRUList.erase(it);
    }
    m_cacheLRUList.push_back(url);
}

void HTTPCache::deleteCacheLRUListData(std::string& url)
{
    auto it = std::find(m_cacheLRUList.begin(), m_cacheLRUList.end(), url);
    if (m_cacheLRUList.end() != it) {
        m_cacheLRUList.erase(it);
    }
}
}

#endif
