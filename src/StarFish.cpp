/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
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
#include "core/modules/threading/ThreadPool.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/page/Window.h"
#include "core/modules/message_loop/Timer.h"
#include "core/modules/canvas/image/ImageData.h"
#include "core/inspector/Inspector.h"
#include "core/extra/Console.h"
#include "core/util/LineBreakerIteratorPool.h"
#include "platform/network/NetworkSharedResourceManager.h"
#include "platform/window/PlatformWindow.h"

#include <malloc.h>
#ifdef PORT_GRAPHIC_BACKEND_EFL
#include <Elementary.h>

#if defined(STARFISH_TIZEN_3_0) || defined(STARFISH_TIZEN_OBS)
#include <Ecore.h>
#else
#include <Ecore_X.h>
#endif

extern Evas* g_internalCanvas;
#endif

#ifdef PORT_GRAPHIC_BACKEND_DALI
#include <dali-toolkit/dali-toolkit.h>
#endif

#ifdef STARFISH_TIZEN_WEARABLE
#include <tizen.h>
#endif
namespace StarFish {

#ifdef STARFISH_ENABLE_TEST
bool g_enablePixelTest = false;
bool g_memLogDump = false;
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

static bool g_starFishGlobalInit = false;
typedef void (*GCCollectionEventListenter)(GC_EventType);
static std::list<GCCollectionEventListenter> g_gcCollectionEventListenterList;
void addGCCollectionListener(void (*fn)(GC_EventType))
{
    g_gcCollectionEventListenterList.push_back(fn);
}

StarFish::StarFish(StarFishStartUpFlag flag, const char* locale,
                   const char* timezoneID, void* platformHandle, int w, int h,
                   float defaultFontSizeMultiplier, ScreenInfo& info,
                   const char* localStorageFilePath,
                   const char* cookieStoreFilePath)
    : m_locale(icu::Locale::createFromName(locale))
    , m_timezoneID(String::fromUTF8(timezoneID))
    , m_defaultFontSizeMultiplier(defaultFontSizeMultiplier)
    , m_console(new Console(this))
#if defined(STARFISH_ENABLE_INSPECTOR)
    , m_inspector(nullptr)
#endif
    , m_enterCount(0)
    , m_screenInfo(info)
    , m_localStorageFilePath(String::fromUTF8(localStorageFilePath))
    , m_cookieStoreFilePath(String::fromUTF8(cookieStoreFilePath))
{
#ifdef PORT_GRAPHIC_BACKEND_DALI
    m_width = w;
    m_height = h;
#endif
    registerMainThread();
    if (!g_starFishGlobalInit) {
        g_starFishGlobalInit = true;

        mallopt(M_MMAP_THRESHOLD, 2048);
        mallopt(M_MMAP_MAX, 1024 * 1024);

        GC_set_abort_func([](const char* msg) {
            STARFISH_LOG_ERROR("gc abort called\n");
            STARFISH_LOG_ERROR("%s\n", msg);
        });

        GC_set_warn_proc(
            [](char* msg, GC_word arg) { STARFISH_LOG_ERROR(msg, arg); });

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
                    "did GC. GC heapSize[%f MB , %f MB] RSS[%.1f MB] "
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
                STARFISH_LOG_INFO("did GC. GC heapSize[%f MB , %f MB]\n",
                                  GC_get_memory_use() / 1024.f / 1024.f,
                                  GC_get_heap_size() / 1024.f / 1024.f);
            }

            auto iter = g_gcCollectionEventListenterList.begin();
            while (iter != g_gcCollectionEventListenterList.end()) {
                (*iter)(evtType);
                iter++;
            }
        });
        GC_set_free_space_divisor(8);
        GC_set_force_unmap_on_gcollect(1);
    }

#if defined(PORT_GRAPHIC_BACKEND_EFL)
    if (!platformHandle) {
        Evas_Object* wndObj = elm_win_add(NULL, "StarFish", ELM_WIN_BASIC);
        elm_win_title_set(wndObj, "StarFish");
        elm_win_autodel_set(wndObj, EINA_TRUE);
        evas_object_resize(wndObj, w, h);
        platformHandle = wndObj;
    } else {
        evas_object_resize((Evas_Object*)platformHandle, w, h);
    }

#endif
    m_nativeHandle = platformHandle;
    m_deviceKind = deviceKindUseTouchScreen;
    m_startUpFlag = flag;
