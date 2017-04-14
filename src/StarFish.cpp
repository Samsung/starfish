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
#include "dom/Document.h"
#include "platform/threading/ThreadPool.h"
#include "platform/message_loop/MessageLoop.h"
#include "platform/window/Window.h"
#include "platform/canvas/image/ImageData.h"
#include "binding/ScriptBindingInstance.h"
#include "inspector/Inspector.h"
#include "extra/Console.h"

#include <malloc.h>
#include <Elementary.h>
#if defined(STARFISH_TIZEN_3_0) || defined(STARFISH_TIZEN_OBS)
#include <Ecore.h>
#else
#include <Ecore_X.h>
#endif

extern Evas* g_internalCanvas;

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
                   const char* timezoneID, void* win, int w, int h,
                   float defaultFontSizeMultiplier)
    : m_locale(icu::Locale::createFromName(locale))
    , m_lineBreaker(nullptr)
    , m_timezoneID(String::fromUTF8(timezoneID))
    , m_defaultFontSizeMultiplier(defaultFontSizeMultiplier)
    , m_console(new Console(this))
#if defined(STARFISH_ENABLE_INSPECTOR)
    , m_inspector(nullptr)
#endif
    , m_enterCount(0)
    , m_seed((unsigned int)time(NULL))
{
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
// malloc_stats();
#else
                STARFISH_LOG_INFO("did GC. GC heapSize[%f MB , %f MB]\n",
                                  GC_get_memory_use() / 1024.f / 1024.f,
                                  GC_get_heap_size() / 1024.f / 1024.f);
#endif
                // malloc_stats();
            }
        });

        GC_set_on_collection_event([](GC_EventType evtType) {
            auto iter = g_gcCollectionEventListenterList.begin();
            while (iter != g_gcCollectionEventListenterList.end()) {
                (*iter)(evtType);
                iter++;
            }
        });
        GC_set_free_space_divisor(64);
        GC_set_force_unmap_on_gcollect(1);
    }

    if (!win) {
        Evas_Object* wndObj = elm_win_add(NULL, "StarFish", ELM_WIN_BASIC);
        elm_win_title_set(wndObj, "StarFish");
        elm_win_autodel_set(wndObj, EINA_TRUE);
        evas_object_resize(wndObj, w, h);
        win = wndObj;
    } else {
        evas_object_resize((Evas_Object*)win, w, h);
    }

    m_nativeWindow = win;

    m_deviceKind = deviceKindUseTouchScreen;
    m_startUpFlag = flag;

    String* s = String::emptyString;
    AtomicString emptyAtom(s);
    m_atomicStringMap.insert(std::make_pair(std::string(), emptyAtom));
    m_staticStrings = new StaticStrings(this);
    UErrorCode code = U_ZERO_ERROR;
    m_lineBreaker = icu::BreakIterator::createLineInstance(m_locale, code);
    STARFISH_RELEASE_ASSERT(code <= U_ZERO_ERROR);
    m_messageLoop = new MessageLoop(this);
#ifndef STARFISH_THREAD_POOL_SIZE
#define STARFISH_THREAD_POOL_SIZE 6
#endif
    m_threadPool = new ThreadPool(STARFISH_THREAD_POOL_SIZE, m_messageLoop);
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
    delete m_lineBreaker;
    delete m_window;
}

void StarFish::run()
{
    m_messageLoop->run();
}

void StarFish::enter()
{
    if (m_enterCount == 0) {
        g_internalCanvas =
            evas_object_evas_get((Evas_Object*)m_window->unwrap());
        m_window->scriptBindingInstance()->enter();
    }
    m_enterCount++;
}

void StarFish::exit()
{
    if (m_enterCount == 1) {
        g_internalCanvas = nullptr;
        m_window->scriptBindingInstance()->exit();
    }
    m_enterCount--;
}

