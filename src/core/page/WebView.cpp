/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

// #define STARFISH_ENABLE_PROFILE_TIMER
// #define STARFISH_ENABLE_PROFILE_LOADING

#ifdef STARFISH_ENABLE_PROFILE_LOADING_TIMER
#ifndef STARFISH_ENABLE_TEST
#error \
    "`STARFISH_ENABLE_PROFILE_LOADING_TIMER` flag needs `STARFISH_ENABLE_TEST`"
#endif
#endif

#include "StarFishConfig.h"

#include "WebView.h"

#include "BrowsingContext.h"
#include "StarFish.h"

#include "core/page/Window.h"
#include "core/layout/Frame.h"
#include "core/layout/FrameBlockBox.h"
#include "core/layout/FrameTreeBuilder.h"
#include "core/layout/StackingContext.h"
#include "core/layout/RepaintRegionTracker.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/Compositor.h"
#include "core/modules/message_loop/Timer.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/threading/Thread.h"
#include "core/modules/threading/ThreadPool.h"
#include "core/util/URL.h"

#include "core/dom/HTMLDocument.h"
#include "core/dom/HTMLBodyElement.h"
#include "core/dom/HTMLCollection.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/dom/HTMLIFrameElement.h"

#include "platform/window/PlatformWindow.h"
#include "core/dom/Document.h"
#include "core/storage/Storage.h"
#include "core/storage/StorageNamespace.h"
#include "browser/storage/WebStorageNamespaceProvider.h"
#include "browser/history/HistoryManager.h"
#include "binding/ScriptEngineInstance.h"
#include "core/inspector/Inspector.h"
#include "core/extra/Console.h"
#include "core/style/ComputedStyle.h"
#include "platform/file/File.h"

#if defined(STARFISH_TIZEN_TV) && defined(STARFISH_ENABLE_AVPLAY)
#include "core/extra/Avplay.h"
#endif
#ifdef STARFISH_ENABLE_TTS
#include "core/modules/tts/TTS.h"
#endif

#ifdef STARFISH_ENABLE_TEST
#include "core/extra/Console.h"
#include "core/dom/HTMLLinkElement.h"

extern bool g_fireOnloadEvent;
extern bool g_forceRendering;
extern StarFish::CanvasSurface* g_surfaceForScreehShot;

int g_testCompatibleMode;
int g_startUpFlag;
#endif

namespace StarFish {
#if defined(STARFISH_ENABLE_TEST)
// should be defined in each window port
void screenShotInRendering(WebView* wv, const char* path,
                           std::function<void()> callback);
// WPT Reference Test
static Nullable<String*> rtExtractReference(Document* document)
{
    HTMLCollection* result = document->getElementsByTagName(
        document->starFish()->staticStrings()->m_link);
    for (size_t i = 0; i < result->length(); i++) {
        HTMLLinkElement* current = result->item(i)->asHTMLLinkElement();
        if (current->rel()->equals(String::fromUTF8("match"))) {
            return current->href();
        }
    }
    return Nullable<String*>();
}
// WPT Reference Test
static void rtShouldTrue(bool condition, WebView* wv, const char* msg)
{
    if (!condition) {
        STARFISH_LOG_INFO("STARFISH_RTERROR %s\n", msg);
        exit(0);
    }
}
// WPT Reference Test
static void rtShouldLoaded(Document* document, const char* msg)
{
    HTMLCollection* error =
        document->getElementsByTagName(String::createASCIIString("sfrtfailed"));
    rtShouldTrue((!error->length()), document->webView(), msg);
}
// WPT Reference Test
static std::string rtCreatePngName(int id)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "out/%d_reftest%d.png", (int)getpid(), id);
    return buf;
}
// WPT Reference Test
static void rtScreenShot(WebView* wv)
{
    std::string capturePng = rtCreatePngName(g_referenceTestState);
    screenShotInRendering(wv, capturePng.c_str(), [capturePng]() {
        STARFISH_LOG_INFO("STARFISH_RTCAPTURED %s\n", capturePng.c_str());
    });
}
// WPT Reference Test
static bool rtPixelDiff(WebView* wv)
{
    std::string cmd = "./tool/imgdiff/imgdiff ";
    cmd += rtCreatePngName(1);
    cmd += " ";
    cmd += rtCreatePngName(2);
    FILE* fp = popen(cmd.c_str(), "r");
    rtShouldTrue(fp, wv, "INVALID_IMGDIFF");

    int ch;
    std::string output;
    while ((ch = fgetc(fp)) != EOF) {
        output += ch;
    }
    pclose(fp);
    return output.find("[imgdiff-fail]") == std::string::npos;
}
// WPT Reference Test
static void rtDoTest(Document* document)
{
    WebView* wv = document->webView();
    if (g_referenceTestState == 1) {
        // Case1: Running TC
        rtShouldLoaded(document, "TC_LOAD_FAIL");

        Nullable<String*> url = rtExtractReference(document);
        rtShouldTrue(url.hasValue(), wv, "WRONG_REF_URL");

        rtScreenShot(wv);
        g_referenceTestState = 2;
        ResourceURL* resourceURL =
            new ResourceURL(url.getValue(), document->baseURL()->baseURI());
        wv->messageLoop()->invokeNavigate(wv, resourceURL, nullptr,
                                          HistoryManagerAction::Add);
    } else if (g_referenceTestState == 2) {
        // Case2: Running Reference
        rtShouldLoaded(document, "REF_LOAD_FAIL");
        rtScreenShot(wv);
        if (rtPixelDiff(wv)) {
            STARFISH_LOG_INFO("STARFISH_RTPASS\n");
        } else {
            STARFISH_LOG_INFO("STARFISH_RTFAIL\n");
        }
        exit(0);
    }
}
#endif

