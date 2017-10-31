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
class NetworkURLWorkerData;

struct CacheControl {
    CacheControl()
        : noCache(false)
        , noStore(false)
        , mustRevalidate(false)
        , maxAge(0)
    {
    }

    bool noCache : 1;
    bool noStore : 1;
    bool mustRevalidate : 1;
    time_t maxAge;
};

class HTTPCache : public gc {
public:
    HTTPCache(String* cacheDirPath);
    ~HTTPCache();
    void initFromIndexFileIfPossible();
    bool cacheHit(ResourceURL* url);
    void caching(NetworkURLWorkerData* data);
    bool flush();
    // expire
    // prunning
    // consistency checking

private:
    void initCacheDir();
    void clearAndRemoveCacheDir();
    CacheControl parseCacheControl(std::string directives);

    HTTPCacheEntryMultiMap m_cacheEntryTable;
    String* m_cacheDirPath;
    String* m_indexFilePath;
};
}
#endif
#endif