void StarFish::loadHTMLDocument(String* filePath)
{
    std::string path;
    if (filePath->startsWith("http")) {
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
    evas_object_geometry_get((Evas_Object*)m_nativeWindow, NULL, NULL, &width,
                             &height);
    m_window = Window::create(this, m_nativeWindow, width, height);
    URL* url =
        URL::createURL(String::emptyString, String::fromUTF8(path.c_str()));

    m_window->setHistory(url);
    m_window->navigate(url);
}

void StarFish::resume()
{
    StarFishEnterer enter(this);
    m_window->resume();
}

void StarFish::pause()
{
    StarFishEnterer enter(this);
    m_window->pause();
    GC_gcollect_and_unmap();
    GC_gcollect_and_unmap();
    GC_gcollect_and_unmap();
    GC_gcollect_and_unmap();
}

void StarFish::close()
{
    m_window->close();
}

String* StarFish::evaluate(String* s)
{
    return m_window->scriptBindingInstance()->evaluate(s);
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

bool StarFish::stringToBlobURLString(String* url, BlobURLStore& store)
{
    size_t idx = url->lastIndexOf('/');
    if (idx == SIZE_MAX) {
        return false;
    }

    idx++;
    if (idx >= url->length()) {
        return false;
    }
    String* uuid = url->substring(idx, url->length() - idx);

    const char* str = uuid->utf8Data();
    if (strlen(str) != 36) {
        return false;
    }

    unsigned int a0, a1, a2, a3, a4, a5, a6, a7;
    sscanf(str, "%04X%04X-%04X-%04X-%04X-%04X%04X%04X", &a0, &a1, &a2, &a3, &a4,
           &a5, &a6, &a7);

    union {
        struct {
            uint16_t a;
            uint16_t b;
        } small;
        uint32_t big;
    } spliter;

#ifdef STARFISH_64
    union {
        struct {
            uint16_t a;
            uint16_t b;
            uint16_t c;
            uint16_t d;
        } small;
        uint64_t big;
    } spliter64;
#endif

#ifdef STARFISH_64
    spliter.small.a = a0;
    spliter.small.b = a1;
    store.m_a = spliter.big;

    spliter.small.a = a2;
    spliter.small.b = a3;
    store.m_b = spliter.big;

    spliter64.small.a = a4;
    spliter64.small.b = a5;
    spliter64.small.c = a6;
    spliter64.small.d = a7;
    store.m_blob = (void*)spliter64.big;
#else
    spliter.small.a = a0;
    spliter.small.b = a1;
    store.m_a = spliter.big;

    spliter.small.a = a2;
    spliter.small.b = a3;
    store.m_b = spliter.big;

    spliter.small.a = a4;
    spliter.small.b = a5;
    store.m_c = spliter.big;

    spliter.small.a = a6;
    spliter.small.b = a7;
    store.m_blob = (void*)spliter.big;
#endif

    return true;
}

String* StarFish::blobURLStoreToString(BlobURLStore store, String* origin)
{
    std::string url = "blob:";
    url += origin->utf8Data();
    url += "/";

    union {
        struct {
            uint16_t a;
            uint16_t b;
        } small;
        uint32_t big;
    } spliter;

#ifdef STARFISH_64
    union {
        struct {
            uint16_t a;
            uint16_t b;
            uint16_t c;
            uint16_t d;
        } small;
        uint64_t big;
    } spliter64;
#endif

    char buf[32];
#ifdef STARFISH_64
    spliter.big = store.m_a;
    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter.small.a);
    url += buf;
    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter.small.b);
    url += buf;
    url += "-";

    spliter.big = store.m_b;
    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter.small.a);
    url += buf;
    url += "-";
    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter.small.b);
    url += buf;
    url += "-";

    spliter64.big = (uint64_t)store.m_blob;
    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter64.small.a);
    url += buf;
    url += "-";

    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter64.small.b);
    url += buf;

    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter64.small.c);
    url += buf;

    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter64.small.d);
    url += buf;
#else
    spliter.big = store.m_a;
    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter.small.a);
    url += buf;
    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter.small.b);
    url += buf;
    url += "-";

    spliter.big = store.m_b;
    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter.small.a);
    url += buf;
    url += "-";
    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter.small.b);
    url += buf;
    url += "-";

    spliter.big = store.m_c;
    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter.small.a);
    url += buf;
    url += "-";

    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter.small.b);
    url += buf;

    spliter.big = (uint32_t)store.m_blob;
    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter.small.a);
    url += buf;

    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter.small.b);
    url += buf;
#endif
    return String::createASCIIString(url.data());
}

BlobURLStore StarFish::addBlobInBlobURLStore(Blob* ptr)
{
#ifndef NDEBUG
    {
        BlobURLStore s;
        s.m_blob = ptr;
        STARFISH_ASSERT(m_urlBlobStore.find(s) == m_urlBlobStore.end());
    }
#endif
    BlobURLStore a;
    a.m_blob = ptr;

#ifdef STARFISH_32
    a.m_a = rand_r(&m_seed);
    a.m_b = rand_r(&m_seed);
    a.m_c = rand_r(&m_seed);
#else
    a.m_a = rand_r(&m_seed);
    a.m_b = rand_r(&m_seed);
#endif

    m_urlBlobStore.insert(a);

    return a;
}

