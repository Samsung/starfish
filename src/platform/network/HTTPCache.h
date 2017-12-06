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

class HTTPCache : public gc {
public:
    HTTPCache(String* cacheDirPath);
    ~HTTPCache();
    bool initFromIndexFileIfPossible();
    HTTPCacheEntryMultiMap::iterator get(ResourceURL* url);
    HTTPCacheEntryMultiMap::iterator end()
    {
        return m_cacheEntryTable.end();
    }

    void put(NetworkURLWorkerData* data);
    bool flush();
    void expire();
    bool pruneAsNeededForCacheSpace(const size_t reserve);
    bool isConsistent();

private:
    void initCacheDirectory();
    void initCacheMeber();
    void addCacheLRUListData(String* url);
    void deleteCacheLRUListData(String* url);
    HTTPCacheEntryMultiMap::iterator findEntry(String* key);

    HTTPCacheEntryMultiMap m_cacheEntryTable;
    HTTPCacheLRUList m_cacheLRUList;
    String* m_cacheDirPath;
    String* m_indexFilePath;
    size_t m_currentCacheSize;
    size_t m_cacheSize;
};
}
#endif
#endif