WebView* WebView::create(StarFish* starFish, const char* locale,
                         const char* timezoneID, uint32_t w, uint32_t h,
                         uint32_t defaultFontSize, String* defaultFontName,
                         const ScreenInfo& info, String* customUserAgentString,
                         String* builtinPolyfillPathString)
{
    return new WebView(starFish, locale, timezoneID, w, h, defaultFontSize,
                       defaultFontName, info, customUserAgentString,
                       builtinPolyfillPathString);
}

WebView::WebView(StarFish* starFish, const char* locale, const char* timezoneID,
                 uint32_t w, uint32_t h, uint32_t defaultFontSize,
                 String* defaultFontName, const ScreenInfo& info,
                 String* customUserAgentString,
                 String* builtinPolyfillPathString)
    : StarFishHoldable(starFish)
    , m_platformWindow(PlatformWindow::create(starFish, w, h))
    , m_topLevelBrowsingContext(nullptr)
    , m_scriptEngineInstance(nullptr)
    , m_storageNamespaceProvider(nullptr)
    , m_localStorageNamespace(nullptr)
    , m_sessionStorageNamespace(nullptr)
    , m_historyManager(nullptr)
    , m_seed((unsigned int)time(NULL))
    , m_navigateStartingTime(0)
    , m_currentActiveAnimatorCount(0)
    , m_inRendering(false)
    , m_needsRendering(false)
    , m_needsEstablishesStackingContext(false)
    , m_needsComputeStackingContextProperties(false)
    , m_needsPainting(false)
    , m_needsComposite(false)
    , m_needsContinuousRendering(false)
    , m_didCompositeBefore(false)
    , m_isActive(false)
    , m_rootStackingContext(nullptr)
    , m_activeAnimatorForAnimationExecutor(SIZE_MAX)
    , m_messageLoop(new MessageLoop(this))
    , m_timer(new Timer(this))
    , m_console(new Console(this))
#ifdef STARFISH_ENABLE_TTS
    , m_tts(new TTS(this))
#endif
#if defined(STARFISH_TIZEN_TV) && defined(STARFISH_ENABLE_AVPLAY)
    , m_avplay(new Avplay(this))
#endif
#if defined(STARFISH_ENABLE_INSPECTOR)
    , m_inspector(nullptr)
#endif
    , m_locale(icu::Locale::createFromName(locale))
    , m_timezoneID(String::fromUTF8(timezoneID))
    , m_defaultFontSize(defaultFontSize)
    , m_screenInfo(info)
    , m_customUserAgentString(customUserAgentString)
    , m_builtinPolyfillPathString(builtinPolyfillPathString)
#ifdef STARFISH_ENABLE_TEST
    , m_testCompatibleMode(StarFishTestCompatibleMode::Normal)
#endif
{
    m_platformWindow->setWebView(this);
    m_deviceKind = deviceKindUseTouchScreen;
#ifdef STARFISH_ENABLE_TEST
    m_testCompatibleMode = g_testCompatibleMode;
    m_startUpFlag = g_startUpFlag;
#else
    m_startUpFlag = 0;
#endif

    m_messageLoop = new MessageLoop(this);
    m_timer = new Timer(this);
#ifndef STARFISH_THREAD_POOL_SIZE
#define STARFISH_THREAD_POOL_SIZE 6
#endif
    m_threadPool = new ThreadPool(STARFISH_THREAD_POOL_SIZE, m_messageLoop);
    m_historyManager = HistoryManager::create(this);
    initRenderingFlags();
    initStorage();

    m_initialFontFamilyDatas = (new (GC_MALLOC(sizeof(FontFamilyData) * 2))
                                    FontFamilyData[2]{ 1, defaultFontName });

    m_platformFontSelector = PlatformFontSelector::create(this);
    m_platformFontCache = PlatformFontCache::create(this);

    // saidly.. few port layer needs this variable
    m_publicLayerUserDataMap["__internalWebContainerImplementLayerVariable"] =
        this;
    m_starFish->m_webViewInstanceCount++;
}

void WebView::destroy()
{
    STARFISH_LOG_INFO("WebView::destroy\n");
#if defined(STARFISH_ENABLE_INSPECTOR)
    delete m_inspector;
    m_inspector = nullptr;
#endif

    pause();

    if (mainBrowsingContext()) {
        mainBrowsingContext()->dispose();
    }

    if (m_rootStackingContext) {
        StackingContext* ctx = m_rootStackingContext;
        std::function<void(StackingContext*)> clearSC =
            [&](StackingContext* ctx) {
                ctx->clearGraphicsBuffer();
                auto iter = ctx->childContexts().begin();
                while (iter != ctx->childContexts().end()) {
                    StackingContextChild* child = *iter;
                    auto iter2 = child->begin();
                    while (iter2 != child->end()) {
                        clearSC(*iter2);
                        iter2++;
                    }
                    iter++;
                }
            };
        clearSC(ctx);
        m_rootStackingContext = nullptr;
    }

    m_threadPool->destroy();
    STARFISH_ASSERT(isMainThread());
    // NOTE: Iterate copied list.
    //       joinIfNeeds() may modify m_activeThreadList.
    GCVector<Thread*> copies = m_activeThreadList;
    for (auto th : copies) {
        th->joinIfNeeds();
    }
    m_messageLoop->destroy();

    m_timer->clear(nullptr);
    m_timer->destroy();

    m_publicLayerUserDataMap.clear();

    delete m_platformFontCache;
    delete m_platformFontSelector;

    m_platformWindow->destroy();
    removeScriptEngineInstance();

    m_starFish->m_webViewInstanceCount--;

    this->WebView::~WebView();
}