void StarFish::removeBlobFromBlobURLStore(Blob* ptr)
{
#ifndef NDEBUG
    {
        BlobURLStore s;
        s.m_blob = ptr;
        STARFISH_ASSERT(m_urlBlobStore.find(s) != m_urlBlobStore.end());
    }
#endif
    BlobURLStore s;
    s.m_blob = ptr;
    m_urlBlobStore.erase(s);
}

bool StarFish::isValidBlobURL(BlobURLStore ptr)
{
    auto iter = m_urlBlobStore.find(ptr);
#ifdef STARFISH_32
    return iter != m_urlBlobStore.end() && ptr.m_a == iter->m_a &&
           ptr.m_b == iter->m_b && ptr.m_c == iter->m_c;
#else
    return iter != m_urlBlobStore.end() && ptr.m_a == iter->m_a &&
           ptr.m_b == iter->m_b;
#endif
}

bool StarFish::isValidBlobURL(Blob* ptr)
{
    BlobURLStore s;
    s.m_blob = ptr;
    auto iter = m_urlBlobStore.find(s);
    return iter != m_urlBlobStore.end();
}

BlobURLStore StarFish::findBlobURL(Blob* ptr)
{
    BlobURLStore s;
    s.m_blob = ptr;
    auto iter = m_urlBlobStore.find(s);
    return *iter;
}

BlobURLStore StarFish::addMediaSourceInBlobURLStore(MediaSource* ptr)
{
#ifndef NDEBUG
    {
        BlobURLStore s;
        s.m_blob = ptr;
        STARFISH_ASSERT(m_urlMediaSourceBlobStore.find(s) ==
                        m_urlMediaSourceBlobStore.end());
    }
#endif
    BlobURLStore a;
    a.m_blob = ptr;

#ifdef STARFISH_32
    a.m_a = rand();
    a.m_b = rand();
    a.m_c = rand();
#else
    a.m_a = rand();
    a.m_b = rand();
#endif

    m_urlMediaSourceBlobStore.insert(a);

    return a;
}

void StarFish::removeMediaSourceFromBlobURLStore(MediaSource* ptr)
{
#ifndef NDEBUG
    STARFISH_LOG_INFO(
        "[TRACE_MSE_GC] StarFish::removeMediaSourceFromBlobURLStore\n");
    {
        BlobURLStore s;
        s.m_blob = ptr;
        STARFISH_ASSERT(m_urlMediaSourceBlobStore.find(s) !=
                        m_urlMediaSourceBlobStore.end());
    }
#endif
    BlobURLStore s;
    s.m_blob = ptr;
    m_urlMediaSourceBlobStore.erase(s);
}

bool StarFish::isValidMediaSourceBlobURL(BlobURLStore ptr)
{
    auto iter = m_urlMediaSourceBlobStore.find(ptr);
#ifdef STARFISH_32
    return iter != m_urlMediaSourceBlobStore.end() && ptr.m_a == iter->m_a &&
           ptr.m_b == iter->m_b && ptr.m_c == iter->m_c;
#else
    return iter != m_urlMediaSourceBlobStore.end() && ptr.m_a == iter->m_a &&
           ptr.m_b == iter->m_b;
#endif
}

bool StarFish::isValidMediaSourceBlobURL(MediaSource* ptr)
{
    BlobURLStore s;
    s.m_blob = ptr;
    auto iter = m_urlMediaSourceBlobStore.find(s);
    return iter != m_urlMediaSourceBlobStore.end();
}

BlobURLStore StarFish::findMediaSourceBlobURL(MediaSource* ptr)
{
    BlobURLStore s;
    s.m_blob = ptr;
    auto iter = m_urlMediaSourceBlobStore.find(s);
    return *iter;
}

void StarFish::clearBlobURLStore()
{
    m_urlMediaSourceBlobStore.clear();
    m_urlBlobStore.clear();
}

