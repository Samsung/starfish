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
#include "StoragePathProvider.h"

namespace Starfish {

class Thread;
class NativeImageData;
class LineBreakIteratorPool;
class Mutex;
class WorkerManager;
class Profiler;

#if defined(STARFISH_ENABLE_HTTPCACHE)
class HTTPCache;
#endif

#ifndef BDWGC_FREE_SPACE_DIVISOR
#define BDWGC_FREE_SPACE_DIVISOR 12
#endif

enum class StarfishRendererType {
    kOpenGL,
    kSoftware,
    kHeadless,
};

struct StarfishConfiguration {
    const char* storageDirectoryPath = nullptr;
    unsigned char gcFrequency = BDWGC_FREE_SPACE_DIVISOR;
    bool isThreadMode = false;
    const char* backend = nullptr;
    StarfishRendererType rendererType = StarfishRendererType::kOpenGL;
};

// ctor of Starfish class is NOT THREAD-SAFE
class Starfish : public gc {
    friend class AtomicString;
    friend class StaticStrings;
    friend class WebView;
    friend class HTMLDocument; // m_caseInsensitiveAttrSet
public:
    Starfish(const StarfishConfiguration& config);

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    void destroy();

    StaticStrings* staticStrings()
    {
        return m_staticStrings;
    }

    LineBreakIteratorPool* lineBreakIteratorPool()
    {
        return m_lineBreakIteratorPool;
    }

    String* localStorageFilePath();

#ifdef STARFISH_ENABLE_HTTPCACHE
    Nullable<HTTPCache*> httpCache();
#endif

    size_t webViewInstanceCount()
    {
        return m_webViewInstanceCount;
    }

    unsigned char gcFrequency()
    {
        return m_gcFrequency;
    }

    void setGCFrequency(unsigned char c)
    {
        m_gcFrequency = c;
        GC_set_free_space_divisor(c);
    }

    void version(int* major, int* minor, int* patch);

    std::string backend()
    {
        return m_backend;
    }

    StarfishRendererType rendererType()
    {
        return m_rendererType;
    }

    void addPointerInRootSet(void* ptr);
    void removePointerFromRootSet(void* ptr);
#ifndef NDEBUG
    size_t countPointersInRootSet(void* ptr);
#endif

    static void doFullGCWithoutSeeingStack();
    static void printEveryReachableGCObjects();

protected:
    StaticStrings* m_staticStrings = nullptr;
    LineBreakIteratorPool* m_lineBreakIteratorPool = nullptr;
    StoragePathProvider m_storagePathProvider;
    GCUnorderedMap<void*, size_t> m_rootMap;
    AtomicStringMap m_atomicStringMap;
    GCUnorderedMap<String*, size_t> m_caseInsensitiveAttrSet;

#ifdef STARFISH_ENABLE_HTTPCACHE
    HTTPCache* m_httpCache = nullptr;
#endif
#if defined(STARFISH_USE_WORKER_PROCESS)
public:
    WorkerManager* workerManager()
    {
        return m_workerManager;
    }

protected:
    WorkerManager* m_workerManager = nullptr;
#endif // STARFISH_ENABLE_SERVICE_WORKER

    size_t m_webViewInstanceCount = 0;
    unsigned char m_gcFrequency = BDWGC_FREE_SPACE_DIVISOR;
    std::string m_backend;
    StarfishRendererType m_rendererType = StarfishRendererType::kOpenGL;

private:
    void initNetworkSharedResourceManager();
};

#if defined(STARFISH_ENABLE_TEST) || defined(STARFISH_ENABLE_PROFILE)
extern bool g_fireOnloadEvent;
#endif

#ifdef STARFISH_ENABLE_PROFILE
extern Profiler g_profiler;
#endif

} // namespace Starfish

#endif