#ifndef STARFISH_LINE_BREAK_ITERATOR_POOL_SIZE
#define STARFISH_LINE_BREAK_ITERATOR_POOL_SIZE 4
#endif
    m_lineBreakIteratorPool =
        new LineBreakIteratorPool(STARFISH_LINE_BREAK_ITERATOR_POOL_SIZE);
    String* s = String::emptyString;
    AtomicString emptyAtom(s);
    m_atomicStringMap.insert(s);
    m_staticStrings = new StaticStrings(this);
    m_messageLoop = new MessageLoop(this);
    m_timer = new Timer(this);
#ifndef STARFISH_THREAD_POOL_SIZE
#define STARFISH_THREAD_POOL_SIZE 6
#endif
    m_threadPool = new ThreadPool(STARFISH_THREAD_POOL_SIZE, m_messageLoop);

    initCookieSession();
}

StarFish::~StarFish()
{
    STARFISH_LOG_INFO("StarFish::~StarFish\n");
#ifdef STARFISH_ENABLE_TEST
    if (fp_mem)
        fclose(fp_mem);
#endif
    close();
#if defined(STARFISH_ENABLE_INSPECTOR)
    delete m_inspector;
#endif
    delete m_platformWindow;
    NetworkSharedResourceManager::close();
}

void StarFish::run()
{
    m_messageLoop->run();
}

void StarFish::enter()
{
    if (m_enterCount == 0) {
#ifdef PORT_GRAPHIC_BACKEND_EFL
        g_internalCanvas =
            evas_object_evas_get((Evas_Object*)m_platformWindow->unwrap());
#endif
    }
    m_enterCount++;
}

void StarFish::exit()
{
    if (m_enterCount == 1) {
#ifdef PORT_GRAPHIC_BACKEND_EFL
        g_internalCanvas = nullptr;
#endif
    }
    m_enterCount--;
}

void StarFish::loadHTMLDocument(String* filePath)
{
    std::string path;
    if (filePath->startsWith("http")) {
        path = filePath->utf8Data();
    } else if (filePath->startsWith("about")) {
        path = filePath->utf8Data();
    } else {
        std::string d = filePath->utf8Data();
        if (d.length() && d[0] == '/') {
            path = std::string("file://") + d;
        } else {
            std::string fileName;
            if (d.find('/') == std::string::npos) {
                path = "./";
                fileName = d;
            } else {
                path += d.substr(0, d.find_last_of('/'));
                fileName = d.substr(d.find_last_of('/') + 1);
                path += "/";
            }

            char* p = realpath(path.c_str(), NULL);
            if (p) {
                path = p;
                free(p);
            }
            path += "/";

            path = std::string("file://") + path + fileName;
#ifdef STARFISH_ENABLE_TEST
            std::string mem_log =
                fileName.substr(0, fileName.length() - 5) + "_mem.txt";
            if (g_memLogDump)
                fp_mem = fopen(mem_log.c_str(), "w");
#endif
        }
    }

    int width;
    int height;

#if defined(PORT_GRAPHIC_BACKEND_DALI)
    width = m_width;
    height = m_height;
#elif defined(PORT_GRAPHIC_BACKEND_EFL)
    evas_object_geometry_get((Evas_Object*)nativeHandle(), NULL, NULL, &width,
                             &height);
#endif

    m_platformWindow =
        PlatformWindow::create(this, nativeHandle(), width, height);

    WebView* webView = WebView::create(this);
    m_platformWindow->setWebView(webView);

    ResourceURL* url = new ResourceURL(String::fromUTF8(path.c_str()));
    webView->navigate(url, HistoryManager::Action::Add);
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

void StarFish::close()
{
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

#if defined(STARFISH_ENABLE_INSPECTOR)
void StarFish::setupInspector(uint32_t portNumber)
{
    STARFISH_ASSERT(m_inspector == nullptr);
    m_inspector = new Inspector(this, portNumber);
}
#endif

void StarFish::initCookieSession()
{
    // NetworkSharedResourceManager is singleton, So do not hold the instance.
    if (m_cookieStoreFilePath) {
        // Disable to store cookies as a file If m_cookieStoreFilePath is
        // nullptr or empty string
        NetworkSharedResourceManager::getInstance()->setCookieStoreFilePath(
            m_cookieStoreFilePath->toUTF8NonGCString());
    }
    NetworkSharedResourceManager::getInstance()->initCookieSession();
}
}
