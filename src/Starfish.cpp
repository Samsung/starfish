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

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/modules/threading/Locker.h"
#include "core/modules/threading/Thread.h"
#include "core/modules/threading/ThreadPool.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/message_loop/Timer.h"
#include "core/util/LineBreakerIteratorPool.h"
#include "core/modules/profiling/Profiling.h"
#include "core/modules/worker/WorkerManager.h"

#include "LWEWebView.h"
#ifdef STARFISH_ENABLE_HTTPCACHE
#include "platform/network/http/HTTPCache.h"
#endif

#include "core/util/Id.h"
#include "platform/process/base/ProcessType.h"
#include "platform/network/curl/NetworkSharedResourceManager.h"

namespace Starfish {

#if defined(STARFISH_ENABLE_TEST) || defined(STARFISH_ENABLE_PROFILE)
bool g_fireOnloadEvent = false;
#endif

#ifdef STARFISH_ENABLE_PROFILE
Profiler g_profiler;
#endif

static bool g_starfishGlobalInit = false;

Starfish::Starfish(const StarfishConfiguration& config)
    : m_localStorageDataFilePath(
          String::fromUTF8(config.localStorageDataFilePath,
                           strlen(config.localStorageDataFilePath)))
#ifdef STARFISH_ENABLE_HTTPCACHE
    , m_httpCache(nullptr)
#endif
    , m_webViewInstanceCount(0)
    , m_gcFrequency(config.gcFrequency)
    , m_backend(config.backend)
    , m_rendererType(config.rendererType)
{
    STARFISH_RELEASE_ASSERT(config.localStorageDataFilePath != nullptr);
    STARFISH_RELEASE_ASSERT(config.cookieStoreDataFilePath != nullptr);
    STARFISH_RELEASE_ASSERT(config.httpCacheDataDirectorypath != nullptr);

    if (!g_starfishGlobalInit) {
        g_starfishGlobalInit = true;

        GC_set_abort_func([](const char* msg) {
            STARFISH_LOG_ERROR("Starfish: GC aborted");
            STARFISH_LOG_ERROR("%s", msg);
        });

        GC_set_warn_proc([](char* msg, GC_word arg) {
            STARFISH_LOG_ERROR("Starfish: GC warning");
            STARFISH_LOG_ERROR("%s", msg);
        });

        GC_set_free_space_divisor(m_gcFrequency);
        GC_set_force_unmap_on_gcollect(1);
    }

#ifndef STARFISH_LINE_BREAK_ITERATOR_POOL_SIZE
#define STARFISH_LINE_BREAK_ITERATOR_POOL_SIZE 4
#endif
    m_lineBreakIteratorPool =
        new LineBreakIteratorPool(STARFISH_LINE_BREAK_ITERATOR_POOL_SIZE);
    m_atomicStringMap.insert(String::emptyString);
    m_staticStrings = new StaticStrings(this);

    initNetworkSharedResourceManager(config.cookieStoreDataFilePath);
#ifdef STARFISH_ENABLE_HTTPCACHE
    if (strlen(config.httpCacheDataDirectorypath) != 0) {
        auto nullable = HTTPCache::getInstance(
            (String::fromUTF8(config.httpCacheDataDirectorypath,
                              strlen(config.httpCacheDataDirectorypath))));
        if (nullable.hasValue()) {
            m_httpCache = nullable.getValue();
        }
    }
#endif

#if defined(STARFISH_USE_WORKER_PROCESS)
    m_workerManager = WorkerManager::create();
#endif

#ifdef STARFISH_ENABLE_PROFILE
    g_profiler.start();
#endif
}

void* Starfish::operator new(size_t size)
{
    static thread_local bool typeInited = false;
    static thread_local GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(Starfish)] = { 0 };

        GC_set_bit(desc, GC_WORD_OFFSET(Starfish, m_staticStrings));
        GC_set_bit(desc, GC_WORD_OFFSET(Starfish, m_lineBreakIteratorPool));
        GC_set_bit(desc, GC_WORD_OFFSET(Starfish, m_localStorageDataFilePath));
        markHashTable(desc, GC_WORD_OFFSET(Starfish, m_rootMap));
        markHashTable(desc, GC_WORD_OFFSET(Starfish, m_atomicStringMap));
        markHashTable(desc, GC_WORD_OFFSET(Starfish, m_caseInsensitiveAttrSet));

#if defined(STARFISH_ENABLE_HTTPCACHE)
        GC_set_bit(desc, GC_WORD_OFFSET(Starfish, m_httpCache));
#endif
#if defined(STARFISH_USE_WORKER_PROCESS)
        GC_set_bit(desc, GC_WORD_OFFSET(Starfish, m_workerManager));
#endif

