/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
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
#ifndef __StarfishCache_
#define __StarfishCache_

#include "HTTPCacheEntry.h"

namespace Starfish {
typedef GCVector<String*> HTTPCacheLRUList;
struct NetworkURLWorkerData;
class File;
class HTTPCache;

class HTTPCache : public gc {
public:
    // cache mode
    static const int LOAD_DEFAULT = -1;
    static const int LOAD_NORMAL = 0;
    static const int LOAD_CACHE_ELSE_NETWORK = 1;
    static const int LOAD_NO_CACHE = 2;
    static const int LOAD_CACHE_ONLY = 3;

    static Nullable<HTTPCache*> getInstance(String* cacheDirPath)
    {
        auto httpCache = new HTTPCache(cacheDirPath);
        return (httpCache->good()) ? Nullable<HTTPCache*>(httpCache)
                                   : Nullable<HTTPCache*>();
    }

    HTTPCache(String* cacheDirPath);
    ~HTTPCache();
    bool initFromIndexFileIfPossible();
    Nullable<HTTPCacheEntry*> get(ResourceURL* url);

    void put(NetworkURLWorkerData* data);
    void update(NetworkURLWorkerData* nwd, HTTPCacheEntry* entry);
    void remove(HTTPCacheEntry* entry);
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

    void* operator new(size_t size)
    {
        STARFISH_ASSERT(size == sizeof(HTTPCache));
        static bool typeInited = false;
        static GC_descr descr;
        if (!typeInited) {
            GC_word desc[GC_BITMAP_SIZE(HTTPCache)] = { 0 };
            fillGCDescriptor(desc);
            descr = GC_make_descriptor(desc, GC_WORD_LEN(HTTPCache));
            typeInited = true;
        }
        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
    }

    static inline void fillGCDescriptor(GC_word* desc)
    {
        GC_set_bit(desc, GC_WORD_OFFSET(HTTPCache, m_cacheEntryTable));
        GC_set_bit(desc, GC_WORD_OFFSET(HTTPCache, m_cacheLRUList));
        GC_set_bit(desc, GC_WORD_OFFSET(HTTPCache, m_cacheDirPath));
        GC_set_bit(desc, GC_WORD_OFFSET(HTTPCache, m_indexFilePath));
    }

    static const size_t kBlockSize;

private:
    bool lock();
    void unlock();
    bool createOrOpenCacheDir();
    void clearCacheDir();
    void init();
    void removeItemInLRUList(String* url);
    void insertToCacheEntryTable(
        const std::pair<size_t, RefPtr<HTTPCacheEntry>>& pair);
    void removeFromCacheEntryTable(HTTPCacheEntry* entry);
    Nullable<HTTPCacheEntry*> findEntryInCacheEntryTable(String* key);
    HTTPCacheLRUList::iterator findItemInLRUList(String* item);
    void extractHTTPCacheEntryProperty(NetworkURLWorkerData* nwd,
                                       CacheControl& cc, HTTPContentInfo& cinfo,
                                       HTTPFreshnessInfo& finfo);

    size_t calcBlocksSize(size_t length);
    size_t calcBlocksSizeOfIndexFile();
    HTTPCacheEntryMap* m_cacheEntryTable;
    HTTPCacheLRUList m_cacheLRUList;
    String* m_cacheDirPath;
    String* m_indexFilePath;
    size_t m_cacheSizeLimit;
    size_t m_currentTotalSizeOfBlocks;
    int m_lockfd;
    bool m_good;
    int m_cacheMode;
};
} // namespace Starfish
#endif
#endif