void WebView::initStorage()
{
    // TODO: The name of disk storage file name should be auto-generated
    m_storageNamespaceProvider =
        WebStorageNamespaceProvider::create(m_starFish->localStorageFilePath());
    m_localStorageNamespace =
        m_storageNamespaceProvider->createLocalStorageNamespace();
    m_sessionStorageNamespace =
        m_storageNamespaceProvider->createSessionStorageNamespace();
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

void WebView::loadHTMLDocument(String* filePath) // navigate function helper
{
    String* resolvedPath = resolvePath(filePath);
    ResourceURL* url = new ResourceURL(resolvedPath);
    navigate(url, HistoryManagerAction::Add, nullptr);
}

void WebView::navigate(ResourceURL* url, HistoryManagerAction type,
                       ResourceURL* referrerURL)
{
    clearBlobURLStore();
    initRenderingFlags();
    clearStack<102400>();
    m_navigateStartingTime = timestamp();
    if (m_topLevelBrowsingContext) {
        m_topLevelBrowsingContext->dispose();
    }
    platformWindow()->hideSoftwareKeyboardIfPossible();
    m_topLevelBrowsingContext = BrowsingContext::create(this);

    removeScriptEngineInstance();
    createScriptEngineInstance();

    m_topLevelBrowsingContext->open(url, type, referrerURL);
    callPublicWebViewHandler(std::string("OnPageStarted"), url->urlString());
}

String* WebView::userAgent()
{
    String* custom = customUserAgentString();
    if (custom->length()) {
        return custom;
    }
    return String::createASCIIString(USER_AGENT(STARFISH_NAME, VERSION));
}

String* WebView::evaluateJavaScript(String* s)
{
    if (mainBrowsingContext()) {
        return toBrowserString(
            mainBrowsingContext()->scriptBindingInstance(),
            evaluateString(mainBrowsingContext()->scriptBindingInstance(), s));
    } else {
        return String::emptyString;
    }
}

bool WebView::stringToBlobURLString(String* url, BlobURLStore& store)
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

    auto utf8Data = uuid->toUTF8NonGCString();
    const char* str = utf8Data.data();
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
        } tiny;
        uint32_t big;
    } spliter;

#ifdef STARFISH_64
    union {
        struct {
            uint16_t a;
            uint16_t b;
            uint16_t c;
            uint16_t d;
        } tiny;
        uint64_t big;
    } spliter64;
#endif

#ifdef STARFISH_64
    spliter.tiny.a = a0;
    spliter.tiny.b = a1;
    store.m_a = spliter.big;

    spliter.tiny.a = a2;
    spliter.tiny.b = a3;
    store.m_b = spliter.big;

    spliter64.tiny.a = a4;
    spliter64.tiny.b = a5;
    spliter64.tiny.c = a6;
    spliter64.tiny.d = a7;
    store.m_blob = (void*)spliter64.big;
#else
    spliter.tiny.a = a0;
    spliter.tiny.b = a1;
    store.m_a = spliter.big;

    spliter.tiny.a = a2;
    spliter.tiny.b = a3;
    store.m_b = spliter.big;

    spliter.tiny.a = a4;
    spliter.tiny.b = a5;
    store.m_c = spliter.big;

    spliter.tiny.a = a6;
    spliter.tiny.b = a7;
    store.m_blob = (void*)spliter.big;
#endif

    return true;
}

String* WebView::blobURLStoreToString(BlobURLStore store, String* origin)
{
    UTF8StringDataNonGCStd url = "blob:";
    url += origin->toUTF8NonGCString();
    url += "/";

    union {
        struct {
            uint16_t a;
            uint16_t b;
        } tiny;
        uint32_t big;
    } spliter;

#ifdef STARFISH_64
    union {
        struct {
            uint16_t a;
            uint16_t b;
            uint16_t c;
            uint16_t d;
        } tiny;
        uint64_t big;
    } spliter64;
#endif

    char buf[32];
#ifdef STARFISH_64
    spliter.big = store.m_a;
    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter.tiny.a);
    url += buf;
    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter.tiny.b);
    url += buf;
    url += "-";

    spliter.big = store.m_b;
    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter.tiny.a);
    url += buf;
    url += "-";
    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter.tiny.b);
    url += buf;
    url += "-";

    spliter64.big = (uint64_t)store.m_blob;
    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter64.tiny.a);
    url += buf;
    url += "-";

    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter64.tiny.b);
    url += buf;

    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter64.tiny.c);
    url += buf;

    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter64.tiny.d);
    url += buf;
#else
    spliter.big = store.m_a;
    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter.tiny.a);
    url += buf;
    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter.tiny.b);
    url += buf;
    url += "-";

    spliter.big = store.m_b;
    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter.tiny.a);
    url += buf;
    url += "-";
    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter.tiny.b);
    url += buf;
    url += "-";

    spliter.big = store.m_c;
    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter.tiny.a);
    url += buf;
    url += "-";

    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter.tiny.b);
    url += buf;

    spliter.big = (uint32_t)store.m_blob;
    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter.tiny.a);
    url += buf;

    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter.tiny.b);
    url += buf;
#endif
    return String::createASCIIString(url.data());
}

BlobURLStore WebView::addBlobInBlobURLStore(Blob* ptr)
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

void WebView::removeBlobFromBlobURLStore(Blob* ptr)
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

bool WebView::isValidBlobURL(BlobURLStore ptr)
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

bool WebView::isValidBlobURL(Blob* ptr)
{
    BlobURLStore s;
    s.m_blob = ptr;
    auto iter = m_urlBlobStore.find(s);
    return iter != m_urlBlobStore.end();
}

BlobURLStore WebView::findBlobURL(Blob* ptr)
{
    BlobURLStore s;
    s.m_blob = ptr;
    auto iter = m_urlBlobStore.find(s);
    return *iter;
}

BlobURLStore WebView::addMediaSourceInBlobURLStore(MediaSource* ptr)
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

