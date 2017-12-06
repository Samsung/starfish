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
#include "core/modules/resource_request/NetworkURLResourceRequestJobDelegate.h"
#include "core/modules/resource_request/ResourceRequest.h"
#include "platform/network/http/HTTPResponse.h"
#include "platform/network/http/HTTPRequest.h"
#include "platform/network/http/HTTPTransaction.h"
#include "platform/network/http/HTTPUtil.h"
#include "platform/loader/ResourceURL.h"
#include "platform/file/File.h"
#include "platform/file/Directory.h"
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
#define NUM_OF_COL 17

namespace StarFish {

static HTTPCache* instance = nullptr;

HTTPCache::HTTPCache(String* cacheDirPath)
    : m_cacheEntryTable()
    , m_cacheDirPath(cacheDirPath)
    , m_indexFilePath()
    , m_currentCacheSize(0)
    , m_cacheSize(DEFAULT_HTTP_CACHE_SIZE)
{
    m_indexFilePath = m_cacheDirPath->concat(INDEX_FILE_NAME);
    if (initFromIndexFileIfPossible() == false) {
        initCacheDirectory();
        initCacheMeber();
    }
}

HTTPCache::~HTTPCache()
{
}

bool HTTPCache::initFromIndexFileIfPossible()
{
    STARFISH_ASSERT(isMainThread());

    File* in = File::create();

    if (!in->open(m_indexFilePath, File::FileMode::Read)) {
        return false;
    }

    Nullable<String*> data = in->readAll();
    in->close();

    if (!data.hasValue()) {
        return false;
    }

    String* index = data.getValue();
    GCVector<StringView> table;

    StringUtils::tokenize(index, "\n", 1, table);

    // Last line is "\n"
    for (auto row = table.begin(); row != table.end() - 1; row++) {
        GCVector<StringView> columns;
        StringUtils::tokenize(&(*row), HTTPCacheEntry::kSeparator, 1, columns);

        if (columns.size() != NUM_OF_COL) {
            return false;
        }
        // TODO : Check whether each column is valid or not

        // entryKey(UINT) urlString(STRING) no-cache(0|1) mustRevalidate(0|1)
        // maxAge(UINT) contentLanguage(STRING) contentLength(UINT)
        // contentType(STRING) contentTransferEncoding(STRING) date(UINT)
        // age(UINT) rquestTime(UINT) responeTime(UINT) lastModified(UINT)
        // Etag(STRING) lastModifyFileTime(UINT) entryFileName(STRING)

        auto tempStr = columns[1].toUTF8NonGCString();
        String* urlString = String::fromUTF8(tempStr.data(), tempStr.length());
        m_cacheLRUList.push_back(urlString);
        ResourceURL* url = new ResourceURL(urlString);

        // 2~4
        CacheControl cc;
        cc.noCache = columns[2].equals("true") ? true : false;
        cc.mustRevalidate = columns[3].equals("true") ? true : false;
        cc.maxAge = String::parseInt64(&columns[4]);

        // 5~8
        HTTPContentInfo cinfo;
        if (!columns[5].equals("null")) {
            cinfo.contentLanguage = columns[5].toUTF8NonGCString();
        }
        cinfo.contentLength = String::parseInt64(&columns[6]);
        if (!columns[7].equals("null")) {
            cinfo.contentType = columns[7].toUTF8NonGCString();
        }
        if (!columns[8].equals("null")) {
            cinfo.contentTransferEncoding = columns[8].toUTF8NonGCString();
        }

        // 9~14
        HTTPFreshnessInfo finfo;
        finfo.date = String::parseInt64(&columns[9]);
        finfo.age = String::parseInt64(&columns[10]);
        finfo.requestTime = String::parseInt64(&columns[11]);
        finfo.responseTime = String::parseInt64(&columns[12]);
        finfo.lastModified = String::parseInt64(&columns[13]);
        if (!columns[14].equals("null")) {
            finfo.etag = columns[14].toUTF8NonGCString();
        }
        // 15~16
        int64_t lmft = String::parseInt64(&columns[15]);
        tempStr = columns[16].toUTF8NonGCString();
        String* efn = String::fromUTF8(tempStr.data(), tempStr.length());

        HTTPCacheEntry* newEntry =
            new HTTPCacheEntry(url, cc, cinfo, finfo, lmft, efn);

        m_cacheEntryTable.insert(
            std::pair<size_t, HTTPCacheEntry*>(newEntry->entryKey(), newEntry));

        m_currentCacheSize += cinfo.contentLength;
    }

    if (!isConsistent()) {
        return false;
    }

    expire();
    return true;
}

HTTPCacheEntryMultiMap::iterator HTTPCache::get(ResourceURL* url)
{
    STARFISH_ASSERT(isMainThread());

    auto entryIter = findEntry(url->urlString());
    if (entryIter == m_cacheEntryTable.end()) {
        return m_cacheEntryTable.end();
    }
    addCacheLRUListData(entryIter->second->url()->urlString());

    return entryIter;
}

HTTPCacheEntryMultiMap::iterator HTTPCache::findEntry(String* key)
{
    auto range = m_cacheEntryTable.equal_range(key->hashValue());
    for (auto it = range.first; it != range.second; ++it) {
        if (it->second->url()->urlString()->equals(key)) {
            return it;
        }
    }
    return m_cacheEntryTable.end();
}

void HTTPCache::put(NetworkURLWorkerData* nwd)
{
    STARFISH_ASSERT(isMainThread());

    if (nwd->cachedEntry) {
        return;
    }

    auto check = findEntry(nwd->request->url()->urlString());
    if (check != end()) {
        return;
    }

    const HeaderMap& headerMap = nwd->request->responseHeaderMap();

    HTTPContentInfo cinfo = HTTPUtil::getHTTPContentInfoFromHeaders(headerMap);
    if (cinfo.contentLength > MAX_ENTRY_FILE_SIZE || cinfo.contentLength == 0) {
        return;
    }

    CacheControl cc;
    auto it = headerMap.find(HTTPHeaderMap::kCacheControl);
    if (it != headerMap.end()) {
        cc = HTTPUtil::parseCacheControl(it->second);
    }

    HTTPFreshnessInfo finfo = HTTPUtil::getHTTPFreshnessInfoFromHeaders(
        nwd->request->document()->scriptBindingInstance(), headerMap);

    // TODO : Change initial value(ex: -1 or using string) of max-age and then
    // modify freshness calculation algorithm appropriately
    if (cc.noStore || (cc.maxAge == 0 && finfo.etag.size() == 0)) {
        return;
    }

    finfo.requestTime = nwd->httpTransaction->httpRequest().requestTime();
    finfo.responseTime = nwd->httpTransaction->httpResponse().responseTime();

    if (!pruneAsNeededForCacheSpace(cinfo.contentLength)) {
        return;
    }

    HTTPCacheEntry* newEntry =
        new HTTPCacheEntry(nwd->request->url(), cc, cinfo, finfo, 0);
    newEntry->setEntryFileNameUsingCachePath(m_cacheDirPath);

    bool ret = newEntry->writeRawDataToEntryFile(nwd->request->response());

    if (!ret) {
        return;
    }

    m_cacheEntryTable.insert(
        std::pair<size_t, HTTPCacheEntry*>(newEntry->entryKey(), newEntry));

    addCacheLRUListData(nwd->request->url()->urlString());

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
        auto tableIter = findEntry(it);
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

bool HTTPCache::pruneAsNeededForCacheSpace(const size_t reserve)
{
    STARFISH_ASSERT(isMainThread());

    // Consider indexFile size
    size_t removedSize = 0;

    if (m_currentCacheSize + reserve > m_cacheSize) {
        auto iter = m_cacheLRUList.begin();
        while (iter != m_cacheLRUList.end() && removedSize < reserve) {
            auto tableIter = findEntry(*iter);
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
        if (iter == m_cacheLRUList.end() && removedSize < reserve) {
            return false;
        }
    }
    return true;
}

bool HTTPCache::isConsistent()
{
    STARFISH_ASSERT(isMainThread());

    Directory* dir = Directory::create();
    if (!dir->open(m_cacheDirPath)) {
        dir->close();
        return false;
    }

    if ((dir->fileCount() - 1) != m_cacheLRUList.size()) {
        dir->close();
        return false;
    }
    dir->close();

    for (auto it = m_cacheEntryTable.begin(); it != m_cacheEntryTable.end();
         it++) {
        File* file = File::create();
        file->open(it->second->entryFileName(), File::Read);
        if (file->isOpen()) {
            if (file->lastModifyTime() != it->second->lastModifyFileTime()) {
                file->close();
                return false;
            }
            if (file->size() != it->second->httpContentInfo().contentLength) {
                file->close();
                return false;
            }
            file->close();
        } else {
            file->close();
            return false;
        }
    }
    return true;
}

void HTTPCache::expire()
{
    STARFISH_ASSERT(isMainThread());

    for (auto it = m_cacheEntryTable.begin(); it != m_cacheEntryTable.end();) {
        if (it->second->usingCount() == 0 &&
            (!it->second->isFresh() &&
             it->second->httpFreshnessInfo().etag.size() == 0)) {
            File* fio = File::create();
            fio->open(it->second->entryFileName(), File::Read);

            if (fio->isOpen()) {
                fio->removeFile();
            }

            fio->close();

            deleteCacheLRUListData(it->second->url()->urlString());
            it = m_cacheEntryTable.erase(it);
        } else {
            it++;
        }
    }
}

void HTTPCache::initCacheDirectory()
{
    STARFISH_ASSERT(isMainThread());

    Directory* dir = Directory::create();

    if (dir->open(m_cacheDirPath)) {
        dir->clear();
    }

    if (!dir->mkDir()) {
        STARFISH_LOG_ERROR("%s directory create error\n",
                           m_cacheDirPath->toUTF8NonGCString().data());
        STARFISH_ASSERT_NOT_REACHED();
    }

    dir->close();

    File* file = File::create();
    if (file->open(m_indexFilePath, File::FileMode::Write)) {
        file->close();
    } else {
        STARFISH_LOG_ERROR("%s file create error\n",
                           m_indexFilePath->toUTF8NonGCString().data());
        STARFISH_ASSERT_NOT_REACHED();
    }
}

void HTTPCache::initCacheMeber()
{
    m_cacheEntryTable.clear();
    m_cacheLRUList.clear();
    m_currentCacheSize = 0;
}

void HTTPCache::addCacheLRUListData(String* url)
{
    // TODO : LRU search speed
    auto iter = m_cacheLRUList.begin();
    while (iter != m_cacheLRUList.end()) {
        if ((String*)(*iter)->equals(url)) {
            iter = m_cacheLRUList.erase(iter);
            break;
        } else {
            iter++;
        }
    }
    m_cacheLRUList.push_back(url);
}

void HTTPCache::deleteCacheLRUListData(String* url)
{
    // TODO : LRU search speed
    auto iter = m_cacheLRUList.begin();
    while (iter != m_cacheLRUList.end()) {
        if ((String*)(*iter)->equals(url)) {
            iter = m_cacheLRUList.erase(iter);
            break;
        } else {
            iter++;
        }
    }
}
} // namespace StarFish

#endif
