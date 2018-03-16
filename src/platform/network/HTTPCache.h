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
#ifndef __StarFishCache_
#define __StarFishCache_

#include "HTTPCacheEntry.h"

namespace StarFish {
typedef GCVector<String*> HTTPCacheLRUList;
class NetworkURLWorkerData;
class File;

class HTTPCache : public gc {
public:
    // cache mode
    static const int LOAD_DEFAULT = -1;
    static const int LOAD_NORMAL = 0;
    static const int LOAD_CACHE_ELSE_NETWORK = 1;
    static const int LOAD_NO_CACHE = 2;
    static const int LOAD_CACHE_ONLY = 3;

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
    void clear();
    void setCacheMode(int mode)
    {
        if (mode == LOAD_DEFAULT || mode == LOAD_NO_CACHE) {
            m_cacheMode = mode;
        } else {
            STARFISH_ASSERT_NOT_REACHED();
        }
    }

    int cacheMode()
    {
        return m_cacheMode;
    }

    bool good()
    {
        return m_good;
    }

    static const size_t kBlockSize;

private:
    bool lock();
    void unlock();
    bool createOrOpenCacheDir();
    void clearCacheDir();
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
    size_t m_cacheSizeLimit;
    size_t m_currentTotalSizeOfBlocks;
    int m_lockfd;
    bool m_good;
    int m_cacheMode;
};
}
#endif
#endif
