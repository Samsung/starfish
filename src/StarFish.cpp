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
#include "core/inspector/Inspector.h"
#include "core/extra/Console.h"
#include "core/style/ComputedStyle.h"
#include "core/util/LineBreakerIteratorPool.h"
#include "platform/file/File.h"
#include "LWEWebView.h"
#ifdef STARFISH_ENABLE_HTTPCACHE
#include "platform/network/HTTPCache.h"
#endif
#include "platform/network/NetworkSharedResourceManager.h"
#include "platform/window/PlatformWindow.h"
#if defined(STARFISH_TIZEN_TV) && defined(STARFISH_ENABLE_AVPLAY)
#include "core/extra/Avplay.h"
#endif
#ifdef STARFISH_ENABLE_TTS
#include "core/modules/tts/TTS.h"
#endif

#include <malloc.h>

#ifdef STARFISH_ENABLE_TEST
int g_testCompatibleMode;
int g_startUpFlag;
#endif

namespace StarFish {

#ifdef STARFISH_ENABLE_TEST
bool g_enablePixelTest = false;
bool g_enableDumpAsText = false;
bool g_DumpAsText_Async = false;
int g_referenceTestState = 0;
#endif

static int g_singletonInstanceCnt = 0;
static bool g_starFishGlobalInit = false;
typedef void (*GCCollectionEventListenter)(GC_EventType);
static std::list<GCCollectionEventListenter> g_gcCollectionEventListenterList;
void addGCCollectionListener(void (*fn)(GC_EventType))
{
    g_gcCollectionEventListenterList.push_back(fn);
}

StarFish::StarFish(const char* locale, const char* timezoneID, int w, int h,
                   uint32_t defaultFontSize, String* defaultFontName,
                   const ScreenInfo& info, const char* localStorageFilePath,
                   const char* cookieStoreFilePath,
                   const char* httpCacheDirectorypath,
                   String* customUserAgentString,
                   String* builtinPolyfillPathString)

