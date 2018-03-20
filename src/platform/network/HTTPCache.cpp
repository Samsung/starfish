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

#include <linux/fs.h>
#include <sys/file.h>

#define INDEX_FILE_NAME "/index.txt"
#define DEFAULT_HTTP_CACHE_SIZE 1024 * 1024 * 50
#define MAX_ENTRY_FILE_SIZE (DEFAULT_HTTP_CACHE_SIZE * 0.04)
#define NUM_OF_COL 18

namespace StarFish {

HTTPCache* g_httpCache = nullptr;
int g_httpCacheCNT = 0;

const size_t HTTPCache::kBlockSize = BLKGETSIZE;

HTTPCache::HTTPCache(String* cacheDirPath)
    : m_cacheEntryTable()
    , m_cacheDirPath(cacheDirPath)
    , m_indexFilePath(nullptr)
    , m_cacheSizeLimit(DEFAULT_HTTP_CACHE_SIZE)
    , m_currentTotalSizeOfBlocks(0)
    , m_lockfd(-1)
    , m_good(false)
    , m_cacheMode(LOAD_DEFAULT)
{
    m_indexFilePath = m_cacheDirPath->concat(INDEX_FILE_NAME);

    if (createOrOpenCacheDir() && lock()) {
        if (!initFromIndexFileIfPossible()) {
            STARFISH_LOG_ERROR("[HTTPCache] Failed to init using index\n")
            init();
            clearCacheDir();
        }
    } else {
        unlock();
        STARFISH_LOG_ERROR("[HTTPCache] Failed to create(or open) cache dir\n")
        return;
    }
    m_good = true;
}

HTTPCache::~HTTPCache()
{
}

bool HTTPCache::lock()
{
    auto path = m_cacheDirPath->toUTF8NonGCString();
    if ((m_lockfd = open(path.data(), O_RDONLY)) != -1) {
        if (flock(m_lockfd, LOCK_EX | LOCK_NB) != -1) {
            STARFISH_LOG_INFO("[HTTPCache] Lock cache dir\n");
            return true;
        }
    }
    STARFISH_LOG_ERROR("[HTTPCache] Failed to lock cache dir\n");
    return false;
}

void HTTPCache::unlock()
{
    if (m_lockfd != -1) {
        if (flock(m_lockfd, LOCK_UN) != -1) {
            close(m_lockfd);
            STARFISH_LOG_INFO("[HTTPCache] Unlock cache dir\n");
        } else {
            STARFISH_LOG_ERROR("[HTTPCache] Failed to unlock cache dir\n");
        }
    }
}

bool HTTPCache::createOrOpenCacheDir()
{
    Directory* dir = Directory::create();
    if (dir->open(m_cacheDirPath)) {
        return dir->close();
    } else {
        return dir->mkDir();
    }
}

void HTTPCache::clear()
{
    clearCacheDir();
}

void HTTPCache::clearCacheDir()
{
    Directory* dir = Directory::create();
    dir->open(m_cacheDirPath);
    dir->clearDir();
    dir->close();
}

bool HTTPCache::initFromIndexFileIfPossible()
{
    STARFISH_ASSERT(isMainThread());

    File* in = File::create();

    if (!in->open(m_indexFilePath, File::FileMode::Read)) {
        return false;
    }

    Nullable<String*> data = in->readAll();
    in->removeFile();

    if (!data.hasValue()) {
        return false;
    }

    String* index = data.getValue();
    GCVector<StringView> table;

    StringUtils::tokenize(index, "\n", 1, table);

    Directory* dir = Directory::create();
    if (dir->open(m_cacheDirPath)) {
        if (dir->fileCount() != (table.size() - 1)) {
            dir->close();
            return false;
        }
    }
    dir->close();

    if (table.size() <= 1) {
        return false;
    }

    // Last line is "\n"
    for (auto row = table.begin(); row != table.end() - 1; row++) {
        GCVector<StringView> columns;
        StringUtils::tokenize(&(*row), HTTPCacheEntry::kSeparator, 1, columns);

        if (columns.size() != NUM_OF_COL) {
            STARFISH_LOG_ERROR("[HTTPCache] Index file is corrupted\n");
            return false;
        }
        // TODO : Check whether each column is valid or not

        // entryKey(UINT) urlString(STRING) no-cache(0|1) mustRevalidate(0|1)
        // maxAge(UINT) contentLanguage(STRING) contentLength(UINT)
        // contentType(STRING) contentTransferEncoding(STRING) date(UINT)
        // age(UINT) rquestTime(UINT) responeTime(UINT) lastModified(UINT)
        // Etag(STRING) entryFilePath(STRING) lastModificationTime(UINT)
        // byteLength(UINT)

        auto tempStr = columns[1].toUTF8NonGCString();
        String* urlString = String::fromUTF8(tempStr.data(), tempStr.length());
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

        // 15~17
        EntryFileInfo einfo;
        einfo.entryFilePath = columns[15].toUTF8NonGCString();
        einfo.lastModificationTime = String::parseInt64(&columns[16]);
        einfo.byteLength = String::parseInt64(&columns[17]);

        HTTPCacheEntry* newEntry =
            new HTTPCacheEntry(url, cc, cinfo, finfo, einfo);

        if (!newEntry->isConsistent()) {
            STARFISH_LOG_ERROR("[HTTPCache] Cached entries are corrupted");
            return false;
        }

        m_cacheEntryTable.insert(
            std::pair<size_t, HTTPCacheEntry*>(newEntry->entryKey(), newEntry));

        m_cacheLRUList.push_back(urlString);

        m_currentTotalSizeOfBlocks += calcBlocksSize(einfo.byteLength);
    }

    expire();
    return true;
}

HTTPCacheEntryMultiMap::iterator HTTPCache::get(ResourceURL* url)
{
    STARFISH_ASSERT(isMainThread());

    if (m_cacheMode == LOAD_NO_CACHE) {
        return m_cacheEntryTable.end();
    }

    String* item = url->urlString();

    auto entryItr = findEntryInCacheEntryTable(item);
    if (entryItr == m_cacheEntryTable.end() || !entryItr->second->canUse()) {
        return m_cacheEntryTable.end();
    }

    auto it = findItemInLRUList(item);
    STARFISH_ASSERT(it != m_cacheLRUList.end());

    m_cacheLRUList.erase(it);
    m_cacheLRUList.push_back(item);

    return entryItr;
}

HTTPCacheEntryMultiMap::iterator HTTPCache::findEntryInCacheEntryTable(
    String* key)
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