void WebView::removeMediaSourceFromBlobURLStore(MediaSource* ptr)
{
#ifndef NDEBUG
    STARFISH_LOG_INFO(
        "[TRACE_MSE_GC] WebView::removeMediaSourceFromBlobURLStore\n");
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

bool WebView::isValidMediaSourceBlobURL(BlobURLStore ptr)
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

bool WebView::isValidMediaSourceBlobURL(MediaSource* ptr)
{
    BlobURLStore s;
    s.m_blob = ptr;
    auto iter = m_urlMediaSourceBlobStore.find(s);
    return iter != m_urlMediaSourceBlobStore.end();
}

BlobURLStore WebView::findMediaSourceBlobURL(MediaSource* ptr)
{
    BlobURLStore s;
    s.m_blob = ptr;
    auto iter = m_urlMediaSourceBlobStore.find(s);
    return *iter;
}

void WebView::clearBlobURLStore()
{
    m_urlMediaSourceBlobStore.clear();
    m_urlBlobStore.clear();
    GCUnorderedSet<BlobURLStore>().swap(m_urlMediaSourceBlobStore);
    GCUnorderedSet<BlobURLStore>().swap(m_urlBlobStore);
}

void WebView::layoutIfNeeds(bool shouldCareStackingContextNow)
{
    INSTALL_PROFILE_TIMER(starFish(), "WebView::rendering::layoutIfNeeds");
    bool didLayout = false;

    {
        didLayout = didLayout | m_topLevelBrowsingContext->layoutIfNeeds();
        for (size_t i = 0; i < m_browsingContextsNeedsLayout.size(); i++) {
            didLayout =
                didLayout | m_browsingContextsNeedsLayout[i]->layoutIfNeeds();
        }
        m_browsingContextsNeedsLayout.clear();

        if (didLayout) {
            m_needsEstablishesStackingContext = true;
        }
    }

    if (shouldCareStackingContextNow) {
        if (!m_rootStackingContext || m_needsEstablishesStackingContext) {
            INSTALL_PROFILE_TIMER(starFish(), "establishesStackingContext");
            clearStackingContext();
#ifdef STARFISH_ENABLE_TEST
            if (startUpFlag() & StarFishStartUpFlag::enableComputedStyleDump) {
                // dump style
                m_topLevelBrowsingContext->document()
                    ->styleResolver()
                    .dumpDOMStyle(m_topLevelBrowsingContext->document());
            }
            if (startUpFlag() & StarFishStartUpFlag::enableFrameTreeDump) {
                FrameTreeBuilder::dumpFrameTree(
                    m_topLevelBrowsingContext->document(), 0);
            }
#endif

            m_topLevelBrowsingContext->document()
                ->frame()
                ->establishesStackingContextIfNeedsAndComputingPaintingFlags();
            if (m_topLevelBrowsingContext->document()->frame()->firstChild()) {
                m_rootStackingContext = m_topLevelBrowsingContext->document()
                                            ->frame()
                                            ->firstChild()
                                            ->asFrameBox()
                                            ->stackingContext();
            } else {
                m_rootStackingContext = nullptr;
            }
            m_needsEstablishesStackingContext = false;
            setNeedsComputeStackingContextProperties();
        }

        if (m_needsComputeStackingContextProperties) {
            {
                INSTALL_PROFILE_TIMER(starFish(),
                                      "computeStackingContextProperties");
                if (m_topLevelBrowsingContext->document()
                        ->frame()
                        ->firstChild() &&
                    m_rootStackingContext) {
                    m_rootStackingContext->computeStackingContextProperties();
                }
                m_needsComputeStackingContextProperties = false;
            }

#ifdef STARFISH_ENABLE_TEST
            if (startUpFlag() &
                StarFishStartUpFlag::enableStackingContextDump) {
                size_t totalSurfaceBufferSize = 0;
                if (m_rootStackingContext) {
                    STARFISH_ASSERT(mainBrowsingContext()
                                        ->document()
                                        ->frame()
                                        ->firstChild()
                                        ->asFrameBox()
                                        ->isRootElement());
                    StackingContext* ctx = mainBrowsingContext()
                                               ->document()
                                               ->frame()
                                               ->firstChild()
                                               ->asFrameBox()
                                               ->stackingContext();

                    std::function<void(StackingContext*, int)> dumpSC =
                        [&dumpSC, &totalSurfaceBufferSize](StackingContext* ctx,
                                                           int depth) {
                            for (int i = 0; i < depth; i++) {
                                printf("  ");
                            }

                            auto fr = ctx->visibleRect();
                            auto se = ctx->screenExtent();

                            if (ctx->needsGraphicsBuffer()) {
                                LayoutUnit minX = ctx->visibleRect().x();
                                LayoutUnit maxX = ctx->visibleRect().maxX();
                                LayoutUnit minY = ctx->visibleRect().y();
                                LayoutUnit maxY = ctx->visibleRect().maxY();

                                size_t bufferWidth = (int)(maxX - minX);
                                size_t bufferHeight = (int)(maxY - minY);

                                totalSurfaceBufferSize +=
                                    (int)(bufferWidth * bufferHeight * 4);
                            }

                            if (ctx->owner()->node() &&
                                ctx->owner()->node()->isHTMLElement()) {
                                std::string className;
                                HTMLElement* element =
                                    ctx->owner()->node()->asHTMLElement();
                                for (unsigned i = 0;
                                     i < element->classNames().size(); i++) {
                                    auto s = element->classNames()[i]
                                                 .string()
                                                 ->toUTF8NonGCString();
                                    className += s;
                                    className += " ";
                                }

                                auto utf8DataLog1 =
                                    element->localName()->toUTF8NonGCString();
                                auto utf8DataLog2 =
                                    element->id()->toUTF8NonGCString();
                                printf(
                                    "StackingContext[%d][%p, node %p %s id:%s "
                                    "className:%s"
                                    ", frame %p, buf? %d opacity %f "
                                    "screenExtent %f %f %f %f visibleRect "
                                    "%d "
                                    "%d %d %d]",
                                    depth / 2, ctx, element,
                                    utf8DataLog1.data(), utf8DataLog2.data(),
                                    className.data(), ctx->owner(),
                                    (int)ctx->needsGraphicsBuffer(),
                                    ctx->owner()->style()->opacity(),
                                    (float)se.x(), (float)se.y(),
                                    (float)se.width(), (float)se.height(),
                                    (int)fr.x(), (int)fr.y(), (int)fr.width(),
                                    (int)fr.height());
                            } else {
                                printf(
                                    "StackingContext[%d][%p, anonymous node"
                                    ", frame %p, buf %d opacity %f "
                                    "screenExtent %f %f %f %f visibleRect %d "
                                    "%d %d %d]",
                                    depth / 2, ctx, ctx->owner(),
                                    (int)ctx->needsGraphicsBuffer(),
                                    ctx->owner()->style()->opacity(),
                                    (float)se.x(), (float)se.y(),
                                    (float)se.width(), (float)se.height(),
                                    (int)fr.x(), (int)fr.y(), (int)fr.width(),
                                    (int)fr.height());
                            }

                            auto reason = ctx->needsGraphicsBufferReason();
                            if (reason) {
                                printf(" buf reason? %d", (int)reason);
                            }

                            SkMatrix m = ctx->transformMatrix();
                            if (!m.isIdentity()) {
                                printf(" matrix [%f %f %f][%f %f %f][%f %f %f]",
                                       m.getScaleX(), m.getSkewX(),
                                       m.getTranslateX(), m.getSkewY(),
                                       m.getScaleY(), m.getTranslateY(),
                                       m.getPerspX(), m.getPerspY(), m.get(8));
                            }
                            printf("\n");

                            auto iter = ctx->childContexts().begin();
                            while (iter != ctx->childContexts().end()) {
                                StackingContextChild* child = *iter;
                                int32_t num = child->at(0)->zIndex();

                                for (int i = 0; i < depth + 1; i++) {
                                    printf("  ");
                                }

                                printf("z-index: %d\n", (int)num);

                                auto iter2 = child->begin();
                                while (iter2 != child->end()) {
                                    dumpSC(*iter2, depth + 2);
                                    iter2++;
                                }

                                iter++;
                            }
                        };

                    dumpSC(ctx, 0);

                    printf("total buffer Size -> %f\n",
                           totalSurfaceBufferSize / 1024.f / 1024.f);
                }
            }
#endif
        }
    }

    clearStack<102400>();
}

void WebView::setNeedsRendering()
{
    if (m_inRendering) {
        return;
    }
    auto wnd = platformWindow();
    m_needsRendering = true;
    wnd->setNeedsRendering();
}

RenderResult WebView::rendering(bool force)
{
    RenderResult renderResult;
    renderResult.didPaintingOrCompositing = false;

    if (!m_needsRendering || !m_isActive) {
        return renderResult;
    }

    if (!force && mainBrowsingContext()->hasPendingStyleSheet() &&
        mainBrowsingContext()->document() &&
        mainBrowsingContext()
            ->document()
            ->resourceLoader()
            .isDocumentInOpenState() &&
        ((timestamp() -
          mainBrowsingContext()
              ->document()
              ->resourceLoader()
              .documentOpenTime()) < 1000)) {
        STARFISH_LOG_INFO("delay rendering due to pending stylesheet\n");
        m_needsRendering = false;
        Canvas* canvas = platformWindow()->preparePainting();
        mainBrowsingContext()->clearingBeforePaint(canvas);
        renderResult.didPaintingOrCompositing = true;
        renderResult.updateRect = LayoutRect(0, 0, platformWindow()->width(),
                                             platformWindow()->height());
        delete canvas;

        return renderResult;
    }

    uint64_t currentTick = longTickCount();
    m_lastRenderingTick = currentTick;
    m_inRendering = true;
    INSTALL_PROFILE_TIMER(starFish(), "WebView::rendering");

    {
        INSTALL_PROFILE_TIMER(
            starFish(),
            "WebView::rendering::call request animation frame handlers");
        auto rafHandlers = std::move(timer()->m_requestAnimationFrameHandler);
        auto iter = rafHandlers.begin();

        while (iter != rafHandlers.end()) {
            Timer::RequestAnimationFrameData* data = iter->second;
            data->m_handler(data->m_window, data->m_data);
            iter++;
        }
    }

    layoutIfNeeds();

    if (m_needsPainting) {
        INSTALL_PROFILE_TIMER(starFish(), "painting");

        renderResult.didPaintingOrCompositing = true;
        renderResult.updateRect = LayoutRect(0, 0, platformWindow()->width(),
                                             platformWindow()->height());

        // painting
        Canvas* canvas = nullptr;

        if (m_rootStackingContext &&
            mainBrowsingContext()->document()->frame()->firstChild()) {
            m_needsComposite = m_rootStackingContext->needsGraphicsBuffer();
        } else {
            m_needsComposite = false;
        }

        bool needsFullPainting = m_didCompositeBefore && !m_needsComposite;

        if (mainBrowsingContext()->document()->frame()->firstChild() &&
            mainBrowsingContext()
                ->document()
                ->frame()
                ->firstChild()
                ->isAbsolutePositioned()) {
            needsFullPainting = true;
        }

        if (needsFullPainting) {
            m_paintingDirtyRect =
                LayoutRect(0, 0, mainBrowsingContext()->window()->innerWidth(),
                           mainBrowsingContext()->window()->innerHeight());
        }

        {
            FrameBlockBox* mainFrame =
                mainBrowsingContext()->document()->frame()->asFrameBlockBox();

            LayoutUnit scrollX = mainFrame->scrollLeft();
            LayoutUnit scrollY = mainFrame->scrollTop();
            LayoutUnit additionalX, additionalY;
            if (mainFrame->firstChild() && m_rootStackingContext) {
                additionalX = mainFrame->firstChild()->asFrameBox()->x();
                additionalY = mainFrame->firstChild()->asFrameBox()->y();
                scrollX -= mainFrame->firstChild()->asFrameBox()->x();
                scrollY -= mainFrame->firstChild()->asFrameBox()->y();
            }

            if (!m_rootStackingContext) {
                m_paintingDirtyRect = LayoutRect(
                    0, 0, mainBrowsingContext()->window()->innerWidth(),
                    mainBrowsingContext()->window()->innerHeight());
            }

            auto prevDrawnStackingContextInfo =
                std::move(m_prevDrawnStackingContextInfo);
            RepaintRegionTracker tracker(
                mainBrowsingContext()->document()->frame()->asFrameBlockBox(),
                m_paintingDirtyRect, prevDrawnStackingContextInfo, scrollX,
                scrollY);
            m_paintingDirtyRect = LayoutRect(0, 0, 0, 0);
            LayoutRect repaintRect = tracker.repaintRegion();

            repaintRect.setX(repaintRect.x() - 1);
            repaintRect.setY(repaintRect.y() - 1);
            repaintRect.setWidth(repaintRect.width() + 2);
            repaintRect.setHeight(repaintRect.height() + 2);
            renderResult.computedRepaintRect = repaintRect;

            {
                // remove definitely useless graphics buffer first.
                auto iter = prevDrawnStackingContextInfo.begin();
                while (iter != prevDrawnStackingContextInfo.end()) {
                    if (iter->second.graphicsBuffer) {
                        if (!iter->first->frame() ||
                            !iter->first->frame()->isFrameBox() ||
                            !iter->first->frame()
                                 ->asFrameBox()
                                 ->stackingContext() ||
                            !iter->first->frame()
                                 ->asFrameBox()
                                 ->stackingContext()
                                 ->needsGraphicsBuffer()) {
                            iter->second.graphicsBuffer->detachNativeBuffer();
                            iter->second.graphicsBuffer = nullptr;
                        }
                    }
                    iter++;
                }
            }

            StackingContext::PaintingStackingContextContext ctx(
                m_needsComposite, prevDrawnStackingContextInfo, repaintRect,
                scrollX, scrollY);

            if (!m_needsComposite) {
                canvas = platformWindow()->preparePainting();
                canvas->save();
                canvas->pixelSnappedClip(repaintRect);
                canvas->translate(-scrollX, -scrollY);
                canvas->translate(-additionalX, -additionalY);
                mainBrowsingContext()->paintWindowBackground(canvas);
                canvas->translate(additionalX, additionalY);

                if (mainFrame->firstChild() && m_rootStackingContext) {
                    m_rootStackingContext->paintStackingContext(canvas, ctx);
                }

                canvas->translate(scrollX, scrollY);
                mainBrowsingContext()->window()->scrolling()->paintScrollbars(
                    canvas, mainFrame, mainFrame->appliedOverflowX(),
                    mainFrame->appliedOverflowY());

                canvas->restore();
                m_didCompositeBefore = false;
                repaintRect.setX(repaintRect.x() - scrollX);
                repaintRect.setY(repaintRect.y() - scrollY);

                float d = screenInfo().devicePixelRatio;
                renderResult.updateRect = LayoutRect(
                    repaintRect.x() * d, repaintRect.y() * d,
                    repaintRect.width() * d, repaintRect.height() * d);
            } else {
                platformWindow()->willCompositing();
                STARFISH_ASSERT(
                    m_rootStackingContext ==
                    mainFrame->firstChild()->asFrameBox()->stackingContext());
                m_rootStackingContext->paintStackingContext(nullptr, ctx);
            }

            LayoutRect screen(0, 0, platformWindow()->width(),
                              platformWindow()->height());
            LayoutRect rt = renderResult.updateRect;
            if (rt.x() < 0) {
                if (rt.width() + rt.x() > 0) {
                    rt.setWidth(rt.width() + rt.x());
                } else {
                    rt.setWidth(0);
                }
                rt.setX(0);
            }
            if (rt.x() >= screen.width()) {
                rt.setX(0);
                rt.setWidth(0);
            }
            if (rt.y() < 0) {
                if (rt.height() + rt.y() > 0) {
                    rt.setHeight(rt.height() + rt.y());
                } else {
                    rt.setHeight(0);
                }
                rt.setY(0);
            }
            if (rt.y() >= screen.height()) {
                rt.setY(0);
                rt.setHeight(0);
            }
            STARFISH_RELEASE_ASSERT(rt.width() >= 0);
            STARFISH_RELEASE_ASSERT(rt.height() >= 0);
            if (rt.maxX() > screen.maxX()) {
                LayoutUnit widthWillBe = screen.maxX() - rt.x();
                rt.setWidth(widthWillBe);
            }
            if (rt.maxY() > screen.maxY()) {
                LayoutUnit heightWillBe = screen.maxY() - rt.y();
                rt.setHeight(heightWillBe);
            }
            renderResult.updateRect = rt;

            /*
            STARFISH_LOG_INFO("repaint region(device) %f %f %f %f\n",
                              (float)renderResult.updateRect.x(),
                              (float)renderResult.updateRect.y(),
                              (float)renderResult.updateRect.width(),
                              (float)renderResult.updateRect.height());
             */
            auto iter = prevDrawnStackingContextInfo.begin();
            while (iter != prevDrawnStackingContextInfo.end()) {
                if (iter->second.graphicsBuffer) {
                    iter->second.graphicsBuffer->detachNativeBuffer();
                }
                iter++;
            }
        }

        m_needsPainting = false;
#ifdef STARFISH_ENABLE_VIRTUAL_CURSOR
        if (!m_needsComposite) {
            platformWindow()->paintVirtualCursor(canvas);
        }
#endif

        delete canvas;
        clearStack<102400>();
    }

    if (m_needsComposite) {
        INSTALL_PROFILE_TIMER(starFish(), "composite");
        renderResult.didPaintingOrCompositing = true;
        renderResult.updateRect = LayoutRect(0, 0, platformWindow()->width(),
                                             platformWindow()->height());

        if (mainBrowsingContext()->document()->frame()->firstChild() &&
            m_rootStackingContext->needsGraphicsBuffer()) {
            Compositor* compositor = platformWindow()->prepareCompositor();
            FrameBlockBox* mainFrame =
                mainBrowsingContext()->document()->frame()->asFrameBlockBox();

            compositor->save();
            compositor->translate(-mainFrame->scrollLeft(),
                                  -mainFrame->scrollTop());
            bool colorFill = false;
            if (mainBrowsingContext()->hasRootElementBackground() ||
                mainBrowsingContext()->hasBodyElementBackground()) {
                Unit::Color clr;
                if (mainBrowsingContext()->hasRootElementBackground()) {
                    HTMLHtmlElement* root =
                        mainBrowsingContext()->document()->rootElement();
                    colorFill = true;
                    clr = root->style()->color();
                } else {
                    HTMLBodyElement* body = mainBrowsingContext()
                                                ->document()
                                                ->rootElement()
                                                ->body();
                    if (!body) {
                        colorFill = true;
                        clr = body->style()->color();
                    }
                }
                if (colorFill) {
                    compositor->clearColor(clr);
                }
            }

            if (!colorFill) {
                mainBrowsingContext()->clearingBeforePaint(compositor);
            }

            m_rootStackingContext->compositeStackingContext(compositor);

            compositor->restore();

            mainBrowsingContext()->window()->scrolling()->paintScrollbars(
                compositor, mainFrame, mainFrame->appliedOverflowX(),
                mainFrame->appliedOverflowY());

            m_didCompositeBefore = true;
#ifdef STARFISH_ENABLE_VIRTUAL_CURSOR
            platformWindow()->paintVirtualCursor(compositor);
#endif
            delete compositor;
        }
        m_needsComposite = false;
    }

    m_needsRendering = false;
    m_inRendering = false;

#if defined(STARFISH_ENABLE_TEST)
    {
        if (g_fireOnloadEvent &&
            testCompatibleMode() ==
                StarFishTestCompatibleMode::ChromiumLayout) {
            if (g_enableDumpAsText && !g_DumpAsText_Async) {
                fprintf(stdout, "#READY\n");

                StringBuilder outStr;
                outStr.appendString(
                    String::createASCIIString("Content-Type: text/plain\n"));
                outStr.appendString(FrameTreeBuilder::dumpFrameTreeAsText(
                    m_topLevelBrowsingContext->document(), 0));
                fprintf(stdout, "%s\n",
                        outStr.finalize()->toUTF8NonGCString().c_str());
                fprintf(stdout, "#EOF\n");
                fprintf(stdout, "#EOF\n");
                fprintf(stdout, "#EOF\n");

                fprintf(stderr, "#EOF\n");
                g_enableDumpAsText = false;
                exit(0);
            }
        }

        if (g_fireOnloadEvent && g_referenceTestState > 0) {
            rtDoTest(m_topLevelBrowsingContext->document());
            return renderResult;
        }

        const char* path = getenv("SCREEN_SHOT");
        if (path && strlen(path) && g_fireOnloadEvent) {
            screenShotInRendering(this, path, []() {
                if (getenv("EXIT_AFTER_SCREEN_SHOT") &&
                    strlen(getenv("EXIT_AFTER_SCREEN_SHOT"))) {
                    exit(0);
                }
            });
        }

#ifdef STARFISH_ENABLE_PROFILE_LOADING
        if (g_fireOnloadEvent) {
            auto currentTime = timestamp();
            auto diff = currentTime - m_navigateStartingTime;
            STARFISH_LOG_INFO(
                "`STARFISH_ENABLE_PROFILE_LOADING` => %fms elapsed since "
                "starting loading\n",
                (float)diff);
            exit(0);
        }
#endif
    }
#endif

    m_needsContinuousRendering = false;

    bool needsContinuousRendering = false;

    if (m_activeAnimationExecutor.size()) {
        for (size_t i = 0; i < m_activeAnimationExecutor.size(); i++) {
            m_activeAnimationExecutor[i]
                ->window()
                ->browsingContext()
                ->registerNeedsLayoutInWebView();
        }
        needsContinuousRendering = true;
    }

    if (timer()->m_requestAnimationFrameHandler.size()) {
        needsContinuousRendering = true;
    }

    if (needsContinuousRendering) {
        m_needsContinuousRendering = true;
        m_needsRendering = true;
    }

    return renderResult;
}

void WebView::setNeedsFullRepainting()
{
    if (m_topLevelBrowsingContext && m_topLevelBrowsingContext->document()) {
        m_topLevelBrowsingContext->document()->document()->setNeedsPainting();
        STARFISH_LOG_INFO("WebView::setNeedsFullRepainting\n");
    }
}

void WebView::clearStackingContext()
{
    if (m_rootStackingContext) {
        StackingContext* ctx = m_rootStackingContext;
        std::function<void(StackingContext*)> clearSC =
            [&](StackingContext* ctx) {
                ctx->owner()->clearStackingContextIfNeeds();
                auto iter = ctx->childContexts().begin();
                while (iter != ctx->childContexts().end()) {
                    StackingContextChild* child = *iter;
                    auto iter2 = child->begin();
                    while (iter2 != child->end()) {
                        clearSC(*iter2);
                        iter2++;
                    }
                    iter++;
                }
            };
        clearSC(ctx);
        m_rootStackingContext = nullptr;
    }
}

void WebView::initRenderingFlags()
{
    m_lastRenderingTick = 0;
    m_inRendering = false;
    m_needsRendering = false;
    m_needsPainting = false;
    m_needsComposite = false;
    m_isActive = true;
    m_paintingDirtyRect =
        LayoutRect(0, 0, platformWindow()->width(), platformWindow()->height());
}

Node* WebView::focusedNode()
{
    if (!m_topLevelBrowsingContext) {
        return nullptr;
    }
    Node* node = mainBrowsingContext()->focusedNode();
    if (!node) {
        return nullptr;
    }

    while (node->isHTMLIFrameElement()) {
        if (node->asHTMLIFrameElement()->browsingContext()->focusedNode()) {
            node =
                node->asHTMLIFrameElement()->browsingContext()->focusedNode();
        } else {
            break;
        }
    }
    return node;
}

BrowsingContext* WebView::focusedBrowsingContext()
{
    if (!m_topLevelBrowsingContext) {
        return nullptr;
    }
    Node* node = mainBrowsingContext()->focusedNode();
    if (!node) {
        return m_topLevelBrowsingContext;
    }

    BrowsingContext* ctx = m_topLevelBrowsingContext;
    while (node->isHTMLIFrameElement()) {
        if (node->asHTMLIFrameElement()->browsingContext()) {
            ctx = node->asHTMLIFrameElement()->browsingContext();
            if (node->asHTMLIFrameElement()->browsingContext()->focusedNode()) {
                node = node->asHTMLIFrameElement()
                           ->browsingContext()
                           ->focusedNode();
            } else {
                break;
            }
        } else {
            break;
        }
    }
    return ctx;
}

bool WebView::hasFocus()
{
    return focusedNode() != nullptr;
}

void WebView::blur()
{
    if (!m_topLevelBrowsingContext) {
        return;
    }
    mainBrowsingContext()->releaseFocusedNode(nullptr);
}

void WebView::pause()
{
    if (m_isActive == false) {
        return;
    }
    STARFISH_LOG_INFO("WebView::pause\n");
    m_isActive = false;

    if (mainBrowsingContext()) {
        mainBrowsingContext()->pause();
    }
}

void WebView::resume()
{
    if (m_isActive) {
        return;
    }
    STARFISH_LOG_INFO("WebView::resume\n");
    m_isActive = true;

    if (mainBrowsingContext()) {
        mainBrowsingContext()->resume();
        if (mainBrowsingContext()->document()) {
            setNeedsFullRepainting();
        }
    }
}

void WebView::onIdle()
{
    if (m_topLevelBrowsingContext) {
        m_topLevelBrowsingContext->onIdle();
    }
}

void WebView::setDefaultFontSize(uint32_t size)
{
    m_defaultFontSize = size;
    mainBrowsingContext()->updateDefaultFontSize();
}

void WebView::addActiveThread(Thread* thread)
{
    STARFISH_ASSERT(isMainThread());
    m_activeThreadList.push_back(thread);
}

void WebView::removeActiveThread(Thread* thread)
{
    STARFISH_ASSERT(isMainThread());
    auto it =
        std::find(m_activeThreadList.begin(), m_activeThreadList.end(), thread);
    if (it != m_activeThreadList.end()) {
        m_activeThreadList.erase(it);
    }
}

#if defined(STARFISH_ENABLE_INSPECTOR)
void WebView::setupInspector(uint32_t portNumber)
{
    STARFISH_ASSERT(m_inspector == nullptr);
    m_inspector = new Inspector(this);
    m_inspector->run(portNumber);
}
#endif

void WebView::registerPublicWebViewHandler(
    const std::string& handlerName, std::function<void(String*, int)> handler)
{
    auto it = m_publicWebViewHandlers.find(handlerName);
    if (it == m_publicWebViewHandlers.end()) {
        m_publicWebViewHandlers.insert(std::make_pair(handlerName, handler));
    } else {
        it->second = handler;
    }
}

void WebView::registerPublicWebViewHandler(const std::string& handlerName,
                                           std::function<void(void*)> handler)
{
    auto it = m_publicWebViewHandlersGeneral.find(handlerName);
    if (it == m_publicWebViewHandlersGeneral.end()) {
        m_publicWebViewHandlersGeneral.insert(
            std::make_pair(handlerName, handler));
    } else {
        it->second = handler;
    }
}

bool WebView::containsPublicWebViewHandler(const std::string& handlerName)
{
    auto it = m_publicWebViewHandlersGeneral.find(handlerName);
    if (it != m_publicWebViewHandlersGeneral.end()) {
        return true;
    }

    return false;
}

void WebView::callPublicWebViewHandler(const std::string& handlerName,
                                       String* url, int param)
{
    auto it = m_publicWebViewHandlers.find(handlerName);
    if (it == m_publicWebViewHandlers.end()) {
        return;
    }

    struct dummy : public gc {
        std::string handlerName;
        WebView* webView;
        String* url;
        int int_param;
    };
    dummy* d = new dummy;
    d->handlerName = handlerName;
    d->webView = this;
    d->url = url;
    d->int_param = param;
    messageLoop()->addIdler(
        nullptr,
        [](size_t, void* data) {
            dummy* d = (dummy*)data;
            auto it = d->webView->m_publicWebViewHandlers.find(d->handlerName);
            if (it != d->webView->m_publicWebViewHandlers.end()) {
                (it->second)(d->url, d->int_param);
            }
            delete d;
        },
        d);
}

void WebView::callPublicWebViewHandler(const std::string& handlerName,
                                       void* param)
{
    auto it = m_publicWebViewHandlersGeneral.find(handlerName);
    if (it == m_publicWebViewHandlersGeneral.end()) {
        return;
    }

    struct Env {
        WebView* webView;
        std::string handlerName;
        void* param;
    };
    Env* env = new Env();
    env->webView = this;
    env->handlerName = handlerName;
    env->param = param;

    messageLoop()->addIdler(
        nullptr,
        [](size_t, void* env) {
            Env* e = (Env*)env;
            auto it =
                e->webView->m_publicWebViewHandlersGeneral.find(e->handlerName);
            if (it != e->webView->m_publicWebViewHandlersGeneral.end()) {
                (it->second)(e->param);
            }
            delete e;
        },
        env);
}
}