    : m_locale(icu::Locale::createFromName(locale))
    , m_timezoneID(String::fromUTF8(timezoneID))
    , m_defaultFontSize(defaultFontSize)
    , m_screenScaleRatio(1)
    , m_console(new Console(this))
#if defined(STARFISH_TIZEN_TV) && defined(STARFISH_ENABLE_AVPLAY)
    , m_avplay(new Avplay(this))
#endif
#if defined(STARFISH_ENABLE_INSPECTOR)
    , m_inspector(nullptr)
#endif
    , m_screenInfo(info)
    , m_localStorageFilePath(String::fromUTF8(localStorageFilePath))
    , m_customUserAgentString(customUserAgentString)
    , m_builtinPolyfillPathString(builtinPolyfillPathString)
    , m_activeThreadListMutex(nullptr)
#ifdef STARFISH_ENABLE_HTTPCACHE
    , m_httpCache(nullptr)
#endif
#ifdef STARFISH_ENABLE_TTS
    , m_tts(nullptr)
#endif
    , m_initialFontFamilyDatas(new (GC_MALLOC(sizeof(FontFamilyData) * 2))
                                   FontFamilyData[2]{ 1, defaultFontName })
#ifdef STARFISH_ENABLE_TEST
    , m_testCompatibleMode(StarFishTestCompatibleMode::Normal)
#endif
{
    registerMainThread();
    if (!g_starFishGlobalInit) {
        g_starFishGlobalInit = true;

#if !defined(STARFISH_ANDROID) && !defined(STARFISH_WINDOWS)
        mallopt(M_MMAP_THRESHOLD, 2048);
        mallopt(M_MMAP_MAX, 1024 * 1024);
#endif

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

    m_deviceKind = deviceKindUseTouchScreen;
#ifdef STARFISH_ENABLE_TEST
    m_testCompatibleMode = g_testCompatibleMode;
    m_startUpFlag = g_startUpFlag;
#else
    m_startUpFlag = 0;
#endif

#ifndef STARFISH_LINE_BREAK_ITERATOR_POOL_SIZE
#define STARFISH_LINE_BREAK_ITERATOR_POOL_SIZE 4
#endif
    m_lineBreakIteratorPool =
        new LineBreakIteratorPool(STARFISH_LINE_BREAK_ITERATOR_POOL_SIZE);
    m_atomicStringMap.insert(String::emptyString);
    m_staticStrings = new StaticStrings(this);
    m_messageLoop = new MessageLoop(this);
    m_timer = new Timer(this);

    m_platformFontSelector = PlatformFontSelector::create(this);
    m_platformFontCache = PlatformFontCache::create(this);

#ifndef STARFISH_THREAD_POOL_SIZE
#define STARFISH_THREAD_POOL_SIZE 6
#endif
    m_threadPool = new ThreadPool(STARFISH_THREAD_POOL_SIZE, m_messageLoop);
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
    g_singletonInstanceCnt++;
#ifdef STARFISH_ENABLE_TTS
    m_tts = new TTS(this);
#endif

    m_platformWindow = PlatformWindow::create(this, w, h);

    WebView* webView = WebView::create(this);
    m_platformWindow->setWebView(webView);

    // saidly.. few port layer needs this variable
    m_publicLayerUserDataMap["__internalWebContainerImplementLayerVariable"] =
        this;
}

void StarFish::run()
{
    m_messageLoop->run();
}

static String* resolvePath(String* filePath)
{
    String* resolvedPath = filePath;
    if (!filePath->startsWith("http") && !filePath->startsWith("about") &&
        !filePath->startsWith("data:")) {
#if defined(OS_WINDOWS)
        String* prefix = String::fromUTF8("file:///");
#else
        String* prefix = String::fromUTF8("file://");
#endif
        Nullable<std::string> result =
            FileUtil::absolutePath(filePath->toUTF8NonGCString());
        if (result.hasValue()) {
            resolvedPath = prefix->concat(String::fromUTF8(
                result.getValue().data(), result.getValue().length()));
        } else {
            // Will navigate to about:blank
            resolvedPath = prefix->concat(resolvedPath);
        }
    }

    return resolvedPath;
}

void StarFish::loadHTMLDocument(String* filePath)
{
    String* resolvedPath = resolvePath(filePath);
    ResourceURL* url = new ResourceURL(resolvedPath);
    m_platformWindow->webView()->navigate(url, HistoryManager::Action::Add,
                                          nullptr);
}

void StarFish::resume()
{
    m_platformWindow->resume();
}

void StarFish::pause()
{
    m_platformWindow->pause();
    GC_gcollect_and_unmap();
    GC_gcollect_and_unmap();
    GC_gcollect_and_unmap();
    GC_gcollect_and_unmap();
}

void StarFish::addActiveThread(Thread* thread)
{
    STARFISH_ASSERT(isMainThread());
    m_activeThreadList.push_back(thread);
}

void StarFish::removeActiveThread(Thread* thread)
{
    STARFISH_ASSERT(isMainThread());
    auto it =
        std::find(m_activeThreadList.begin(), m_activeThreadList.end(), thread);
    if (it != m_activeThreadList.end()) {
        m_activeThreadList.erase(it);
    }
}

void StarFish::close()
{
#if defined(STARFISH_ENABLE_INSPECTOR)
    delete m_inspector;
#endif
    platformWindow()->webView()->close();
    m_threadPool->close();
    STARFISH_ASSERT(isMainThread());
    // NOTE: Iterate copied list.
    //       joinIfNeeds() may modify m_activeThreadList.
    GCVector<Thread*> copies = m_activeThreadList;
    for (auto th : copies) {
        th->joinIfNeeds();
    }
    m_messageLoop->close();
    m_timer->close();
    m_platformWindow->close();

    delete m_platformFontCache;
    delete m_platformFontSelector;

    if (g_singletonInstanceCnt <= 1) {
        NetworkSharedResourceManager::close();
#ifdef STARFISH_ENABLE_HTTPCACHE
        HTTPCache::destory();
#endif
        g_singletonInstanceCnt = 0;
    } else {
        g_singletonInstanceCnt--;
    }

    m_publicLayerUserDataMap.clear();
}

String* StarFish::evaluate(String* s)
{
    return toBrowserString(m_platformWindow->webView()
                               ->mainBrowsingContext()
                               ->scriptBindingInstance(),
                           evaluateString(m_platformWindow->webView()
                                              ->mainBrowsingContext()
                                              ->scriptBindingInstance(),
                                          s));
}

#if defined(STARFISH_ENABLE_INSPECTOR)
void StarFish::setupInspector(uint32_t portNumber)
{
    STARFISH_ASSERT(m_inspector == nullptr);
    m_inspector = new Inspector(this);
    m_inspector->run(portNumber);
}
#endif

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

void StarFish::setDefaultFontSize(uint32_t size)
{
    m_defaultFontSize = size;
    if (m_platformWindow->webView()) {
        m_platformWindow->webView()
            ->mainBrowsingContext()
            ->updateDefaultFontSize();
    }
}

String* StarFish::userAgent()
{
    String* custom = customUserAgentString();
    if (custom->length()) {
        return custom;
    }
    return String::createASCIIString(USER_AGENT(STARFISH_NAME, VERSION));
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

void StarFish::registerWebViewHandler(const std::string& handlerName,
                                      std::function<void(String*, int)> handler)
{
    auto it = m_lweWebViewHandlers.find(handlerName);
    if (it == m_lweWebViewHandlers.end()) {
        m_lweWebViewHandlers.insert(std::make_pair(handlerName, handler));
    } else {
        it->second = handler;
    }
}

void StarFish::registerWebViewHandler(const std::string& handlerName,
                                      std::function<void(void*)> handler)
{
    auto it = m_lweWebViewHandlersGeneral.find(handlerName);
    if (it == m_lweWebViewHandlersGeneral.end()) {
        m_lweWebViewHandlersGeneral.insert(
            std::make_pair(handlerName, handler));
    } else {
        it->second = handler;
    }
}

bool StarFish::containsWebViewHandler(const std::string& handlerName)
{
    auto it = m_lweWebViewHandlersGeneral.find(handlerName);
    if (it != m_lweWebViewHandlersGeneral.end()) {
        return true;
    }

    return false;
}

void StarFish::callWebViewHandler(const std::string& handlerName, String* url,
                                  int param)
{
    auto it = m_lweWebViewHandlers.find(handlerName);
    if (it == m_lweWebViewHandlers.end()) {
        return;
    }
    struct dummy : public gc {
        std::string handlerName;
        StarFish* starFish;
        String* url;
        int int_param;
    };
    dummy* d = new dummy;
    d->handlerName = handlerName;
    d->starFish = this;
    d->url = url;
    d->int_param = param;
    messageLoop()->addIdler(
        nullptr,
        [](size_t, void* data) {
            dummy* d = (dummy*)data;
            auto it = d->starFish->m_lweWebViewHandlers.find(d->handlerName);
            if (it != d->starFish->m_lweWebViewHandlers.end()) {
                (it->second)(d->url, d->int_param);
            }
            delete d;
        },
        d);
}

void StarFish::callWebViewHandler(const std::string& handlerName, void* param)
{
    auto it = m_lweWebViewHandlersGeneral.find(handlerName);
    if (it == m_lweWebViewHandlersGeneral.end()) {
        return;
    }

    struct Env {
        StarFish* starFish;
        std::string handlerName;
        void* param;
    };
    Env* env = new Env();
    env->starFish = this;
    env->handlerName = handlerName;
    env->param = param;

    messageLoop()->addIdler(
        nullptr,
        [](size_t, void* env) {
            Env* e = (Env*)env;
            auto it =
                e->starFish->m_lweWebViewHandlersGeneral.find(e->handlerName);
            if (it != e->starFish->m_lweWebViewHandlersGeneral.end()) {
                (it->second)(e->param);
            }
            delete e;
        },
        env);
}
}