        descr = GC_make_descriptor(desc, GC_WORD_LEN(Starfish));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void Starfish::destroy()
{
    STARFISH_LOG_INFO("Starfish::destroy");
    NetworkSharedResourceManager::destroy();

#if !defined(STARFISH_WEBWORKER_HOST)
    LWE::CookieManager::Destroy();
#endif

#ifdef STARFISH_ENABLE_HTTPCACHE
    if (m_httpCache) {
        m_httpCache->flush();
        m_httpCache = nullptr;
    }
#endif

#if defined(STARFISH_USE_WORKER_PROCESS)
    if (m_workerManager) {
        m_workerManager->destroy();
        m_workerManager = nullptr;
    }
#endif

    delete m_lineBreakIteratorPool;
    m_lineBreakIteratorPool = nullptr;

    this->Starfish::~Starfish();
    GC_FREE(this);
}

void Starfish::initNetworkSharedResourceManager(
    const char* cookieStoreDataFilePath)
{
    // NetworkSharedResourceManager is singleton, So do not hold the instance.
    if (cookieStoreDataFilePath) {
        // Disable to store cookies as a file If m_cookieStoreDataFilePath is
        // nullptr or empty string
        NetworkSharedResourceManager::getInstance()->setCookieStoreFilePath(
            cookieStoreDataFilePath);
    }
    NetworkSharedResourceManager::getInstance()->initCookieSession();
}

void Starfish::addPointerInRootSet(void* ptr)
{
    auto iter = m_rootMap.find(ptr);
    if (iter == m_rootMap.end()) {
        m_rootMap.insert(std::make_pair(ptr, 1));
    } else {
        iter.value()++;
    }
}

void Starfish::removePointerFromRootSet(void* ptr)
{
    auto iter = m_rootMap.find(ptr);
    if (iter != m_rootMap.end()) {
        if (iter->second == 1) {
            m_rootMap.erase(iter);
        } else {
            iter.value()--;
        }
    }
}

void Starfish::version(int* major, int* minor, int* patch)
{
    std::stringstream ss(STARFISH_VERSION_STR);
    int mj, mn, p;
    char dot;
    ss >> mj >> dot >> mn >> dot >> p;

    if (major) {
        *major = mj;
    }
    if (minor) {
        *minor = mn;
    }
    if (patch) {
        *patch = p;
    }
}

#ifndef NDEBUG
size_t Starfish::countPointersInRootSet(void* ptr)
{
    auto iter = m_rootMap.find(ptr);
    if (iter != m_rootMap.end()) {
        return iter->second;
    } else {
        return 0;
    }
}
#endif

void Starfish::doFullGCWithoutSeeingStack()
{
    GC_register_mark_stack_func([]() {
        // do nothing for skip stack
        // assume there is no gc-object on stack
    });
    GC_gcollect();
    GC_gcollect();
    GC_gcollect_and_unmap();
    GC_register_mark_stack_func(nullptr);
}

void Starfish::printEveryReachableGCObjects()
{
    STARFISH_LOG_ERROR("print reachable pointers -->");
    GC_gcollect();
    GC_disable();
    GC_enumerate_reachable_objects_inner(
        [](void* obj, size_t bytes, void* cd) {
            size_t size;
            int kind = GC_get_kind_and_size(obj, &size);
            STARFISH_ASSERT(size == bytes);
            void* ptr = GC_USR_PTR_FROM_BASE(obj);
            STARFISH_LOG_ERROR("@@@ kind %d pointer %p", (int)kind, ptr);
#if !defined(NDEBUG) && (!defined(OS_WINDOWS) && !defined(STARFISH_ANDROID) && \
                         !defined(STARFISH_WEBWORKER_HOST))
            GC_print_backtrace(ptr);
#endif
        },
        nullptr);
    GC_enable();
    STARFISH_LOG_ERROR("<-- end of print reachable pointers");
}

#ifdef STARFISH_ENABLE_HTTPCACHE
Nullable<HTTPCache*> Starfish::httpCache()
{
    // HTTPCache is supported only on the window.
    if (!isMainThread() || !m_httpCache) {
        return Nullable<HTTPCache*>();
    }

    return Nullable<HTTPCache*>(m_httpCache);
}
#endif
} // namespace Starfish