#if defined(STARFISH_ENABLE_INSPECTOR)
void StarFish::setupInspector(uint32_t portNumber)
{
    STARFISH_ASSERT(m_inspector == nullptr);
    m_inspector = new Inspector(this, portNumber);
}
#endif
StaticStrings::StaticStrings(StarFish* sf)
    : m_starFish(sf)
    , m_xhtmlNamespaceURI(
          AtomicString::createAtomicString(sf, "http://www.w3.org/1999/xhtml"))
    , m_documentLocalName(AtomicString::createAtomicString(sf, "#document"))
    , m_documentFragmentLocalName(
          AtomicString::createAtomicString(sf, "#document-fragment"))
    , m_textLocalName(AtomicString::createAtomicString(sf, "#text"))
    , m_cdataSectionLocalName(
          AtomicString::createAtomicString(sf, "#cdata-section"))
    , m_commentLocalName(AtomicString::createAtomicString(sf, "#comment"))
{
#define DEFINE_HTML_LOCAL_NAMES(name)  \
    m_##name##TagName = QualifiedName( \
        m_xhtmlNamespaceURI, AtomicString::createAtomicString(sf, #name));
    STARFISH_ENUM_HTML_TAG_NAMES(DEFINE_HTML_LOCAL_NAMES)
#undef DEFINE_HTML_LOCAL_NAMES
    m_id = QualifiedName(AtomicString::emptyAtomicString(),
                         AtomicString::createAtomicString(sf, "id"));
    m_name = QualifiedName(AtomicString::emptyAtomicString(),
                           AtomicString::createAtomicString(sf, "name"));
    m_class = QualifiedName(AtomicString::emptyAtomicString(),
                            AtomicString::createAtomicString(sf, "class"));
    m_localName =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "localName"));
    m_style = QualifiedName(AtomicString::emptyAtomicString(),
                            AtomicString::createAtomicString(sf, "style"));
    m_src = QualifiedName(AtomicString::emptyAtomicString(),
                          AtomicString::createAtomicString(sf, "src"));
    m_width = QualifiedName(AtomicString::emptyAtomicString(),
                            AtomicString::createAtomicString(sf, "width"));
    m_height = QualifiedName(AtomicString::emptyAtomicString(),
                             AtomicString::createAtomicString(sf, "height"));
    m_rel = QualifiedName(AtomicString::emptyAtomicString(),
                          AtomicString::createAtomicString(sf, "rel"));
    m_href = QualifiedName(AtomicString::emptyAtomicString(),
                           AtomicString::createAtomicString(sf, "href"));
    m_type = QualifiedName(AtomicString::emptyAtomicString(),
                           AtomicString::createAtomicString(sf, "type"));
    m_dir = QualifiedName(AtomicString::emptyAtomicString(),
                          AtomicString::createAtomicString(sf, "dir"));
    m_disabled =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "disabled"));
    m_color = QualifiedName(AtomicString::emptyAtomicString(),
                            AtomicString::createAtomicString(sf, "color"));
    m_face = QualifiedName(AtomicString::emptyAtomicString(),
                           AtomicString::createAtomicString(sf, "face"));
    m_size = QualifiedName(AtomicString::emptyAtomicString(),
                           AtomicString::createAtomicString(sf, "size"));
    m_charset = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "charset"));
    m_content = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "content"));
    m_lang = QualifiedName(AtomicString::emptyAtomicString(),
                           AtomicString::createAtomicString(sf, "lang"));
    m_colspan = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "colspan"));
    m_rowspan = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "rowspan"));
    m_bgColor = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "bgcolor"));
    m_span = QualifiedName(AtomicString::emptyAtomicString(),
                           AtomicString::createAtomicString(sf, "span"));
    m_scope = QualifiedName(AtomicString::emptyAtomicString(),
                            AtomicString::createAtomicString(sf, "scope"));
#ifdef STARFISH_ENABLE_MULTIMEDIA
    m_default = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "default"));
    m_loop = QualifiedName(AtomicString::emptyAtomicString(),
                           AtomicString::createAtomicString(sf, "loop"));
    m_autoplay =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "autoplay"));
    m_preload = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "preload"));
    m_controls =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "controls"));
    m_kind = QualifiedName(AtomicString::emptyAtomicString(),
                           AtomicString::createAtomicString(sf, "kind"));
    m_label = QualifiedName(AtomicString::emptyAtomicString(),
                            AtomicString::createAtomicString(sf, "label"));
    m_srclang = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "srclang"));
