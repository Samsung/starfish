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

#include <malloc.h>
#if defined(PORT_WINDOW_BACKEND_EFL)
#include <Elementary.h>
#endif

#if defined(PORT_GRAPHIC_BACKEND_EFL)
extern Evas* g_internalCanvas;
#endif

#if defined(STARFISH_TIZEN_3_0) || defined(STARFISH_TIZEN_OBS)
#include <Ecore.h>
#elif !defined(STARFISH_ANDROID) && !defined(STARFISH_WINDOWS)
#include <Ecore_X.h>
#endif

#ifdef STARFISH_TIZEN_WEARABLE_WIDGET
#include <tizen.h>
#endif
#ifdef STARFISH_ENABLE_TTS
#include "core/modules/tts/TTS.h"
#endif
#if defined(STARFISH_TIZEN_TV)
#include <cursor_module.h> //Include this header to enable and disable cursor
#include <Ecore_Wayland.h>
int vd_util_cursormod;
void Initialize_CursorMod() // Initialize cursor module
{
    Eina_Inlist* globals = ecore_wl_globals_get();
    Eina_Inlist* tmp;
    Ecore_Wl_Global* global;

    struct wl_display* display = ecore_wl_display_get();
    struct wl_registry* registry = ecore_wl_registry_get();
    struct wl_seat* seat = ecore_wl_input_seat_get(ecore_wl_input_get());
    int id = -1;

    EINA_INLIST_FOREACH_SAFE(globals, tmp, global)
    {
        if (!strcmp(global->interface, "tizen_cursor")) {
            id = global->id;
            vd_util_cursormod =
                CursorModule_Initialize(display, registry, seat, id);
        } else {
            vd_util_cursormod = 0;
        }
    }
}

#endif

namespace StarFish {

#ifdef STARFISH_ENABLE_TEST
bool g_enablePixelTest = false;
bool g_memLogDump = false;
bool g_enableDumpAsText = false;
bool g_DumpAsText_Async = false;

FILE* fp_mem = NULL;
static double process_mem_usage()
{
    double vm_usage = 0.0;
    double resident_set = 0.0;

    // 'file' stat seems to give the most reliable results

    std::ifstream stat_stream("/proc/self/stat", std::ios_base::in);

    // dummy vars for leading entries in stat that we don't care about
    std::string pid, comm, state, ppid, pgrp, session, tty_nr;
    std::string tpgid, flags, minflt, cminflt, majflt, cmajflt;
    std::string utime, stime, cutime, cstime, priority, nice;
    std::string O, itrealvalue, starttime;

    // the two fields we want
    unsigned long vsize;
    long rss;

    // don't care about the rest
    stat_stream >> pid >> comm >> state >> ppid >> pgrp >> session >> tty_nr >>
        tpgid >> flags >> minflt >> cminflt >> majflt >> cmajflt >> utime >>
        stime >> cutime >> cstime >> priority >> nice >> O >> itrealvalue >>
        starttime >> vsize >> rss;

    stat_stream.close();

    // in case x86-64 is configured to use 2MB pages
    long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024;
    vm_usage = vsize / 1024.0;
    resident_set = rss * page_size_kb;

    return resident_set;
}

#include <stdio.h>
#include <string.h>

struct smaps_sizes {
    int KernelPageSize;
    int MMUPageSize;
    int Private_Clean;
    int Private_Dirty;
    int Pss;
    int Referenced;
    int Rss;
    int Shared_Clean;
    int Shared_Dirty;
    int Size;
    int Swap;
};

smaps_sizes getSmapsStats()
{
    // Setup our pipe for reading and execute our command.
    char command[512];
    snprintf(command, sizeof(command), "cat /proc/%d/smaps", getpid());
    FILE* file = popen(command, "r");

    struct smaps_sizes sizes;
    memset(&sizes, 0, sizeof sizes);

    char line[BUFSIZ];
    while (fgets(line, sizeof line, file)) {
        // puts(line);
        char substr[32];
        int n;
        if (sscanf(line, "%31[^:]: %d", substr, &n) == 2) {
            if (strcmp(substr, "KernelPageSize") == 0) {
                sizes.KernelPageSize += n;
            } else if (strcmp(substr, "MMUPageSize") == 0) {
                sizes.MMUPageSize += n;
            } else if (strcmp(substr, "Private_Clean") == 0) {
                sizes.Private_Clean += n;
            } else if (strcmp(substr, "Private_Dirty") == 0) {
                sizes.Private_Dirty += n;
            } else if (strcmp(substr, "Pss") == 0) {
                sizes.Pss += n;
            } else if (strcmp(substr, "Referenced") == 0) {
                sizes.Referenced += n;
            } else if (strcmp(substr, "Rss") == 0) {
                sizes.Rss += n;
            } else if (strcmp(substr, "Shared_Clean") == 0) {
                sizes.Shared_Clean += n;
            } else if (strcmp(substr, "Shared_Dirty") == 0) {
                sizes.Shared_Dirty += n;
            } else if (strcmp(substr, "Size") == 0) {
                sizes.Size += n;
            } else if (strcmp(substr, "Swap") == 0) {
                sizes.Swap += n;
            }
        }
    }
    fclose(file);
    return sizes;
}
#endif
static int g_singletonInstanceCnt = 0;
static bool g_starFishGlobalInit = false;
typedef void (*GCCollectionEventListenter)(GC_EventType);
static std::list<GCCollectionEventListenter> g_gcCollectionEventListenterList;
void addGCCollectionListener(void (*fn)(GC_EventType))
{
    g_gcCollectionEventListenterList.push_back(fn);
}

StarFish::StarFish(StarFishStartUpFlag flag, const char* locale,
                   const char* timezoneID, void* platformHandle, int w, int h,
                   int x, int y, float defaultFontSizeMultiplier,
                   String* defaultFontName, const ScreenInfo& info,
                   const char* localStorageFilePath,
                   const char* cookieStoreFilePath,
                   const char* httpCacheDirectorypath,
                   String* customUserAgentString,
                   String* builtinPolyfillPathString)

