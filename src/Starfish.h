/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
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

#ifndef __Starfish__
#define __Starfish__

#include "StaticStrings.h"
namespace Starfish {

class Thread;
class PlatformWindow;
class NativeImageData;
class LineBreakIteratorPool;
class Mutex;
class PerProcess;
class ServiceWorkerProcessManager;
class ServiceWorkerOption;
#if defined(STARFISH_ENABLE_HTTPCACHE)
class HTTPCache;
#endif

#ifndef BDWGC_FREE_SPACE_DIVISOR
#define BDWGC_FREE_SPACE_DIVISOR 6
#endif

// ctor of Starfish class is NOT THREAD-SAFE
class Starfish : public gc {
    friend class AtomicString;
    friend class StaticStrings;
    friend class WebView;
    friend class HTMLDocument; // m_caseInsensitiveAttrSet
public:
    Starfish(const char* localStorageFilePath, const char* cookieStoreFilePath,
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
    Nullable<HTTPCache*> httpCache();
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
#if defined(STARFISH_ENABLE_SERVICE_WORKER)
public:
    PerProcess* perProcess()
    {
        return m_perProcess;
    }

    ServiceWorkerOption* serviceWorkerOption()
    {
        return m_serviceWorkerOption;
    }

protected:
    PerProcess* m_perProcess{ nullptr };

#if !defined(STARFISH_WEBWORKER_HOST)
    ServiceWorkerProcessManager* m_serviceWorkerProcessManager{ nullptr };
#endif
    ServiceWorkerOption* m_serviceWorkerOption;

#endif // STARFISH_ENABLE_SERVICE_WORKER

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
extern bool g_starfishIgnoreSSLVerify;
} // namespace Starfish

#endif
