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
#ifndef __StarFishCache_
#define __StarFishCache_

#include "HTTPCacheEntry.h"

namespace StarFish {
typedef GCVector<String*> HTTPCacheLRUList;
class NetworkURLWorkerData;
class File;

class HTTPCache : public gc {
public:
    static Nullable<HTTPCache*> create(String* cacheDirPath)
    {
        HTTPCache* newObject = new HTTPCache(cacheDirPath);
        return (newObject->good()) ? Nullable<HTTPCache*>(newObject)
                                   : Nullable<HTTPCache*>();
    }

    HTTPCache(String* cacheDirPath);
    ~HTTPCache();
    bool initFromIndexFileIfPossible();
    HTTPCacheEntryMultiMap::iterator get(ResourceURL* url);
    HTTPCacheEntryMultiMap::iterator end()
    {
        return m_cacheEntryTable.end();
    }

    void put(NetworkURLWorkerData* data);
    void update(NetworkURLWorkerData* nwd, HTTPCacheEntry* entry);
    bool flush();
    void expire();
    bool pruneAsNeededForCacheSpace(const size_t reserve);
    bool isConsistent();

    bool good()
    {
        return m_good;
    }

    static const size_t kBlockSize;

private:
    bool lock();
    void unlock();
    bool initCacheDirectory();
    void init();
    void removeItemInLRUList(String* url);
    HTTPCacheEntryMultiMap::iterator findEntryInCacheEntryTable(String* key);
    HTTPCacheLRUList::iterator findItemInLRUList(String* item);
    void extractHTTPCacheEntryProperty(NetworkURLWorkerData* nwd,
                                       CacheControl& cc, HTTPContentInfo& cinfo,
                                       HTTPFreshnessInfo& finfo);

    size_t calcBlocksSize(size_t length);
    size_t calcBlocksSizeOfIndexFile();
    HTTPCacheEntryMultiMap m_cacheEntryTable;
    HTTPCacheLRUList m_cacheLRUList;
    String* m_cacheDirPath;
    String* m_indexFilePath;
    String* m_lockFilePath;
    size_t m_cacheSizeLimit;
    size_t m_currentTotalSizeOfBlocks;
    int m_lockfd;
    bool m_good;
};
}
#endif
#endif
