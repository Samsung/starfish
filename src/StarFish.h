/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
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

#ifndef __StarFish__
#define __StarFish__

#include "StaticStrings.h"
namespace StarFish {

class Thread;
class PlatformWindow;
class NativeImageData;
class LineBreakIteratorPool;
class Mutex;
#if defined(STARFISH_ENABLE_HTTPCACHE)
class HTTPCache;
#endif

void addGCCollectionListener(void (*fn)(GC_EventType));

// ctor of StarFish class is NOT THREAD-SAFE
class StarFish : public gc {
    friend class AtomicString;
    friend class StaticStrings;
    friend class WebView;
    friend class HTMLDocument; // m_caseInsensitiveAttrSet
public:
    StarFish(const char* localStorageFilePath, const char* cookieStoreFilePath,
             const char* httpCacheDirectorypath);

    void destroy();

    StaticStrings* staticStrings()
    {
        return m_staticStrings;
    }

    LineBreakIteratorPool* lineBreakIteratorPool()
    {
        return m_lineBreakIteratorPool;
    }

    String* localStorageFilePath()
    {
        return m_localStorageFilePath;
    }

#ifdef STARFISH_ENABLE_HTTPCACHE
    HTTPCache* httpCache()
    {
        return m_httpCache;
    }

#endif

    size_t webViewInstanceCount()
    {
        return m_webViewInstanceCount;
    }

    void addPointerInRootSet(void* ptr);
    void removePointerFromRootSet(void* ptr);
#ifndef NDEBUG
    size_t countPointersInRootSet(void* ptr);
#endif

    void updateProfileRecord(String* tag, float time)
    {
        auto it = m_profilingRecods.find(tag);
        if (it == m_profilingRecods.end()) {
            m_profilingRecods.insert(std::make_pair(tag, time));
        } else {
            it->second = time;
        }
    }

    float profileRecode(String* tag) const
    {
        auto it = m_profilingRecods.find(tag);
        if (it == m_profilingRecods.end()) {
            return 0;
        }

        return it->second;
    }

    static void doFullGCWithoutSeeingStack();
    static void printEveryReachableGCObjects();

protected:
    StaticStrings* m_staticStrings;
    LineBreakIteratorPool* m_lineBreakIteratorPool;
    String* m_localStorageFilePath;
    GCUnorderedMap<void*, size_t> m_rootMap;
    AtomicStringMap m_atomicStringMap;
    GCUnorderedMap<String*, size_t> m_caseInsensitiveAttrSet;
#ifdef STARFISH_ENABLE_HTTPCACHE
    HTTPCache* m_httpCache;
#endif
    GCUnorderedMap<String*, float> m_profilingRecods;
    size_t m_webViewInstanceCount;

private:
    void initNetworkSharedResourceManager(const char* cookieStoreFilePath);
};

#ifdef STARFISH_ENABLE_TEST
extern bool g_enablePixelTest;
extern bool g_enableDumpAsText;
extern bool g_DumpAsText_Async;
extern int g_referenceTestState; // 0:None, 1:RunningTC, 2:RunningReference
#endif
}

#endif