    if (nwd->cachedEntry || m_cacheMode == LOAD_NO_CACHE) {
        return;
    }

    auto check = findEntryInCacheEntryTable(nwd->request->url()->urlString());
    if (check != end()) {
        return;
    }

    CacheControl cc;
    HTTPContentInfo cinfo;
    HTTPFreshnessInfo finfo;

    extractHTTPCacheEntryProperty(nwd, cc, cinfo, finfo);

    if (cinfo.contentLength == 0) {
        return;
    }

    size_t sizeOfBlocks = calcBlocksSize(nwd->request->response().size());

    if (sizeOfBlocks > MAX_ENTRY_FILE_SIZE) {
        return;
    }

    if (cc.noStore || (cc.maxAge == 0 && finfo.etag.size() == 0)) {
        return;
    }

    if (!pruneAsNeededForCacheSpace(sizeOfBlocks)) {
        return;
    }

    HTTPCacheEntry* newEntry =
        new HTTPCacheEntry(nwd->request->url(), cc, cinfo, finfo);

    newEntry->setEntryFileNameUsingCachePath(m_cacheDirPath);

    if (!newEntry->writeRawDataToEntryFile(nwd->request->response())) {
        STARFISH_LOG_ERROR("[HTTPCache] Failed to write RawData\n");
        return;
    }

    m_cacheEntryTable.insert(
        std::pair<size_t, HTTPCacheEntry*>(newEntry->entryKey(), newEntry));
    m_cacheLRUList.push_back(newEntry->url()->urlString());
    m_currentTotalSizeOfBlocks += sizeOfBlocks;

