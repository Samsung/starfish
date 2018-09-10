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

#ifdef STARFISH_ENABLE_TEST
#include <unistd.h>
#include <ios>
#include <iostream>
#include <fstream>
#endif

#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/dom/Document.h"
#include "core/page/BrowsingContext.h"
#include "core/page/WebView.h"
#include "core/modules/threading/Locker.h"
#include "core/modules/threading/Thread.h"
#include "core/modules/threading/ThreadPool.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/page/Window.h"
#include "core/modules/message_loop/Timer.h"
#include "core/modules/canvas/image/NativeImageData.h"
#include "core/util/LineBreakerIteratorPool.h"
#include "LWEWebView.h"
#ifdef STARFISH_ENABLE_HTTPCACHE
#include "platform/network/HTTPCache.h"
#endif
#include "platform/network/NetworkSharedResourceManager.h"
#include "platform/window/PlatformWindow.h"

namespace StarFish {

#ifdef STARFISH_ENABLE_TEST
bool g_enablePixelTest = false;
bool g_enableDumpAsText = false;
bool g_DumpAsText_Async = false;
int g_referenceTestState = 0;
#endif

static bool g_starFishGlobalInit = false;
typedef void (*GCCollectionEventListenter)(GC_EventType);
static std::list<GCCollectionEventListenter> g_gcCollectionEventListenterList;
void addGCCollectionListener(void (*fn)(GC_EventType))
{
    g_gcCollectionEventListenterList.push_back(fn);
}

StarFish::StarFish(const char* localStorageFilePath,
                   const char* cookieStoreFilePath,
                   const char* httpCacheDirectorypath)
    : m_localStorageFilePath(String::fromUTF8(localStorageFilePath))
#ifdef STARFISH_ENABLE_HTTPCACHE
    , m_httpCache(nullptr)
#endif
    , m_webViewInstanceCount(0)
{
    registerMainThread();
    if (!g_starFishGlobalInit) {
        g_starFishGlobalInit = true;

        GC_set_abort_func([](const char* msg) {
            STARFISH_LOG_ERROR("StarFish: GC aborted\n");
            STARFISH_LOG_ERROR("%s\n", msg);
        });

        GC_set_warn_proc([](char* msg, GC_word arg) {
            STARFISH_LOG_ERROR("StarFish: GC warning\n");
            STARFISH_LOG_ERROR("%s\n", msg);
        });

        GC_set_on_collection_event([](GC_EventType evtType) {

            if (GC_EVENT_RECLAIM_END == evtType) {
                STARFISH_LOG_INFO("Done GC: HeapSize: [%f MB , %f MB]\n",
                                  GC_get_memory_use() / 1024.f / 1024.f,
                                  GC_get_heap_size() / 1024.f / 1024.f);
            }

            auto iter = g_gcCollectionEventListenterList.begin();
            while (iter != g_gcCollectionEventListenterList.end()) {
                (*iter)(evtType);
                iter++;
            }
        });
        GC_set_free_space_divisor(12);
        GC_set_force_unmap_on_gcollect(1);
    }

#ifndef STARFISH_LINE_BREAK_ITERATOR_POOL_SIZE
#define STARFISH_LINE_BREAK_ITERATOR_POOL_SIZE 4
#endif
    m_lineBreakIteratorPool =
        new LineBreakIteratorPool(STARFISH_LINE_BREAK_ITERATOR_POOL_SIZE);
    m_atomicStringMap.insert(String::emptyString);
    m_staticStrings = new StaticStrings(this);

    initNetworkSharedResourceManager(cookieStoreFilePath);
#ifdef STARFISH_ENABLE_HTTPCACHE
    if (httpCacheDirectorypath != nullptr) {
        auto nullable =
            HTTPCache::getInstance((String::fromUTF8(httpCacheDirectorypath)));
        if (nullable.hasValue()) {
            m_httpCache = nullable.getValue();
        }
    }
#endif
}

void StarFish::destroy()
{
    STARFISH_LOG_INFO("StarFish::destroy");
    NetworkSharedResourceManager::destroy();
#ifdef STARFISH_ENABLE_HTTPCACHE
    HTTPCache::destory();
#endif
}

void StarFish::initNetworkSharedResourceManager(const char* cookieStoreFilePath)
{
    // NetworkSharedResourceManager is singleton, So do not hold the instance.
    if (cookieStoreFilePath) {
        // Disable to store cookies as a file If m_cookieStoreFilePath is
        // nullptr or empty string
        NetworkSharedResourceManager::getInstance()->setCookieStoreFilePath(
            cookieStoreFilePath);
    }
    NetworkSharedResourceManager::getInstance()->initCookieSession();
}

void StarFish::addPointerInRootSet(void* ptr)
{
    auto iter = m_rootMap.find(ptr);
    if (iter == m_rootMap.end()) {
        m_rootMap.insert(std::make_pair(ptr, 1));
    } else {
        iter->second++;
    }
}

void StarFish::removePointerFromRootSet(void* ptr)
{
    auto iter = m_rootMap.find(ptr);
    if (iter != m_rootMap.end()) {
        if (iter->second == 1) {
            m_rootMap.erase(iter);
        } else {
            iter->second--;
        }
    }
}

#ifndef NDEBUG
size_t StarFish::countPointersInRootSet(void* ptr)
{
    auto iter = m_rootMap.find(ptr);
    if (iter != m_rootMap.end()) {
        return iter->second;
    } else {
        return 0;
    }
}
#endif
}