#endif

    m_click = QualifiedName(AtomicString::emptyAtomicString(),
                            AtomicString::createAtomicString(sf, "click"));
    m_onclick = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "onclick"));
    m_mousedown =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "mousedown"));
    m_onmousedown =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "onmousedown"));
    m_mousemove =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "mousemove"));
    m_onmousemove =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "onmousemove"));
    m_mouseout =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "mouseout"));
    m_onmouseout =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "onmouseout"));
    m_mouseover =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "mouseover"));
    m_onmouseover =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "onmouseover"));
    m_mouseup = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "mouseup"));
    m_onmouseup =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "onmouseup"));
    m_touchstart =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "touchstart"));
    m_ontouchstart =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "ontouchstart"));
    m_touchmove =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "touchmove"));
    m_ontouchmove =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "ontouchmove"));
    m_touchend =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "touchend"));
    m_ontouchend =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "ontouchend"));
    m_load = QualifiedName(AtomicString::emptyAtomicString(),
                           AtomicString::createAtomicString(sf, "load"));
    m_onload = QualifiedName(AtomicString::emptyAtomicString(),
                             AtomicString::createAtomicString(sf, "onload"));
    m_error = QualifiedName(AtomicString::emptyAtomicString(),
                            AtomicString::createAtomicString(sf, "error"));
    m_onerror = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "onerror"));
    m_unload = QualifiedName(AtomicString::emptyAtomicString(),
                             AtomicString::createAtomicString(sf, "unload"));
    m_onunload =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "onunload"));
    m_visibilitychange =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "visibilitychange"));
    m_DOMContentLoaded =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "DOMContentLoaded"));
    m_readystatechange =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "readystatechange"));
    m_progress =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "progress"));
    m_abort = QualifiedName(AtomicString::emptyAtomicString(),
                            AtomicString::createAtomicString(sf, "abort"));
    m_timeout = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "timeout"));
    m_loadend = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "loadend"));
    m_loadstart =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "loadstart"));
    m_enter = QualifiedName(AtomicString::emptyAtomicString(),
                            AtomicString::createAtomicString(sf, "enter"));
    m_exit = QualifiedName(AtomicString::emptyAtomicString(),
                           AtomicString::createAtomicString(sf, "exit"));
    m_blur = QualifiedName(AtomicString::emptyAtomicString(),
                           AtomicString::createAtomicString(sf, "blur"));
    m_onblur = QualifiedName(AtomicString::emptyAtomicString(),
                             AtomicString::createAtomicString(sf, "onblur"));
    m_focus = QualifiedName(AtomicString::emptyAtomicString(),
                            AtomicString::createAtomicString(sf, "focus"));
    m_onfocus = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "onfocus"));
    m_focusin = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "focusin"));
    m_onfocusin =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "onfocusin"));
    m_focusout =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "focusout"));
    m_onfocusout =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "onfocusout"));
#ifdef STARFISH_ENABLE_MULTIMEDIA
    m_cuechange =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "cuechange"));
    m_sourceopen =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "sourceopen"));
    m_sourceended =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "sourceended"));
    m_sourceclose =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "sourceclose"));
    m_addsourcebuffer =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "addsourcebuffer"));
    m_removesourcebuffer = QualifiedName(
        AtomicString::emptyAtomicString(),
        AtomicString::createAtomicString(sf, "removesourcebuffer"));
    m_updatestart =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "updatestart"));
    m_update = QualifiedName(AtomicString::emptyAtomicString(),
                             AtomicString::createAtomicString(sf, "update"));
    m_updateend =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "updateend"));
    m_suspend = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "suspend"));
    m_emptied = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "emptied"));
    m_stalled = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "stalled"));
    m_loadedmetadata =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "loadedmetadata"));
    m_loadeddata =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "loadeddata"));
    m_canplay = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "canplay"));
    m_canplaythrough =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "canplaythrough"));
    m_playing = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "playing"));
    m_waiting = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "waiting"));
    m_seeking = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "seeking"));
    m_seeked = QualifiedName(AtomicString::emptyAtomicString(),
                             AtomicString::createAtomicString(sf, "seeked"));
    m_ended = QualifiedName(AtomicString::emptyAtomicString(),
                            AtomicString::createAtomicString(sf, "ended"));
    m_open = QualifiedName(AtomicString::emptyAtomicString(),
                           AtomicString::createAtomicString(sf, "open"));
    m_closed = QualifiedName(AtomicString::emptyAtomicString(),
                             AtomicString::createAtomicString(sf, "closed"));
    m_durationchange =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "durationchange"));
    m_timeupdate =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "timeupdate"));
    m_play = QualifiedName(AtomicString::emptyAtomicString(),
                           AtomicString::createAtomicString(sf, "play"));
    m_pause = QualifiedName(AtomicString::emptyAtomicString(),
                            AtomicString::createAtomicString(sf, "pause"));
    m_ratechange =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "ratechange"));
    m_volumechange =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "volumechange"));
#endif
    m_keydown = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "keydown"));
    m_keyup = QualifiedName(AtomicString::emptyAtomicString(),
                            AtomicString::createAtomicString(sf, "keyup"));
}
}