    STARFISH_LOG_INFO("[HTTPCache] Current size : %.2lf\n",
                      (double)m_currentTotalSizeOfBlocks / (1024 * 1024));
}

void HTTPCache::update(NetworkURLWorkerData* nwd, HTTPCacheEntry* entry)
{
    if (m_cacheMode == LOAD_NO_CACHE) {
        return;
    }

    if (findEntryInCacheEntryTable(entry->url()->urlString()) == end()) {
        put(nwd);
        return;
    }

    size_t old = calcBlocksSize(entry->entryFileInfo().byteLength);
    if (entry->needsPropertiesUpdate()) {
        entry->setNeedsPropertiesUpdate(false);

        CacheControl cc;
        HTTPContentInfo cinfo;
        HTTPFreshnessInfo finfo;

        extractHTTPCacheEntryProperty(nwd, cc, cinfo, finfo);

        entry->setCacheControl(cc);
        entry->setHTTPContentInfo(cinfo);
        entry->setHTTPFreshnessInfo(finfo);
    }

    if (entry->needsRawDataUpdate()) {
        entry->setNeedsRawDataUpdate(false);
        size_t sizeOfBlocks = calcBlocksSize(entry->entryFileInfo().byteLength);
        if (!pruneAsNeededForCacheSpace(sizeOfBlocks)) {
            // TODO
            STARFISH_ASSERT_NOT_REACHED();
            return;
        }

        if (!entry->writeRawDataToEntryFile(nwd->request->response())) {
            STARFISH_LOG_ERROR("[HTTPCache] Failed to write RawData\n");
            // TODO
            STARFISH_ASSERT_NOT_REACHED();
            return;
        }
        m_currentTotalSizeOfBlocks += (sizeOfBlocks - old);
    }
}

void HTTPCache::extractHTTPCacheEntryProperty(NetworkURLWorkerData* nwd,
                                              CacheControl& cc,
                                              HTTPContentInfo& cinfo,
                                              HTTPFreshnessInfo& finfo)
{
    const HeaderMap& headerMap = nwd->request->responseHeaderMap();

    // TODO : Change initial value(ex: -1 or using string) of max-age and then
    // modify freshness calculation algorithm appropriately
    auto it = headerMap.find(HTTPHeaderMap::kCacheControl);
    if (it != headerMap.end()) {
        cc = HTTPUtil::parseCacheControl(it->second);
    }

    cinfo = HTTPUtil::getHTTPContentInfoFromHeaders(headerMap);

    finfo = HTTPUtil::getHTTPFreshnessInfoFromHeaders(
        nwd->request->document()->scriptBindingInstance(), headerMap);

    finfo.requestTime = nwd->httpTransaction->httpRequest().requestTime();
    finfo.responseTime = nwd->httpTransaction->httpResponse().responseTime();
}

bool HTTPCache::flush()
{
    STARFISH_ASSERT(isMainThread());
    STARFISH_LOG_INFO("HTTPCache::flush()\n");

    if (g_httpCacheCNT > 1) {
        g_httpCacheCNT--;
        return false;
    }

    bool check = true;

    expire();

    if (!pruneAsNeededForCacheSpace(calcBlocksSizeOfIndexFile())) {
        STARFISH_LOG_ERROR(
            "[HTTPCache] Failed to reserve free space to index files\n");
        check = false;
    }

    if (!check || !isConsistent()) {
        STARFISH_LOG_ERROR("[HTTPCache] Cached entries are corrupted")
        clearCacheDir();
        return false;
    }

    STARFISH_LOG_INFO("[HTTPCache] Current size : %.2lf\n",
                      (double)m_currentTotalSizeOfBlocks / (1024 * 1024));

    File* out = File::create();
    if (!out->open(m_indexFilePath, File::FileMode::Write)) {
        return false;
    }

    for (auto it : m_cacheLRUList) {
        auto tableItr = findEntryInCacheEntryTable(it);
        HTTPCacheEntry* entry = tableItr->second;
        if (!out->writeLine(entry->toString())) {
            out->close();
            return false;
        }
    }

    unlock();
    return (out->flush() == 0) & (out->close() == 0);
}