    : m_locale(icu::Locale::createFromName(locale))
    , m_timezoneID(String::fromUTF8(timezoneID))
    , m_defaultFontSizeMultiplier(defaultFontSizeMultiplier)
    , m_screenScaleRatio(1)
    , m_shouldFitWindow(true)
    , m_console(new Console(this))
#if defined(STARFISH_TIZEN_TV) && defined(STARFISH_ENABLE_AVPLAY)
    , m_avplay(new Avplay(this))
#endif
#if defined(STARFISH_ENABLE_INSPECTOR)
    , m_inspector(nullptr)
#endif
#if defined(TIZEN_DEVICE_API)
    , m_widgetContext(nullptr)
#endif
#if defined(STARFISH_TIZEN_WEARABLE_WIDGET)
    , m_updateFlag(false)
#endif
    , m_enterCount(0)
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
    , m_lweWebView(nullptr)
    , m_posX(0)
    , m_posY(0)
    , m_lweWebViewControlDelegator(nullptr)
{
    m_nativeHandle = platformHandle;
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

#ifdef STARFISH_SHOW_MEMSTATE
        addGCCollectionListener([](GC_EventType evtType) {
            if (GC_EVENT_PRE_START_WORLD == evtType) {
#ifdef STARFISH_ENABLE_TEST
                if (fp_mem && g_memLogDump)
                    fprintf(fp_mem, "%f %f\n",
                            GC_get_memory_use() / 1024.f / 1024.f,
                            process_mem_usage() / 1024.f);

                auto stat = getSmapsStats();

                STARFISH_LOG_INFO(
                    "Done GC: HeapSize: [%f MB , %f MB] RSS[%.1f MB] "
                    "Private_Dirty[%.1fMB]\n",
                    GC_get_memory_use() / 1024.f / 1024.f,
                    GC_get_heap_size() / 1024.f / 1024.f,
                    process_mem_usage() / 1024.f, stat.Private_Dirty / 1024.f);
#else
                STARFISH_LOG_INFO("did GC. GC heapSize[%f MB , %f MB]\n",
                                  GC_get_memory_use() / 1024.f / 1024.f,
                                  GC_get_heap_size() / 1024.f / 1024.f);
#endif
            }
        });
#endif

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

#if defined(PORT_WINDOW_BACKEND_EFL)
    Evas_Object* wndObj = nullptr;
    if (!m_nativeHandle) {
        wndObj = elm_win_add(NULL, STARFISH_NAME, ELM_WIN_BASIC);
        elm_win_title_set(wndObj, STARFISH_NAME);
        elm_win_autodel_set(wndObj, EINA_TRUE);
        evas_object_resize(wndObj, w, h);
        evas_object_move(wndObj, x, y);
        m_nativeHandle = wndObj;
    } else {
        m_shouldFitWindow = false;
        m_posX = x;
        m_posY = y;
    }
#ifdef STARFISH_TIZEN
#ifdef STARFISH_ENABLE_TRANSPARENT_WINDOW
    // Set efl configuration for resizing window (Without this, Window'll be
    // full-screen only )
    elm_win_aux_hint_add(wndObj, "wm.policy.win.user.geometry", "1");

    elm_win_alpha_set(wndObj, EINA_TRUE);
    Evas_Object* bg = elm_bg_add(wndObj);
    evas_object_color_set(bg, 0x00, 0x00, 0x00, 0x00);

    evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add(wndObj, bg);
    evas_object_show(bg);
#else
    Evas_Object* bg = elm_bg_add(wndObj);
    evas_object_color_set(bg, 0xff, 0xff, 0xff, 0xff);

    evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add(wndObj, bg);
    evas_object_show(bg);
#endif
#endif

#endif

#if defined(STARFISH_TIZEN_TV) && !defined(STARFISH_DALI)
    Initialize_CursorMod();
    Ecore_Wl_Window* wl_window =
        elm_win_wl_window_get((Evas_Object*)m_nativeHandle);
    struct wl_surface* surface = ecore_wl_window_surface_get(wl_window);
    Cursor_Set_Config(surface, TIZEN_CURSOR_CONFIG_CURSOR_AVAILABLE,
                      NULL); // Enable cursor in application
    CursorModule_Finalize(); // Finalize cursor module
    vd_util_cursormod = 0;
#endif

    m_deviceKind = deviceKindUseTouchScreen;
    m_startUpFlag = flag;
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

    m_platformWindow = PlatformWindow::create(this, nativeHandle(), w, h);

    WebView* webView = WebView::create(this);
    m_platformWindow->setWebView(webView);
}

StarFish::~StarFish()
{
    STARFISH_LOG_INFO("StarFish::~StarFish\n");
#ifdef STARFISH_ENABLE_TEST
    if (fp_mem) {
        fclose(fp_mem);
    }
#endif
#if defined(STARFISH_ENABLE_INSPECTOR)
    delete m_inspector;
#endif
#if defined(PORT_CANVAS_BACKEND_EFL) || defined(PORT_IMAGEDECODER_BACKEND_EFL)
    g_internalCanvas = nullptr;
#endif
    if (m_platformWindow) {
        close();
        delete m_platformWindow;
    }

    delete m_platformFontCache;
    delete m_platformFontSelector;

    if (g_singletonInstanceCnt <= 1) {
        NetworkSharedResourceManager::close();
#ifdef STARFISH_ENABLE_HTTPCACHE
        HTTPCache::destory();
#endif
    } else {
        g_singletonInstanceCnt--;
    }
}

void StarFish::run()
{
    m_messageLoop->run();
}

void StarFish::enter()
{
    if (m_enterCount == 0) {
#if defined(PORT_CANVAS_BACKEND_EFL) || defined(PORT_IMAGEDECODER_BACKEND_EFL)
        g_internalCanvas =
            evas_object_evas_get((Evas_Object*)m_platformWindow->unwrap());
#endif
    }
    m_enterCount++;
}

void StarFish::exit()
{
    if (m_enterCount == 1) {
#if defined(PORT_CANVAS_BACKEND_EFL) || defined(PORT_IMAGEDECODER_BACKEND_EFL)
        g_internalCanvas = nullptr;
#endif
    }
    m_enterCount--;
}

void StarFish::loadHTMLDocument(String* filePath)
{
    String* resolvedPath = filePath;
    if (!filePath->startsWith("http") && !filePath->startsWith("about") &&
        !filePath->startsWith("data:")) {
#if defined(OS_WINDOWS)
        String* prefix = String::fromUTF8("file:///");
#else
        String* prefix = String::fromUTF8("file://");
#endif
        Nullable<String*> result = File::absolutePath(filePath);
        if (result.hasValue()) {
            resolvedPath = prefix->concat(result.getValue());
        } else {
            // Will navigate to about:blank
            resolvedPath = prefix->concat(resolvedPath);
        }
    }
    ResourceURL* url = new ResourceURL(resolvedPath);
    m_platformWindow->webView()->navigate(url, HistoryManager::Action::Add,
                                          nullptr);
}

void StarFish::resume()
{
    StarFishEnterer enter(this);
    m_platformWindow->resume();
}

void StarFish::pause()
{
    StarFishEnterer enter(this);
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
    m_threadPool->close();
    m_messageLoop->close();
    STARFISH_ASSERT(isMainThread());
    // NOTE: Iterate copied list.
    //       joinIfNeeds() may modify m_activeThreadList.
    GCVector<Thread*> copies = m_activeThreadList;
    for (auto th : copies) {
        th->joinIfNeeds();
    }
    m_timer->close();
    m_platformWindow->close();
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

void StarFish::callWebViewHandler(const std::string& handlerName, String* url,
                                  int error_code)
{
    auto it = m_lweWebViewHandlers.find(handlerName);
    if (it == m_lweWebViewHandlers.end()) {
        return;
    }
    struct dummy : public gc {
        std::string handlerName;
        StarFish* starFish;
        String* url;
        int errorCode;
    };
    dummy* d = new dummy;
    d->handlerName = handlerName;
    d->starFish = this;
    d->url = url;
    d->errorCode = error_code;
    messageLoop()->addIdler(
        nullptr,
        [](size_t, void* data) {
            dummy* d = (dummy*)data;
            auto it = d->starFish->m_lweWebViewHandlers.find(d->handlerName);
            if (it != d->starFish->m_lweWebViewHandlers.end()) {
                (it->second)(d->url, d->errorCode);
            }
            delete d;
        },
        d);
}
}
