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

#include "StarfishConfig.h"

#include "WebView.h"

#include "BrowsingContext.h"
#include "Starfish.h"
#include "PlatformIntegrationData.h"

#include "core/page/Window.h"
#include "core/style/Style.h"
#include "core/style/ComputedStyle.h"
#include "core/dom/Node.h"
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

#include "core/dom/MouseEvent.h"
#include "core/dom/KeyboardEvent.h"
#include "core/dom/Touch.h"
#include "core/dom/HTMLDocument.h"
#include "core/dom/HTMLBodyElement.h"
#include "core/dom/HTMLCollection.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/dom/HTMLIFrameElement.h"

#include "platform/window/PlatformWindow.h"
#include "platform/event/PlatformKeyEventData.h"
#include "platform/loader/ResourceLoader.h"
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
#include "core/modules/serviceworker/ServiceWorkerServiceHost.h"

#if defined(OS_POSIX)
#include <malloc.h>
#endif

#ifdef STREAMLINE_PROFILE
#include "streamline_annotate.h"
ANNOTATE_DEFINE;
#else
#define ANNOTATE_SETUP
#define ANNOTATE_CHANNEL_COLOR(channel, color, str)
#define ANNOTATE_CHANNEL_END(channel)
#define ANNOTATE_BLUE 0xff00001b
#endif

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
extern Starfish::CanvasSurface* g_surfaceForScreehShot;

int g_testCompatibleMode;
int g_startUpFlag;
#endif