bool HTTPCache::pruneAsNeededForCacheSpace(const size_t reserve)
{
    STARFISH_ASSERT(isMainThread());

    // Consider indexFile size
    size_t removedSize = 0;

    if (m_currentTotalSizeOfBlocks + reserve > m_cacheSizeLimit) {
        auto iter = m_cacheLRUList.begin();
        while (iter != m_cacheLRUList.end() && removedSize < reserve) {
            auto tableIter = findEntryInCacheEntryTable(*iter);
            HTTPCacheEntry* cacheEntry = tableIter->second;

            if (cacheEntry->usingCount() > 0) {
                iter++;
                continue;
            }

            File* fio = File::create();
            if (fio->open(cacheEntry->entryFileInfo().entryFilePath,
                          File::Read)) {
                auto info = cacheEntry->entryFileInfo();
                size_t sizeOfBlock = calcBlocksSize(info.byteLength);
                fio->removeFile();
                fio->close();

                m_currentTotalSizeOfBlocks -= sizeOfBlock;
                removedSize += sizeOfBlock;
            }
            m_cacheEntryTable.erase(tableIter);
            iter = m_cacheLRUList.erase(iter);
        }
        STARFISH_LOG_INFO(
            "[HTTPCache] Reserve : %.2lf, Prune : %.2lf, Current size : "
            "%.2lf\n",
            (double)reserve / (1024 * 1024),
            (double)removedSize / (1024 * 1024),
            (double)m_currentTotalSizeOfBlocks / (1024 * 1024));
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

    if (!dir->open(m_cacheDirPath) ||
        (dir->fileCount()) != m_cacheLRUList.size()) {
        STARFISH_LOG_ERROR("[HTTPCache] Cache dir status is inconsistent\n");
        dir->close();
        return false;
    }

    dir->close();

    for (auto& it : m_cacheEntryTable) {
        if (!it.second->isConsistent()) {
            STARFISH_LOG_ERROR("[HTTPCache] Entry status is inconsistent\n");
            return false;
        }
    }

    return true;
}

void HTTPCache::expire()
{
    STARFISH_ASSERT(isMainThread());

    for (auto it = m_cacheEntryTable.begin(); it != m_cacheEntryTable.end();) {
        if (it->second->shouldExpire()) {
            File* fio = File::create();
            fio->open(it->second->entryFileInfo().entryFilePath, File::Read);

            if (fio->isOpen()) {
                fio->removeFile();
            }

            fio->close();

            removeItemInLRUList(it->second->url()->urlString());
            it = m_cacheEntryTable.erase(it);
        } else {
            it++;
        }
    }
}

void HTTPCache::init()
{
    HTTPCacheEntryMultiMap().swap(m_cacheEntryTable);
    HTTPCacheLRUList().swap(m_cacheLRUList);
    m_currentTotalSizeOfBlocks = 0;
}

HTTPCacheLRUList::iterator HTTPCache::findItemInLRUList(String* item)
{
    auto it = m_cacheLRUList.begin();
    while (it != m_cacheLRUList.end()) {
        if ((*it)->equals(item)) {
            return it;
        } else {
            it++;
        }
    }
    return m_cacheLRUList.end();
}

void HTTPCache::removeItemInLRUList(String* item)
{
    auto it = findItemInLRUList(item);
    if (it != m_cacheLRUList.end()) {
        m_cacheLRUList.erase(it);
    }
}

size_t HTTPCache::calcBlocksSize(size_t length)
{
    return (length / kBlockSize + ((length % kBlockSize) ? 1 : 0)) * kBlockSize;
}

size_t HTTPCache::calcBlocksSizeOfIndexFile()
{
    size_t bytes = 0;
    for (auto it : m_cacheLRUList) {
        auto tableIter = findEntryInCacheEntryTable(it);
        HTTPCacheEntry* entry = tableIter->second;
        bytes += entry->toString()->toUTF8NonGCString().size() + 1;
    }
    return calcBlocksSize(bytes);
}
}

#endif