namespace Starfish {
#if defined(STARFISH_ENABLE_TEST)
// should be defined in each window port
void screenShotInRendering(WebView* wv, const char* path,
                           std::function<void()> callback);
// WPT Reference Test
static Nullable<String*> rtExtractReference(Document* document)
{
    HTMLCollection* result = document->getElementsByTagName(
        document->starfish()->staticStrings()->m_link);
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

#ifndef STARFISH_FILLING_GRAPHICS_BUFFER_TIME_LIMIT
#define STARFISH_FILLING_GRAPHICS_BUFFER_TIME_LIMIT 25
#endif
size_t WebView::g_fillingGraphicsBufferTileFrameTimeLimitInMS =
    STARFISH_FILLING_GRAPHICS_BUFFER_TIME_LIMIT;

WebView* WebView::create(Starfish* starfish, const char* locale,
                         const char* timezoneID, uint32_t w, uint32_t h,
                         uint32_t defaultFontSize, String* defaultFontName,
                         const ScreenInfo& info, String* customUserAgentString,
                         String* builtinPolyfillPathString)
{
    return new WebView(starfish, locale, timezoneID, w, h, defaultFontSize,
                       defaultFontName, info, customUserAgentString,
                       builtinPolyfillPathString);
}

WebView::WebView(Starfish* starfish, const char* locale, const char* timezoneID,
                 uint32_t w, uint32_t h, uint32_t defaultFontSize,
                 String* defaultFontName, const ScreenInfo& info,
                 String* customUserAgentString,
                 String* builtinPolyfillPathString)
    : WebBase(starfish)
    , m_platformWindow(PlatformWindow::create(starfish, w, h))
    , m_topLevelBrowsingContext(nullptr)
    , m_scriptEngineInstance(nullptr)
    , m_storageNamespaceProvider(nullptr)
    , m_localStorageNamespace(nullptr)
    , m_sessionStorageNamespace(nullptr)
    , m_historyManager(nullptr)
    , m_lastRenderingTick(0)
    , m_navigateStartingTime(0)
    , m_currentActiveAnimatorCount(0)
    , m_inRendering(false)
    , m_needsRendering(false)
    , m_needsEstablishesStackingContext(false)
    , m_needsComputeStackingContextProperties(false)
    , m_needsPainting(false)
    , m_needsComposite(false)
    , m_needsContinuousRendering(false)
    , m_needsFullPainting(false)
    , m_didCompositeBefore(false)
    , m_isActive(false)
    , m_inIdleMode(false)
    , m_rootStackingContext(nullptr)
    , m_messageLoop(new MessageLoop())
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
    , m_testCompatibleMode(StarfishTestCompatibleMode::Normal)
#endif
    , m_baseBackgroundColor(Unit::Color(255, 255, 255, 255))
    , m_baseForegroundColor(Unit::Color(0, 0, 0, 255))
    , m_webSecurityMode(WebSecurityMode::Enable)
    , m_idleModeJob(LWE::IdleModeJob::IdleModeDefault)
    , m_idleModeCheckIntervalInMS(0)
    , m_idleCheckTimerID(TimerInvalidID)

{
    m_platformWindow->setWebView(this);
    m_deviceKind = deviceKindUseTouchScreen;
#ifdef STARFISH_ENABLE_TEST
    m_testCompatibleMode = g_testCompatibleMode;
    m_startUpFlag = g_startUpFlag;
#else
    m_startUpFlag = 0;
#endif

#ifndef STARFISH_THREAD_POOL_SIZE
#define STARFISH_THREAD_POOL_SIZE 6
#endif
    m_threadPool = new ThreadPool(STARFISH_THREAD_POOL_SIZE, m_messageLoop);
    m_historyManager = HistoryManager::create(this);
    initRenderingFlags();
    initStorage();

    AtomicString atomicDefaultFontName =
        AtomicString::createAtomicString(starfish, defaultFontName);

    m_initialFontFamilyDatas =
        (new (GC_MALLOC_ATOMIC(sizeof(FontFamilyData) * 2))
             FontFamilyData[2]{ 1, atomicDefaultFontName });

    m_platformFontSelector = PlatformFontSelector::create(this);
    m_platformFontCache = PlatformFontCache::create(this);

    // saidly.. few port layer needs this variable
    m_publicLayerUserDataMap["__internalWebContainerImplementLayerVariable"] =
        this;

#ifdef STARFISH_ENABLE_SERVICE_WORKER
    // TODO: creating host from a certain host manager
    ServiceWorkerServiceHost::getInstance()->init(m_messageLoop);
#endif

    m_starfish->m_webViewInstanceCount++;

    setIdleModeCheckIntervalInMS(IdleModeCheckDefaultIntervalInMS);
}

void WebView::setIdleModeCheckIntervalInMS(uint32_t i)
{
    if (m_idleModeCheckIntervalInMS != i) {
        m_idleModeCheckIntervalInMS = i;
        m_timer->removeTimer(m_idleModeCheckIntervalInMS);
        m_idleCheckTimerID = m_timer->addTimer(
            m_idleModeCheckIntervalInMS, nullptr,
            [](void* data) {
                WebView* wv = (WebView*)data;
                uint64_t currentTick = longTickCount();
                if (!wv->m_inIdleMode &&
                    currentTick - wv->m_lastRenderingTick >
                        wv->m_idleModeCheckIntervalInMS * 1000) {
                    wv->enterIdleMode();
                }
            },
            this, true);
    }
}

void WebView::enterIdleMode()
{
    STARFISH_LOG_INFO("enter idle mode\n");
    m_inIdleMode = true;

    onIdle();

    // drop CanvasSurfaces if possible
    if (((int)m_idleModeJob & (int)LWE::IdleModeJob::ClearDrawnBuffers) &&
        m_didCompositeBefore) {
        LongTaskFinder f("drop CanvasSurfaces when entering idle mode");
        auto iter = m_stackingContextsNeedsGraphicsBuffer.begin();
        while (iter != m_stackingContextsNeedsGraphicsBuffer.end()) {
            StackingContext* sc = *iter;
            iter++;
            if (!sc->owner()->hasOwnGraphicsBufferMethod()) {
                auto holder = sc->graphicsBufferHolder();
                if (holder) {
                    for (size_t i = 0; i < holder->m_surfaces.size(); i++) {
                        if (holder->m_surfaces[i]) {
                            holder->m_surfaces[i]->detachNativeBuffer();
                            holder->m_surfaces[i] = nullptr;
                        }
                    }
                }
            }
        }
    }

    if (((int)m_idleModeJob & (int)LWE::IdleModeJob::ForceGC)) {
        LongTaskFinder f("force gc when entering idle mode");
        GC_gcollect();
        GC_gcollect();
        GC_gcollect_and_unmap();
    }

    if (((int)m_idleModeJob & (int)LWE::IdleModeJob::DropDecodedImageBuffer)) {
        LongTaskFinder f(
            "drop decoded image datas in NativeImageData when entering idle "
            "mode");
        auto& globalImages = NativeImageData::everyNativeImageInstances();
        for (size_t i = 0; i < globalImages.size(); i++) {
            globalImages[i]->pruneInternalDataIfPossible();
        }
    }

#if defined(OS_POSIX) && !defined(STARFISH_ANDROID)
    if (((int)m_idleModeJob & (int)LWE::IdleModeJob::ForceGC)) {
        LongTaskFinder f("calling malloc_trim when entering idle mode");
        malloc_trim(0);
    }
#endif
}

void WebView::addJavaScriptNativeInterface(
    String* exposedObjectName, String* jsFunctionName, void* scriptObject,
    Escargot::ScriptNativeFunctionPointer scriptNativeFunctionPointer)
{
    m_jsInterfaceList.push_back(std::make_tuple(exposedObjectName,
                                                jsFunctionName, scriptObject,
                                                scriptNativeFunctionPointer));
}

void WebView::removeJavaScriptNativeInterface(String* exposedObjectName,
                                              String* jsFunctionName)
{
    m_jsInterfaceList.erase(
        std::remove_if(
            m_jsInterfaceList.begin(), m_jsInterfaceList.end(),
            [exposedObjectName, jsFunctionName](
                const std::tuple<String*, String*, void*,
                                 Escargot::ScriptNativeFunctionPointer>& e) {
                return exposedObjectName->equals(std::get<0>(e)) &&
                       jsFunctionName->equals(std::get<1>(e));
            }),
        m_jsInterfaceList.end());
}

void WebView::applyJavaScriptNativeInterface(ScriptBindingInstance* instance)
{
    for (auto it = m_jsInterfaceList.begin(); it != m_jsInterfaceList.end();
         it++) {
        registerJavaScriptNativeInterface(instance, std::get<0>(*it),
                                          std::get<1>(*it), std::get<2>(*it),
                                          std::get<3>(*it));
    }
}

void WebView::destroy()
{
    STARFISH_LOG_INFO("WebView::destroy\n");
#if defined(STARFISH_ENABLE_INSPECTOR)
    delete m_inspector;
    m_inspector = nullptr;
#endif

    pause();

#ifdef STARFISH_ENABLE_TTS
    m_tts->destroy();
#endif

    m_repaintRegionInRendering.clear();
    m_globalPointingEventListener.clear();
    m_jsInterfaceList.clear();

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

    removeScriptEngineInstance();

    m_threadPool->destroy();
    m_messageLoop->destroy();

    m_timer->clear(nullptr);
    m_timer->destroy();

    m_publicLayerUserDataMap.clear();

    delete m_platformFontCache;
    delete m_platformFontSelector;

    m_platformWindow->destroy();
    removeScriptEngineInstance();

    m_starfish->m_webViewInstanceCount--;
    this->WebView::~WebView();

    m_platformWindow->clearNativeHandlers();

    clearStack<ELABORATE_CLEAR_STACK_SIZE>();
}

void WebView::createScriptEngineInstance()
{
    if (!m_scriptEngineInstance) {
        PromiseJobListener listener = [](ExecutionStateRef* state,
                                         JobRef* job) {
            // web view on loop
            Window* window =
                (Window*)state->context()->globalObject()->extraData();

            window->webView()->messageLoop()->addIdler(
                window->executionContext(),
                [](size_t, void* data, void* data2) {
                    Window* window = (Window*)data;

                    if (!window->webView()->isActive()) {
                        return;
                    }

                    JobRef* job = (JobRef*)data2;
                    auto sbresult = job->run();

                    if (sbresult.error.hasValue()) {
                        STARFISH_LOG_ERROR(
                            "Uncaught %s\n",
                            toBrowserString(
                                window->scriptBindingInstance(),
                                ValueRef::create(sbresult.error.getValue()))
                                ->toUTF8NonGCString()
                                .data());
                    }
                },
                window, job);
        };

        m_scriptEngineInstance = new ScriptEngineInstance(
            locale().getName(), timezoneID()->toUTF8NonGCString().data(),
            listener);
    }
}

void WebView::removeScriptEngineInstance()
{
    if (m_scriptEngineInstance) {
        m_scriptEngineInstance->dispose();

        delete m_scriptEngineInstance;
        m_scriptEngineInstance = nullptr;
    }
}

void WebView::initStorage()
{
    // TODO: The name of disk storage file name should be auto-generated
    m_storageNamespaceProvider =
        WebStorageNamespaceProvider::create(m_starfish->localStorageFilePath());
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
    ReferrerURL* rUrl = new ReferrerURL(String::emptyString);
    navigate(url, HistoryManagerAction::Add, rUrl);
}

void WebView::navigate(ResourceURL* url, HistoryManagerAction type,
                       ReferrerURL* referrerURL)
{
    clearBlobURLStore();
    clearMediaSourceBlobURLStore();
    initRenderingFlags();

    clearStack<ELABORATE_CLEAR_STACK_SIZE>();

    m_navigateStartingTime = timestamp();
    if (m_topLevelBrowsingContext) {
        m_topLevelBrowsingContext->dispose();
    }
    platformWindow()->hideSoftwareKeyboardIfPossible();
    m_topLevelBrowsingContext = BrowsingContext::create(this);

    removeScriptEngineInstance();
    createScriptEngineInstance();

    m_topLevelBrowsingContext->open(url, type, referrerURL);
    applyJavaScriptNativeInterface(
        mainBrowsingContext()->scriptBindingInstance());
    struct Param : public gc {
        String* url;
    };
    Param* p = new Param;
    p->url = url->urlString();
    callPublicWebViewHandler(OnPageStarted, p);
}

String* WebView::userAgent()
{
    String* custom = customUserAgentString();
    if (custom->length()) {
        return custom;
    }
    return String::createASCIIString(USER_AGENT(STARFISH_NAME, VERSION));
}

LWE::WebSecurityMode WebView::getWebSecurityMode() const
{
    return m_webSecurityMode;
}

void WebView::setWebSecurityMode(LWE::WebSecurityMode value)
{
    m_webSecurityMode = value;
}

String* WebView::evaluateJavaScript(String* s)
{
    if (mainBrowsingContext()) {
        ANNOTATE_SETUP;
        ANNOTATE_CHANNEL_COLOR(3003, ANNOTATE_BLUE,
                               "WebView::evaluateJavaScript");
        String* result = toBrowserString(
            mainBrowsingContext()->scriptBindingInstance(),
            evaluateString(mainBrowsingContext()->scriptBindingInstance(), s));
        ANNOTATE_CHANNEL_END(3003);
        return result;

    } else {
        return String::emptyString;
    }
}

void WebView::evaluateJavaScript(String* s, std::function<void(std::string)> cb)
{
    String* ret = String::emptyString;

    if (mainBrowsingContext()) {
        toBrowserString(
            mainBrowsingContext()->scriptBindingInstance(),
            evaluateString(mainBrowsingContext()->scriptBindingInstance(), s));
    }
    cb(ret->toUTF8NonGCString());
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
    a.m_a = rand_r(&m_seed);
    a.m_b = rand_r(&m_seed);
    a.m_c = rand_r(&m_seed);
#else
    a.m_a = rand_r(&m_seed);
    a.m_b = rand_r(&m_seed);
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

void WebView::clearMediaSourceBlobURLStore()
{
    m_urlMediaSourceBlobStore.clear();
    GCUnorderedSet<BlobURLStore>().swap(m_urlMediaSourceBlobStore);
}

void WebView::layoutIfNeeded(bool shouldCareStackingContextNow)
{
    INSTALL_PROFILE_TIMER("WebView::rendering::layoutIfNeeded");
    bool didLayout = false;

    {
        didLayout = didLayout | m_topLevelBrowsingContext->layoutIfNeeded();
        auto browsingContextsNeedsLayout =
            std::move(m_browsingContextsNeedsLayout);
        m_browsingContextsNeedsLayout.clear();
        for (size_t i = 0; i < browsingContextsNeedsLayout.size(); i++) {
            didLayout =
                didLayout | browsingContextsNeedsLayout[i]->layoutIfNeeded();
        }

        if (didLayout) {
            m_needsEstablishesStackingContext = true;
        }
    }

    if (shouldCareStackingContextNow) {
        if (!m_rootStackingContext || m_needsEstablishesStackingContext) {
            INSTALL_PROFILE_TIMER("establishesStackingContext");
            clearStackingContext();
#ifdef STARFISH_ENABLE_TEST
            if (startUpFlag() & StarfishStartUpFlag::enableComputedStyleDump) {
                // dump style
                m_topLevelBrowsingContext->document()
                    ->styleResolver()
                    .dumpDOMStyle(m_topLevelBrowsingContext->document());
            }
            if (startUpFlag() & StarfishStartUpFlag::enableFrameTreeDump) {
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
                INSTALL_PROFILE_TIMER("computeStackingContextProperties");
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
                StarfishStartUpFlag::enableStackingContextDump) {
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
                                    "%f "
                                    "%f %f %f]",
                                    depth / 2, ctx, element,
                                    utf8DataLog1.data(), utf8DataLog2.data(),
                                    className.data(), ctx->owner(),
                                    (int)ctx->needsGraphicsBuffer(),
                                    ctx->owner()->style()->opacity(),
                                    (float)se.x(), (float)se.y(),
                                    (float)se.width(), (float)se.height(),
                                    (float)fr.x(), (float)fr.y(),
                                    (float)fr.width(), (float)fr.height());
                            } else {
                                printf(
                                    "StackingContext[%d][%p, anonymous node"
                                    ", frame %p, buf %d opacity %f "
                                    "screenExtent %f %f %f %f visibleRect %f "
                                    "%f %f %f]",
                                    depth / 2, ctx, ctx->owner(),
                                    (int)ctx->needsGraphicsBuffer(),
                                    ctx->owner()->style()->opacity(),
                                    (float)se.x(), (float)se.y(),
                                    (float)se.width(), (float)se.height(),
                                    (float)fr.x(), (float)fr.y(),
                                    (float)fr.width(), (float)fr.height());
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

    clearStack<DEFAULT_CLEAR_STACK_SIZE>();
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

static void cleanupLayoutRepaintTracker(BrowsingContext* ctx)
{
    ctx->layoutRepaintTracker().clearDatasRelatedWithStackingContext();
    ctx->iterateChildContext(
        [](BrowsingContext* ctx) { cleanupLayoutRepaintTracker(ctx); });
}

static void saveCurrentPaintingState(StackingContext* ctx)
{
    PrevDrawnStackingContextInfo info;
    if (ctx->isIFrameStackingContext()) {
        info.screenExtent = ctx->parent()->screenExtent();
    } else {
        info.screenExtent = ctx->screenExtent();
    }
    info.opacity = ctx->owner()->style()->opacity();
    info.needsGraphicsBuffer = ctx->needsGraphicsBuffer();
    info.transformMatrix = ctx->transformMatrix();

    if (info.needsGraphicsBuffer) {
        info.graphicsBufferVisibleRect = ctx->visibleRect();
        info.graphicsBufferHolder = ctx->graphicsBufferHolder();
    } else {
        StackingContext* owner = ctx->parent();
        while (owner != nullptr && !owner->needsGraphicsBuffer()) {
            owner = owner->parent();
        }

        if (owner) {
            info.graphicsLayerOwner = owner->owner()->node();
        }
        info.extentOnGraphicsLayer =
            computeBoxExtent(ctx->owner()->frameVisibleRect(),
                             ctx->owner()->computeMatrixOnGraphicsBuffer());
    }

    ctx->owner()->node()->webView()->prevDrawnStackingContextInfo().insert(
        std::make_pair(ctx->owner()->node(), info));

    auto iter = ctx->childContexts().begin();
    while (iter != ctx->childContexts().end()) {
        StackingContextChild* child = *iter;
        auto iter2 = child->begin();
        while (iter2 != child->end()) {
            StackingContext* childCtx = *iter2;
            saveCurrentPaintingState(childCtx);
            iter2++;
        }
        iter++;
    }
}

RenderResult WebView::rendering(bool force)
{
    RenderResult renderResult;
    renderResult.didPaintingOrCompositing = false;
    if (!m_needsRendering || !m_isActive) {
        return renderResult;
    }

    m_inIdleMode = false;

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
    ANNOTATE_SETUP;
    ANNOTATE_CHANNEL_COLOR(3001, ANNOTATE_BLUE, "WebView::rendering");
    INSTALL_PROFILE_TIMER("WebView::rendering");

    {
        auto rafHandlers = std::move(timer()->m_requestAnimationFrameHandler);
        if (rafHandlers.size()) {
            INSTALL_PROFILE_TIMER(
                "WebView::rendering::call request animation frame handlers");

            auto iter = rafHandlers.begin();
            while (iter != rafHandlers.end()) {
                Timer::RequestAnimationFrameData* data = iter->second;
                data->m_handler(data->m_data);
                iter++;
            }
        }
    }

    size_t totalAllocatedCanvasSurfaceSizeBefore =
        CanvasSurface::g_totalAllocatedCanvasSurfaceSize;

    layoutIfNeeded();

    bool didPainting = false;
    if (m_needsPainting) {
        didPainting = true;
        INSTALL_PROFILE_TIMER("painting");
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

        if (!m_didCompositeBefore && m_needsComposite) {
            STARFISH_LOG_INFO("Start composite mode\n");
        } else if (m_didCompositeBefore && !m_needsComposite) {
            STARFISH_LOG_INFO("End composite mode\n");
        }

        bool needsFullPainting =
            (m_didCompositeBefore && !m_needsComposite) || m_needsFullPainting;
        m_needsFullPainting = false;

        if (mainBrowsingContext()->document()->frame()->firstChild() &&
            mainBrowsingContext()
                ->document()
                ->frame()
                ->firstChild()
                ->isAbsolutePositioned()) {
            needsFullPainting = true;
        }

        {
            FrameBlockBox* mainFrame =
                mainBrowsingContext()->document()->frame()->asFrameBlockBox();
            STARFISH_ASSERT(mainFrame != nullptr);

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
                needsFullPainting = true;
            }

            auto prevDrawnStackingContextInfo =
                std::move(m_prevDrawnStackingContextInfo);
            RepaintRegionTracker tracker(
                mainBrowsingContext()->document()->frame()->asFrameBlockBox(),
                needsFullPainting, prevDrawnStackingContextInfo, scrollX,
                scrollY, m_needsComposite);
            m_repaintRegionInRendering = std::move(tracker.repaintRegion());
            auto repaintRect = m_repaintRegionInRendering[nullptr];
            cleanupLayoutRepaintTracker(mainBrowsingContext());

#if defined(STARFISH_ENABLE_PROFILE_TIMER)
            {
                auto iter = m_repaintRegionInRendering.begin();
                while (iter != m_repaintRegionInRendering.end()) {
                    if (iter->first) {
                        STARFISH_LOG_INFO(
                            "repaint region node %s #%s className(%s) %f %f %f "
                            "%f\n",
                            iter->first->localName()
                                ->toUTF8NonGCString()
                                .data(),
                            iter->first->asElement()
                                ->id()
                                ->toUTF8NonGCString()
                                .data(),
                            iter->first->asElement()
                                ->className()
                                ->toUTF8NonGCString()
                                .data(),
                            (float)iter->second.x(), (float)iter->second.y(),
                            (float)iter->second.width(),
                            (float)iter->second.height());
                    } else {
                        STARFISH_LOG_INFO(
                            "repaint region (screen) %f %f %f %f\n",
                            (float)iter->second.x(), (float)iter->second.y(),
                            (float)iter->second.width(),
                            (float)iter->second.height());
                    }
                    iter++;
                }
            }
#endif

            renderResult.computedRepaintRect = repaintRect;

            {
                // remove definitely useless graphics buffer first.
                auto iter = prevDrawnStackingContextInfo.begin();
                while (iter != prevDrawnStackingContextInfo.end()) {
                    if (iter->second.graphicsBufferHolder) {
                        if (!iter->first->frame() ||
                            !iter->first->frame()->isFrameBox() ||
                            !iter->first->frame()
                                 ->asFrameBox()
                                 ->stackingContext() ||
                            !iter->first->frame()
                                 ->asFrameBox()
                                 ->stackingContext()
                                 ->needsGraphicsBuffer()) {
                            if (iter->second.graphicsBufferHolder) {
                                iter->second.graphicsBufferHolder
                                    ->detachNativeBuffers();
                                iter->second.graphicsBufferHolder = nullptr;
                            }
                        }
                    }
                    iter++;
                }
            }

            StackingContext::PaintingStackingContextContext ctx(
                m_needsComposite, prevDrawnStackingContextInfo, repaintRect,
                m_repaintRegionInRendering, scrollX, scrollY);
            if (!m_needsComposite) {
                canvas = platformWindow()->preparePainting();
                canvas->save();
                canvas->pixelSnappedClip(repaintRect);
                canvas->translate(-scrollX, -scrollY);
                canvas->translate(-additionalX, -additionalY);

                if (!mainBrowsingContext()->hasWindowBackgroundColor().first) {
                    mainBrowsingContext()->clearingBeforePaint(canvas);
                }

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

                auto iter = m_stackingContextsNeedsGraphicsBuffer.begin();
                while (iter != m_stackingContextsNeedsGraphicsBuffer.end()) {
                    (*iter)->fillGraphicsBufferContents(ctx);
                    iter++;
                }
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
                if (iter->second.graphicsBufferHolder) {
                    iter->second.graphicsBufferHolder->detachNativeBuffers();
                    iter->second.graphicsBufferHolder = nullptr;
                }
                iter++;
            }
            if (m_rootStackingContext) {
                saveCurrentPaintingState(m_rootStackingContext);
            }
        }

        m_needsPainting = false;
#ifdef STARFISH_ENABLE_VIRTUAL_CURSOR
        if (!m_needsComposite) {
            platformWindow()->paintVirtualCursor(canvas);
        }
#endif

        delete canvas;
        clearStack<DEFAULT_CLEAR_STACK_SIZE>();
    }

    bool someTilesSkippedPaintingDueToTimeOver = false;
    if (m_needsComposite) {
        INSTALL_PROFILE_TIMER("composite");
        renderResult.didPaintingOrCompositing = true;
        renderResult.updateRect = LayoutRect(0, 0, platformWindow()->width(),
                                             platformWindow()->height());

        if (mainBrowsingContext()->document()->frame()->firstChild() &&
            m_rootStackingContext->needsGraphicsBuffer()) {
            if (!didPainting) {
                INSTALL_PROFILE_TIMER("composite - fill blank tiles");
                // fill blank tiles first before using 3d context
                auto iter = m_stackingContextsNeedsGraphicsBuffer.begin();
                while (iter != m_stackingContextsNeedsGraphicsBuffer.end()) {
                    if ((*iter)->fillGraphicsBufferContentsWithoutClipRect()) {
                        someTilesSkippedPaintingDueToTimeOver = true;
                        break;
                    }
                    iter++;
                }
            }

            Compositor* compositor = platformWindow()->prepareCompositor();
            FrameBlockBox* mainFrame =
                mainBrowsingContext()->document()->frame()->asFrameBlockBox();

            compositor->save();
            compositor->translate(-mainFrame->scrollLeft(),
                                  -mainFrame->scrollTop());
            auto bgColor = mainBrowsingContext()->hasWindowBackgroundColor();
            bool colorFill = bgColor.first;
            if (colorFill) {
                compositor->clearColor(bgColor.second);
            } else {
                mainBrowsingContext()->clearingBeforePaint(compositor);
            }

            {
                auto iter = m_stackingContextsNeedsGraphicsBuffer.begin();
                while (iter != m_stackingContextsNeedsGraphicsBuffer.end()) {
                    (*iter)->compositeStackingContext(compositor);
                    iter++;
                }
            }

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

    size_t totalAllocatedCanvasSurfaceSizeAfter =
        CanvasSurface::g_totalAllocatedCanvasSurfaceSize;

    if (totalAllocatedCanvasSurfaceSizeBefore !=
        totalAllocatedCanvasSurfaceSizeAfter) {
        STARFISH_LOG_INFO("totalAllocatedCanvasSurfaceSize %fMB\n",
                          totalAllocatedCanvasSurfaceSizeAfter / 1024.f /
                              1024.f);
    }

#if defined(STARFISH_ENABLE_TEST)
    {
        if (g_fireOnloadEvent &&
            testCompatibleMode() ==
                StarfishTestCompatibleMode::ChromiumLayout) {
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
                ANNOTATE_CHANNEL_END(3001);
                exit(0);
            }
        }

        if (g_fireOnloadEvent && g_referenceTestState > 0) {
            rtDoTest(m_topLevelBrowsingContext->document());
            ANNOTATE_CHANNEL_END(3001);
            return renderResult;
        }

        const char* path = getenv("SCREEN_SHOT");
        if (path && strlen(path) && g_fireOnloadEvent) {
            screenShotInRendering(this, path, []() {
                if (getenv("EXIT_AFTER_SCREEN_SHOT") &&
                    strlen(getenv("EXIT_AFTER_SCREEN_SHOT"))) {
                    ANNOTATE_CHANNEL_END(3001);
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
            ANNOTATE_CHANNEL_END(3001);
            exit(0);
        }
#endif
    }
#endif

    m_needsContinuousRendering = false;

    bool needsContinuousRendering = false;

    if (m_activeAnimationExecutor.size()) {
        for (size_t i = 0; i < m_activeAnimationExecutor.size(); i++) {
            auto& a = m_activeAnimationExecutor[i]->activeAnimations();
            for (size_t j = 0; j < a.size(); j++) {
                a[j]->targetElement()->setNeedsStyleRecalcForAnimation();
            }
        }
        needsContinuousRendering = true;
    }

    if (timer()->m_requestAnimationFrameHandler.size()) {
        needsContinuousRendering = true;
    }

    if (someTilesSkippedPaintingDueToTimeOver) {
        needsContinuousRendering = true;
        m_needsComposite = true;
    }

    if (needsContinuousRendering) {
        m_needsContinuousRendering = true;
        m_needsRendering = true;
        GC_set_free_space_divisor(1);
    } else {
        GC_set_free_space_divisor(BDWGC_FREE_SPACE_DIVISOR);
    }

    ANNOTATE_CHANNEL_END(3001);
    return renderResult;
}

void WebView::setNeedsFullRepainting()
{
    STARFISH_LOG_INFO("WebView::setNeedsFullRepainting\n");
    markNeedsPaintingConsiderInRendering();
    m_needsFullPainting = true;
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
    m_needsFullPainting = true;
    m_isActive = true;
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
    if (mainBrowsingContext()) {
        mainBrowsingContext()->updateDefaultFontSize();
    }
}

void WebView::dispatchTouchEvent(TouchEventKind kind, TouchData* touches,
                                 size_t touchCount)
{
    if (m_globalPointingEventListener.size()) {
        float x, y;
        if (kind == TouchEventKind::TouchEventStart ||
            kind == TouchEventKind::TouchEventMove) {
            x = touches[0].screenX();
            y = touches[0].screenY();
        } else {
            x = std::numeric_limits<float>::quiet_NaN();
            y = std::numeric_limits<float>::quiet_NaN();
        }
        Node::GlobalPointingEventKind newKind;
        if (kind == TouchEventKind::TouchEventStart) {
            newKind =
                Node::GlobalPointingEventKind::GlobalPointingEventKindDown;
        } else if (kind == TouchEventKind::TouchEventMove) {
            newKind =
                Node::GlobalPointingEventKind::GlobalPointingEventKindMove;
        } else {
            newKind = Node::GlobalPointingEventKind::GlobalPointingEventKindUp;
        }
        for (size_t i = 0; i < m_globalPointingEventListener.size();) {
            EventTarget* nd = m_globalPointingEventListener[i];
            nd->onGlobalPointingEvent(x, y, newKind);
            if (std::find(m_globalPointingEventListener.begin(),
                          m_globalPointingEventListener.end(),
                          nd) != m_globalPointingEventListener.end()) {
                i++;
            }
        }

        if (kind == TouchEventKind::TouchEventEnd) {
            for (size_t i = 0; i < m_globalPointingEventListener.size(); i++) {
                m_globalPointingEventListener[i]
                    ->document()
                    ->browsingContext()
                    ->releaseActiveNode();
            }
        }
    }

    if (mainBrowsingContext()) {
        mainBrowsingContext()->dispatchTouchEvent(kind, touches, touchCount);
    }
}

void WebView::dispatchMouseEvent(MouseEventKind kind, MouseData data)
{
    if (m_globalPointingEventListener.size()) {
        float x, y;
        if (kind == MouseEventKind::MouseEventDown ||
            kind == MouseEventKind::MouseEventMove) {
            x = data.screenX();
            y = data.screenY();
        } else {
            x = std::numeric_limits<float>::quiet_NaN();
            y = std::numeric_limits<float>::quiet_NaN();
        }
        Node::GlobalPointingEventKind newKind;
        if (kind == MouseEventKind::MouseEventDown) {
            newKind =
                Node::GlobalPointingEventKind::GlobalPointingEventKindDown;
        } else if (kind == MouseEventKind::MouseEventMove) {
            newKind =
                Node::GlobalPointingEventKind::GlobalPointingEventKindMove;
        } else {
            newKind = Node::GlobalPointingEventKind::GlobalPointingEventKindUp;
        }
        for (size_t i = 0; i < m_globalPointingEventListener.size();) {
            EventTarget* nd = m_globalPointingEventListener[i];
            nd->onGlobalPointingEvent(x, y, newKind);
            if (std::find(m_globalPointingEventListener.begin(),
                          m_globalPointingEventListener.end(),
                          nd) != m_globalPointingEventListener.end()) {
                i++;
            }
        }

        if (kind == MouseEventKind::MouseEventUp) {
            for (size_t i = 0; i < m_globalPointingEventListener.size(); i++) {
                m_globalPointingEventListener[i]
                    ->document()
                    ->browsingContext()
                    ->releaseActiveNode();
            }
        }
    }

    if (mainBrowsingContext()) {
        mainBrowsingContext()->dispatchMouseEvent(kind, data);
    }
}

void WebView::dispatchMouseWheelEvent(float screenX, float screenY, int z,
                                      bool isVerticalWheelEvent)
{
    if (mainBrowsingContext()) {
        mainBrowsingContext()->dispatchMouseWheelEvent(screenX, screenY, z,
                                                       isVerticalWheelEvent);
    }
}

void WebView::dispatchKeyEvent(KeyEventKind kind, PlatformKeyEventData data)
{
    if (mainBrowsingContext()) {
        mainBrowsingContext()->dispatchKeyEvent(kind, data);
    }
}

void WebView::dispatchCompositionEvent(CompositionEventKind kind, String* data,
                                       Node* node)
{
    if (mainBrowsingContext()) {
        mainBrowsingContext()->dispatchCompositionEvent(kind, data, node);
    }
}

void WebView::addGlobalPointingEventInterceptListener(EventTarget* node)
{
    size_t sizeBefore = m_globalPointingEventListener.size();
    if (sizeBefore == 0) {
        MouseData mdata(MouseButtonValue::NoButton,
                        MouseButtonsValue::NoButtonDown,
                        m_lastMouseMovePoint.x(), m_lastMouseMovePoint.y(), 0);
        mdata.setDefaultPrevented();

        node->document()->browsingContext()->dispatchMouseEvent(
            MouseEventKind::MouseEventUp, mdata);
    }

    auto iter = std::find(m_globalPointingEventListener.begin(),
                          m_globalPointingEventListener.end(), node);
    if (iter == m_globalPointingEventListener.end()) {
        m_globalPointingEventListener.insert(
            m_globalPointingEventListener.end(), node);
    }
}

void WebView::removeGlobalPointingEventInterceptListener(EventTarget* node)
{
    auto iter = std::find(m_globalPointingEventListener.begin(),
                          m_globalPointingEventListener.end(), node);
    if (iter != m_globalPointingEventListener.end()) {
        m_globalPointingEventListener.erase(iter);
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
    StarfishPubicWebViewHandlerKind handlerKind,
    std::function<void(void*)> handler)
{
    auto it = m_publicWebViewHandlers.find(handlerKind);
    if (it == m_publicWebViewHandlers.end()) {
        m_publicWebViewHandlers.insert(std::make_pair(handlerKind, handler));
    } else {
        it->second = handler;
    }
}

bool WebView::containsPublicWebViewHandler(
    StarfishPubicWebViewHandlerKind handlerKind)
{
    auto it = m_publicWebViewHandlers.find(handlerKind);
    if (it != m_publicWebViewHandlers.end()) {
        return true;
    }

    return false;
}

void WebView::callPublicWebViewHandler(
    StarfishPubicWebViewHandlerKind handlerKind, void* param)
{
    auto it = m_publicWebViewHandlers.find(handlerKind);
    if (it == m_publicWebViewHandlers.end()) {
        return;
    }

    struct Env : public gc {
        WebView* webView;
        StarfishPubicWebViewHandlerKind handlerKind;
        void* param;
    };
    Env* env = new Env();
    env->webView = this;
    env->handlerKind = handlerKind;
    env->param = param;

    messageLoop()->addIdler(
        nullptr,
        [](size_t, void* env) {
            Env* e = (Env*)env;
            auto it = e->webView->m_publicWebViewHandlers.find(e->handlerKind);
            if (it != e->webView->m_publicWebViewHandlers.end()) {
                (it->second)(e->param);
            }
        },
        env);
}
}
