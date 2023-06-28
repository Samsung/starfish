/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

// #define STARFISH_ENABLE_PROFILE_TIMER
// #define STARFISH_ENABLE_PROFILE_LOADING

#ifdef STARFISH_ENABLE_PROFILE_LOADING_TIMER
#ifndef STARFISH_ENABLE_TEST
#error \
    "`STARFISH_ENABLE_PROFILE_LOADING_TIMER` flag needs `STARFISH_ENABLE_TEST`"
#endif
#endif

#include <SkMatrix.h>

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
#include "core/modules/threading/Mutex.h"
#include "core/modules/threading/Locker.h"
#include "core/util/URL.h"

#include "core/animation/AnimationTask.h"

#include "core/dom/MouseEvent.h"
#include "core/dom/KeyboardEvent.h"
#include "core/dom/Touch.h"
#include "core/dom/PseudoElement.h"
#include "core/dom/HTMLDocument.h"
#include "core/dom/HTMLBodyElement.h"
#include "core/dom/HTMLCollection.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/dom/HTMLIFrameElement.h"
#include "core/dom/Scrolling.h"
#include "core/page/PopStateEvent.h"

#include "platform/window/PlatformWindow.h"
#include "platform/event/PlatformKeyEventData.h"
#include "platform/loader/ResourceLoader.h"
#include "core/dom/Document.h"
#include "core/dom/ExecutionContext.h"
#include "core/storage/Storage.h"
#include "core/storage/StorageNamespace.h"
#include "core/storage/WebStorageNamespaceProvider.h"
#include "browser/history/HistoryManager.h"
#include "binding/ScriptEngineInstance.h"
#include "core/inspector/Inspector.h"
#include "core/style/ComputedStyle.h"
#include "platform/file/PlatformFile.h"
#include "core/modules/canvas/image/BufferedNativeImageData.h"

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
int g_exitCode;
#endif

namespace Starfish {
#if defined(STARFISH_ENABLE_TEST)
// should be defined in each window port
void screenShotInRendering(WebView* wv, const char* path,
                           std::function<void()> callback);
// WPT Reference Test
static Nullable<String*> rtExtractReference(Document* document)
{
    STARFISH_ASSERT(document != nullptr);

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
    STARFISH_ASSERT(wv != nullptr);
    STARFISH_ASSERT(msg != nullptr);

    if (!condition) {
        STARFISH_LOG_INFO("STARFISH_RTERROR %s", msg);
        exit(0);
    }
}
// WPT Reference Test
static void rtShouldLoaded(Document* document, const char* msg)
{
    STARFISH_ASSERT(document != nullptr);
    STARFISH_ASSERT(msg != nullptr);

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
    STARFISH_ASSERT(wv != nullptr);

    std::string capturePng = rtCreatePngName(g_referenceTestState);
    screenShotInRendering(wv, capturePng.c_str(), [capturePng]() {
        STARFISH_LOG_INFO("STARFISH_RTCAPTURED %s", capturePng.c_str());
    });
}
// WPT Reference Test
static bool rtPixelDiff(WebView* wv)
{
    STARFISH_ASSERT(wv != nullptr);

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
    STARFISH_ASSERT(document != nullptr);

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
            STARFISH_LOG_INFO("STARFISH_RTPASS");
        } else {
            STARFISH_LOG_INFO("STARFISH_RTFAIL");
        }
        exit(0);
    }
}
#endif

#ifndef STARFISH_FILLING_GRAPHICS_BUFFER_TIME_LIMIT
#ifdef STARFISH_FLUTTER
#define STARFISH_FILLING_GRAPHICS_BUFFER_TIME_LIMIT 100
#else
#define STARFISH_FILLING_GRAPHICS_BUFFER_TIME_LIMIT 10
#endif
#endif
size_t WebView::g_fillingGraphicsBufferTileFrameTimeLimitInMS =
    STARFISH_FILLING_GRAPHICS_BUFFER_TIME_LIMIT;

WebView* WebView::create(Starfish* starfish, const char* locale,
                         const char* timezoneID, uint32_t w, uint32_t h,
                         uint32_t defaultFontSize, String* defaultFontName,
                         const ScreenInfo& info, String* customUserAgentString,
                         String* builtinPolyfillPathString)
{
    STARFISH_ASSERT(starfish != nullptr);
    STARFISH_ASSERT(locale != nullptr);
    STARFISH_ASSERT(timezoneID != nullptr);
    STARFISH_ASSERT(defaultFontName != nullptr);
    STARFISH_ASSERT(customUserAgentString != nullptr);
    STARFISH_ASSERT(builtinPolyfillPathString != nullptr);

    return new WebView(starfish, locale, timezoneID, w, h, defaultFontSize,
                       defaultFontName, info, customUserAgentString,
                       builtinPolyfillPathString);
}

WebView::WebView(Starfish* starfish, const char* locale, const char* timezoneID,
                 uint32_t w, uint32_t h, uint32_t defaultFontSize,
                 String* defaultFontName, const ScreenInfo& info,
                 String* customUserAgentString,
                 String* builtinPolyfillPathString)
    : WebBase(starfish, locale, timezoneID, customUserAgentString)
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
    , m_isActive(true)
    , m_inIdleMode(false)
    , m_didFirstRenderingAfterWakeup(true)
    , m_rootStackingContext(nullptr)
#ifdef STARFISH_ENABLE_TTS
    , m_tts(new TTS(this))
#endif
#if defined(STARFISH_TIZEN_TV) && defined(STARFISH_ENABLE_AVPLAY)
    , m_avplay(new Avplay(this))
#endif
#if defined(STARFISH_ENABLE_INSPECTOR)
    , m_inspector(nullptr)
#endif
#if defined(STARFISH_ENABLE_MULTI_THREAD_IMAGE_DECODING)
    , m_imageDecodeThreadPool(nullptr)
#endif
    , m_activeImageURLsInRenderingMutex(new Mutex())
    , m_defaultFontSize(defaultFontSize)
    , m_screenInfo(info)
    , m_builtinPolyfillPathString(builtinPolyfillPathString)
#ifdef STARFISH_ENABLE_TEST
    , m_testCompatibleMode(StarfishTestCompatibleMode::Normal)
#endif
    , m_baseBackgroundColor(Unit::Color(255, 255, 255, 255))
    , m_baseForegroundColor(Unit::Color(0, 0, 0, 255))
    , m_idleModeJob(LWE::IdleModeJob::IdleModeDefault)
    , m_idleModeCheckIntervalInMS(0)
    , m_idleCheckTimerID(TimerInvalidID)
    , m_needsDownloadWebFontsEarly(false)
    , m_needsDownScaleImageResourceLargerThan(0)
    , m_scrollbarVisible(true)
    , m_useExternalPopup(false)
    , m_useSpatialNavigation(false)
{
    STARFISH_ASSERT(starfish != nullptr);
    STARFISH_ASSERT(locale != nullptr);
    STARFISH_ASSERT(timezoneID != nullptr);
    STARFISH_ASSERT(defaultFontName != nullptr);
    STARFISH_ASSERT(customUserAgentString != nullptr);
    STARFISH_ASSERT(builtinPolyfillPathString != nullptr);

    m_platformWindow->setWebView(this);
    m_deviceKind = deviceKindUseTouchScreen;
#ifdef STARFISH_ENABLE_TEST
    m_testCompatibleMode = g_testCompatibleMode;
    m_startUpFlag = g_startUpFlag;
#else
    m_startUpFlag = 0;
#endif

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

    m_starfish->m_webViewInstanceCount++;

#ifndef STARFISH_IMAGE_DECODE_THREAD_THREAD_POOL_SIZE
#define STARFISH_IMAGE_DECODE_THREAD_THREAD_POOL_SIZE 4
#endif
#if defined(STARFISH_ENABLE_MULTI_THREAD_IMAGE_DECODING)
    m_imageDecodeThreadPool = new ThreadPool(
        STARFISH_IMAGE_DECODE_THREAD_THREAD_POOL_SIZE, m_messageLoop);
#endif

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
                STARFISH_ASSERT(data != nullptr);

                WebView* wv = (WebView*)data;
                uint64_t currentTick = longTickCount();
                if (!wv->m_inIdleMode &&
                    currentTick - wv->m_lastRenderingTick >
                        (uint64_t)wv->m_idleModeCheckIntervalInMS * 1000) {
                    wv->enterIdleMode();
                }
            },
            this, true);
    }
}

void WebView::clearDrawnBuffers()
{
    if (m_didCompositeBefore) {
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
    } else {
        m_needsFullPainting = true;
        m_needsPainting = true;
    }

    platformWindow()->onClearDrawnBuffers();
}

void WebView::enterIdleMode()
{
    STARFISH_LOG_INFO("enter idle mode");
    m_inIdleMode = true;
    m_didFirstRenderingAfterWakeup = false;

    onIdle();

    // drop CanvasSurfaces if possible
    if (((int)m_idleModeJob & (int)LWE::IdleModeJob::ClearDrawnBuffers)) {
        clearDrawnBuffers();
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
        auto& globalImages =
            BufferedNativeImageData::everyNativeImageInstances();
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
    STARFISH_ASSERT(exposedObjectName != nullptr);
    STARFISH_ASSERT(jsFunctionName != nullptr);
    STARFISH_ASSERT(scriptObject != nullptr);
    STARFISH_ASSERT(scriptNativeFunctionPointer != nullptr);

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

    m_jsInterfaceList.push_back(std::make_tuple(exposedObjectName,
                                                jsFunctionName, scriptObject,
                                                scriptNativeFunctionPointer));
}

void WebView::removeJavaScriptNativeInterface(String* exposedObjectName,
                                              String* jsFunctionName)
{
    STARFISH_ASSERT(exposedObjectName != nullptr);
    STARFISH_ASSERT(jsFunctionName != nullptr);

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
    STARFISH_ASSERT(instance != nullptr);

    for (auto it = m_jsInterfaceList.begin(); it != m_jsInterfaceList.end();
         it++) {
        registerJavaScriptNativeInterface(instance, std::get<0>(*it),
                                          std::get<1>(*it), std::get<2>(*it),
                                          std::get<3>(*it));
    }
}

void WebView::destroy()
{
    STARFISH_LOG_INFO("WebView::destroy");
#if defined(STARFISH_ENABLE_INSPECTOR)
    delete m_inspector;
    m_inspector = nullptr;
#endif

    pause();

#ifdef STARFISH_ENABLE_TTS
    m_tts->destroy();
#endif

    m_browsingContextsNeedsLayout.clear();
    m_browsingContextsDidLayout.clear();
    m_repaintRegionInRendering.clear();
    m_globalPointingEventListener.clear();
    m_jsInterfaceList.clear();
    m_repaintRegionTrackerContext.clear();

    if (mainBrowsingContext()) {
        mainBrowsingContext()->dispose();
    }

    if (m_rootStackingContext) {
        StackingContext* ctx = m_rootStackingContext;
        std::function<void(StackingContext*)> clearSC =
            [&](StackingContext* ctx) {
                STARFISH_ASSERT(ctx != nullptr);

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

#if defined(STARFISH_ENABLE_MULTI_THREAD_IMAGE_DECODING)
    m_imageDecodeThreadPool->destroy();
#endif

    m_threadPool->destroy();
    m_messageLoop->destroy();

    m_timer->clear(nullptr);
    m_timer->destroy();

    removeScriptEngineInstance();

    std::unordered_set<std::string>().swap(m_activeImageURLsInRendering);
    m_activeImageURLsInRenderingMutex = nullptr;

    m_publicLayerUserDataMap.clear();

    delete m_platformFontCache;
    delete m_platformFontSelector;

    m_platformWindow->destroy();

    m_starfish->m_webViewInstanceCount--;
    this->WebView::~WebView();

    m_platformWindow->clearNativeHandlers();

    clearStack<ELABORATE_CLEAR_STACK_SIZE>();
}

void WebView::ensureScriptEngineInstance()
{
    if (!m_scriptEngineInstance) {
        m_scriptEngineInstance = new ScriptEngineInstance(
            locale().data(), timezoneID()->toUTF8NonGCString().data());

        // Add a global promise hook to call on settled.
        m_scriptEngineInstance->engineInstance()->registerPromiseHook(
            [](ExecutionStateRef* state, VMInstanceRef::PromiseHookType type,
               PromiseObjectRef* promise, ValueRef* parent) {
                // Only promises created in starfish have an extraData.
                if (promise && promise->extraData()) {
                    Promise* p = static_cast<Promise*>(promise->extraData());
                    switch (type) {
                    case VMInstanceRef::PromiseHookType::Resolve:
                        p->onSettled();
                    case VMInstanceRef::PromiseHookType::Init:
                    case VMInstanceRef::PromiseHookType::Before:
                    case VMInstanceRef::PromiseHookType::After:
                        // Note: Implement if you need.
                        break;
                    default:
                        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
                        break;
                    }
                }
            });
    }
}

void WebView::removeScriptEngineInstance()
{
    if (m_scriptEngineInstance) {
        m_scriptEngineInstance->engineInstance()->unregisterPromiseHook();
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
    STARFISH_ASSERT(filePath != nullptr);

    String* resolvedPath = filePath;
    if (!filePath->startsWith("http") && !filePath->startsWith("about") &&
        !filePath->startsWith("data:")) {
        String* prefix = String::fromUTF8("file://");
        Nullable<std::string> result =
            PlatformFileUtil::absolutePath(filePath->toUTF8NonGCString());
        if (result.hasValue()) {
            resolvedPath = prefix->concat(String::fromUTF8(
                result.getValue().data(), result.getValue().length()));
        } else if (!resolvedPath->startsWith("file://", false)) {
            // Will navigate to about:blank
            resolvedPath = prefix->concat(resolvedPath);
        }
    }

    return resolvedPath;
}

void WebView::loadHTMLDocument(String* filePath) // navigate function helper
{
    STARFISH_ASSERT(filePath != nullptr);

    String* resolvedPath = resolvePath(filePath);
    ResourceURL* url = new ResourceURL(resolvedPath);
    ReferrerURL* rUrl = new ReferrerURL(String::emptyString);
    navigate(url, HistoryManagerAction::Add, rUrl);
}

void WebView::navigate(ResourceURL* url, HistoryManagerAction type,
                       ReferrerURL* referrerURL)
{
    STARFISH_ASSERT(url != nullptr);
    STARFISH_ASSERT(referrerURL != nullptr);

    if (type == HistoryManagerAction::Intact &&
        url->getUrlPathString()->equals(referrerURL->getUrlPathString())) {
        navigateSameDocument(url, type, referrerURL);
    } else {
        navigateCrossDocument(url, type, referrerURL);
    }
}

void WebView::navigateCrossDocument(ResourceURL* url, HistoryManagerAction type,
                                    ReferrerURL* referrerURL)
{
    clearBlobURLStore();
    clearMediaSourceBlobURLStore();
    clearActiveImageURLsInRenderingSet();

    m_navigateStartingTime = timestamp();

    m_browsingContextsNeedsLayout.clear();
    m_browsingContextsDidLayout.clear();
    m_repaintRegionInRendering.clear();
    m_globalPointingEventListener.clear();
    m_repaintRegionTrackerContext.clear();

    if (mainBrowsingContext()) {
        mainBrowsingContext()->dispose();
    }

    if (m_rootStackingContext) {
        StackingContext* ctx = m_rootStackingContext;
        std::function<void(StackingContext*)> clearSC =
            [&](StackingContext* ctx) {
                STARFISH_ASSERT(ctx != nullptr);

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

    platformWindow()->hideSoftwareKeyboardIfPossible();
    m_topLevelBrowsingContext = BrowsingContext::create(this);

    removeScriptEngineInstance();

    std::unordered_set<std::string>().swap(m_activeImageURLsInRendering);

    initRenderingFlags();
    clearStack<ELABORATE_CLEAR_STACK_SIZE>();

    ensureScriptEngineInstance();

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

void WebView::navigateSameDocument(ResourceURL* url, HistoryManagerAction type,
                                   ReferrerURL* referrerURL)
{
    Document* document = m_topLevelBrowsingContext->window()->document();
    bool isHashChanged = !referrerURL->hash()->equals(url->hash());

    // Change documentURI.
    document->setDocumentURI(url);

    // Scroll to target.
    String* str = url->hash();
    if (str->length() > 1) {
        Element* e =
            document->getElementById(str->substring(1, str->length() - 1));
        if (e) {
            e->scrollIntoView();
        }
    }

    // Dispatch popstate event.
    if (historyManager()->currentEntry()) {
        auto* serializedState = historyManager()->currentEntry()->state();
        auto* deserializedState = Serializer::deserialize(
            document->executionContext(), serializedState);
        PopStateEventInit init;
        init.setState(deserializedState);
        PopStateEvent* event = new PopStateEvent(
            document->executionContext(),
            starfish()->staticStrings()->m_popstate.localName(), init);

        struct Params {
            Window* window;
            PopStateEvent* event;
        };
        Params* params = new Params();
        params->window = document->window();
        params->event = event;
        messageLoop()->addIdler(
            document->window(),
            [](size_t handle, void* data) {
                Params* params = static_cast<Params*>(data);
                params->window->dispatchEventByUA(params->event);
                delete params;
            },
            params);
    }
}

String* WebView::evaluateJavaScript(String* s)
{
    STARFISH_ASSERT(s != nullptr);

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
    STARFISH_ASSERT(s != nullptr);

    String* ret = String::emptyString;

    if (mainBrowsingContext()) {
        ret = toBrowserString(
            mainBrowsingContext()->scriptBindingInstance(),
            evaluateString(mainBrowsingContext()->scriptBindingInstance(), s));
    }
    cb(ret->toUTF8NonGCString());
}

BlobURLStore WebView::addMediaSourceInBlobURLStore(MediaSource* ptr)
{
    STARFISH_ASSERT(ptr != nullptr);

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

    std::uniform_int_distribution<uint32_t> distribution;

#ifdef STARFISH_32
    a.m_a = distribution(randomEngine());
    a.m_b = distribution(randomEngine());
    a.m_c = distribution(randomEngine());
#else
    a.m_a = distribution(randomEngine());
    a.m_b = distribution(randomEngine());
#endif

    m_urlMediaSourceBlobStore.insert(a);

    return a;
}

void WebView::removeMediaSourceFromBlobURLStore(MediaSource* ptr)
{
    STARFISH_ASSERT(ptr != nullptr);

#ifndef NDEBUG
    STARFISH_LOG_INFO(
        "[TRACE_MSE_GC] WebView::removeMediaSourceFromBlobURLStore");
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
    STARFISH_ASSERT(ptr != nullptr);

    BlobURLStore s;
    s.m_blob = ptr;
    auto iter = m_urlMediaSourceBlobStore.find(s);
    return iter != m_urlMediaSourceBlobStore.end();
}

Nullable<BlobURLStore> WebView::findMediaSourceBlobURL(MediaSource* ptr)
{
    STARFISH_ASSERT(ptr != nullptr);

    BlobURLStore s;
    s.m_blob = ptr;
    auto iter = m_urlMediaSourceBlobStore.find(s);
    if (iter != m_urlMediaSourceBlobStore.end()) {
        return *iter;
    } else {
        return nullptr;
    }
}

void WebView::clearMediaSourceBlobURLStore()
{
    m_urlMediaSourceBlobStore.clear();
    GCUnorderedSet<BlobURLStore>().swap(m_urlMediaSourceBlobStore);
}

void WebView::computeLayoutPaintingDirty()
{
    INSTALL_PROFILE_TIMER("WebView::computeLayoutPaintingDirty");

    auto browsingContextsDidLayout = std::move(m_browsingContextsDidLayout);
    m_browsingContextsDidLayout.clear();
    for (size_t i = 0; i < browsingContextsDidLayout.size(); i++) {
        browsingContextsDidLayout[i]->computeLayoutPaintingDirty();
    }
}

void WebView::layoutIfNeeded(bool shouldCareStackingContextNow)
{
    INSTALL_PROFILE_TIMER("WebView::layoutIfNeeded");
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

    bool didStackingContextJob = false;
    if (shouldCareStackingContextNow) {
        if (!m_rootStackingContext || m_needsEstablishesStackingContext) {
            INSTALL_PROFILE_TIMER("establishesStackingContext");
            didStackingContextJob = true;
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
            didStackingContextJob = true;
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
                            STARFISH_ASSERT(ctx != nullptr);
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
                            } else if (ctx->owner()->node() != nullptr) {
                                printf(
                                    "StackingContext[%d][%p, %s"
                                    ", frame %p, buf %d opacity %f "
                                    "screenExtent %f %f %f %f visibleRect %f "
                                    "%f %f %f]",
                                    depth / 2, ctx,
                                    ctx->owner()
                                        ->node()
                                        ->localName()
                                        ->toUTF8NonGCString()
                                        .data(),
                                    ctx->owner(),
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

    if (didLayout || didStackingContextJob) {
        clearStack<DEFAULT_CLEAR_STACK_SIZE>();
    }
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
    STARFISH_ASSERT(ctx != nullptr);

    ctx->layoutRepaintTracker().clearDatasRelatedWithStackingContext();
    ctx->iterateChildContext(
        [](BrowsingContext* ctx) { cleanupLayoutRepaintTracker(ctx); });
}

static void saveCurrentPaintingState(StackingContext* ctx)
{
    STARFISH_ASSERT(ctx != nullptr);

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
        info.additionalPixelRatio = ctx->additionalPixelRatio();
        info.graphicsBufferHolder = ctx->graphicsBufferHolder();
    } else {
        StackingContext* owner = ctx->parent();
        while (owner != nullptr && !owner->needsGraphicsBuffer()) {
            owner = owner->parent();
        }

        if (owner) {
            info.graphicsLayerOwner = owner->owner()->node();
        }
        info.extentOnGraphicsLayer = computeBoxExtent(
            LayoutRect(0, 0, ctx->owner()->width(), ctx->owner()->height()),
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
        ((timestamp() - mainBrowsingContext()
                            ->document()
                            ->resourceLoader()
                            .documentOpenTime()) < 1000)) {
        STARFISH_LOG_INFO("delay rendering due to pending stylesheet");
        m_needsRendering = false;
        Canvas* canvas = platformWindow()->preparePainting();
        mainBrowsingContext()->clearingBeforePaint(canvas);
        renderResult.didPaintingOrCompositing = true;
        renderResult.updateRect = LayoutRect(0, 0, platformWindow()->width(),
                                             platformWindow()->height());
        delete canvas;
        m_didFirstRenderingAfterWakeup = true;
        return renderResult;
    }

    m_lastRenderingTick = longTickCount();
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
    computeLayoutPaintingDirty();

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
            STARFISH_LOG_INFO("Start composite mode");

        } else if (m_didCompositeBefore && !m_needsComposite) {
            STARFISH_LOG_INFO("End composite mode");
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
            auto oldRepaintRegionTrackerContext =
                std::move(m_repaintRegionTrackerContext);
            {
                INSTALL_PROFILE_TIMER("track repaint region");
                RepaintRegionTracker tracker(
                    oldRepaintRegionTrackerContext,
                    m_repaintRegionTrackerContext,
                    mainBrowsingContext()
                        ->document()
                        ->frame()
                        ->asFrameBlockBox(),
                    needsFullPainting, prevDrawnStackingContextInfo, scrollX,
                    scrollY, m_needsComposite);
                m_repaintRegionInRendering = std::move(tracker.repaintRegion());
            }

            auto repaintRect = m_repaintRegionInRendering[nullptr];
            cleanupLayoutRepaintTracker(mainBrowsingContext());

#if defined(STARFISH_ENABLE_PROFILE_TIMER)
            {
                auto iter = m_repaintRegionInRendering.begin();
                while (iter != m_repaintRegionInRendering.end()) {
                    if (iter->first) {
                        STARFISH_LOG_INFO(
                            "repaint region node %s #%s className(%s) %f %f %f "
                            "%f",
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
                        STARFISH_LOG_INFO("repaint region (screen) %f %f %f %f",
                                          (float)iter->second.x(),
                                          (float)iter->second.y(),
                                          (float)iter->second.width(),
                                          (float)iter->second.height());
                    }
                    iter++;
                }
            }
#endif

            renderResult.computedRepaintRect = repaintRect;

            if (!m_needsComposite) {
                // release every graphics buffer first
                auto iter = prevDrawnStackingContextInfo.begin();
                while (iter != prevDrawnStackingContextInfo.end()) {
                    if (iter->second.graphicsBufferHolder) {
                        iter->second.graphicsBufferHolder->flushSurfaces();
                        iter->second.graphicsBufferHolder = nullptr;
                    }
                    iter++;
                }
            } else {
                INSTALL_PROFILE_TIMER(
                    "remove definitely useless graphics buffer first");
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
                                    ->flushSurfaces();
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
                INSTALL_PROFILE_TIMER("painting job");
                canvas = platformWindow()->preparePainting();
                canvas->save();
                renderResult.updateRect = canvas->pixelSnappedClip(repaintRect);
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
                if (scrollbarVisible()) {
                    mainBrowsingContext()
                        ->window()
                        ->scrolling()
                        ->paintScrollbars(
                            mainBrowsingContext()->window()->scrolling(),
                            canvas, mainFrame, mainFrame->appliedOverflowX(),
                            mainFrame->appliedOverflowY());
                }
                canvas->restore();
                m_didCompositeBefore = false;
            } else {
                INSTALL_PROFILE_TIMER("painting job(composite)");
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

            /*
            STARFISH_LOG_INFO("repaint region(device) %f %f %f %f",
                              (float)renderResult.updateRect.x(),
                              (float)renderResult.updateRect.y(),
                              (float)renderResult.updateRect.width(),
                              (float)renderResult.updateRect.height());
             */
            auto iter = prevDrawnStackingContextInfo.begin();
            while (iter != prevDrawnStackingContextInfo.end()) {
                if (iter->second.graphicsBufferHolder) {
                    iter->second.graphicsBufferHolder->flushSurfaces();
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
        m_didCompositeBefore = true;

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
            if (scrollbarVisible()) {
                mainBrowsingContext()->window()->scrolling()->paintScrollbars(
                    mainBrowsingContext()->window()->scrolling(), compositor,
                    mainFrame, mainFrame->appliedOverflowX(),
                    mainFrame->appliedOverflowY());
            }

#ifdef STARFISH_ENABLE_VIRTUAL_CURSOR
            platformWindow()->paintVirtualCursor(compositor);
#endif
            delete compositor;
        } else {
            Compositor* compositor = platformWindow()->prepareCompositor();
            compositor->clearColor(Unit::Color(0, 0, 0, 0));
            delete compositor;
        }
        m_needsComposite = false;
    }

    // cleanup box-shadow cache
    {
        auto iter = m_boxShadowCachePerRendering.begin();
        while (iter != m_boxShadowCachePerRendering.end()) {
            delete iter->second;
            iter->second = nullptr;
            iter++;
        }
        m_boxShadowCachePerRendering.clear();
    }

    m_needsRendering = false;
    m_inRendering = false;

    size_t totalAllocatedCanvasSurfaceSizeAfter =
        CanvasSurface::g_totalAllocatedCanvasSurfaceSize;

    if (totalAllocatedCanvasSurfaceSizeBefore !=
        totalAllocatedCanvasSurfaceSizeAfter) {
        STARFISH_LOG_INFO("totalAllocatedCanvasSurfaceSize %fMB",
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
                "starting loading",
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
        uint64_t tick = tickCount();
        for (size_t i = 0; i < m_activeAnimationExecutor.size(); i++) {
            auto& transitions =
                m_activeAnimationExecutor[i]->activeTransitions();
            for (auto task : transitions) {
                task->initializeStartTimeIfNeeded(tick);
                needsContinuousRendering = true;
                if (task->targetElement()->isPseudoElement() == true) {
                    // we should give damage on parent element
                    // because style of pseudo element is computed by
                    // its parent element
                    task->targetElement()
                        ->asPseudoElement()
                        ->originElement()
                        ->setNeedsStyleRecalcForAnimation();
                } else {
                    task->targetElement()->setNeedsStyleRecalcForAnimation();
                }
            }

            auto& animations = m_activeAnimationExecutor[i]->activeAnimations();
            for (auto& animation : animations) {
                size_t idx = animation.first->m_index;
                for (auto task : animation.second) {
                    task->initializeStartTimeIfNeeded(tick);
                    if (task->targetElement()->isPseudoElement() == true) {
                        // we should give damage on parent element
                        // because style of pseudo element is computed by
                        // its parent element
                        if (animation.first->m_playState ==
                            AnimationPlayStateValue::
                                AnimationPlayStateRunningValue) {
                            task->targetElement()
                                ->asPseudoElement()
                                ->originElement()
                                ->setNeedsStyleRecalcForAnimation();
                            needsContinuousRendering = true;

                            if (task->playState() ==
                                AnimationPlayStateValue::
                                    AnimationPlayStatePausedValue) {
                                task->setStartTime(tick - task->gapTime());
                                task->setGapTime(0);
                                task->setIsRunning(true);
                                task->setPlayState(
                                    AnimationPlayStateValue::
                                        AnimationPlayStateRunningValue);
                            }
                        } else {
                            if (task->playState() ==
                                AnimationPlayStateValue::
                                    AnimationPlayStateRunningValue) {
                                task->setGapTime(tick - task->startTime());
                                task->setIsRunning(false);
                                task->setPlayState(
                                    AnimationPlayStateValue::
                                        AnimationPlayStatePausedValue);
                            }
                        }
                    } else {
                        if (animation.first->m_playState ==
                            AnimationPlayStateValue::
                                AnimationPlayStateRunningValue) {
                            if (!task->isInForwardsFillMode()) {
                                task->targetElement()
                                    ->setNeedsStyleRecalcForAnimation();
                                needsContinuousRendering = true;
                            }

                            if (task->playState() ==
                                AnimationPlayStateValue::
                                    AnimationPlayStatePausedValue) {
                                task->setStartTime(tick - task->gapTime());
                                task->setGapTime(0);
                                task->setIsRunning(true);
                                task->setPlayState(
                                    AnimationPlayStateValue::
                                        AnimationPlayStateRunningValue);
                            }
                        } else {
                            if (task->playState() ==
                                AnimationPlayStateValue::
                                    AnimationPlayStateRunningValue) {
                                task->setGapTime(tick - task->startTime());
                                task->setIsRunning(false);
                                task->setPlayState(
                                    AnimationPlayStateValue::
                                        AnimationPlayStatePausedValue);
                            }
                        }
                    }
                }
            }
        }
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
    m_didFirstRenderingAfterWakeup = true;
    return renderResult;
}

void WebView::setNeedsFullRepainting()
{
    STARFISH_LOG_INFO("WebView::setNeedsFullRepainting");
    markNeedsPaintingConsiderInRendering();
    m_needsFullPainting = true;
}

void WebView::clearStackingContext()
{
    if (m_rootStackingContext) {
        StackingContext* ctx = m_rootStackingContext;
        std::function<void(StackingContext*)> clearSC =
            [&](StackingContext* ctx) {
                STARFISH_ASSERT(ctx != nullptr);

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
    STARFISH_LOG_INFO("WebView::pause");
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
    STARFISH_LOG_INFO("WebView::resume");
    m_isActive = true;

    if (mainBrowsingContext()) {
        mainBrowsingContext()->resume();
        if (mainBrowsingContext()->document()) {
            setNeedsFullRepainting();
        }
    }
}

void WebView::resize(uint32_t width, uint32_t height)
{
    STARFISH_LOG_INFO("WebView::resize");
    if (mainBrowsingContext()) {
        float dpr = screenInfo().devicePixelRatio;
        LayoutUnit scaledWidth = LayoutUnit(width) / dpr;
        scaledWidth = scaledWidth.ceil();
        LayoutUnit scaledHeight = LayoutUnit(height) / dpr;
        scaledHeight = scaledHeight.ceil();

        m_screenInfo.rect.setWidth(scaledWidth);
        m_screenInfo.rect.setHeight(scaledHeight);
        m_screenInfo.availableRect.setWidth(scaledWidth);
        m_screenInfo.availableRect.setHeight(scaledHeight);

        mainBrowsingContext()->window()->resize(scaledWidth, scaledHeight);
        setNeedsFullRepainting();
    }
}

void WebView::onIdle()
{
    clearActiveImageURLsInRenderingSet();

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
    STARFISH_ASSERT(touches != nullptr);

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
            nd->onGlobalPointingEvent(x, y, touches[0].timeStamp(), newKind);
            if (std::find(m_globalPointingEventListener.begin(),
                          m_globalPointingEventListener.end(),
                          nd) != m_globalPointingEventListener.end()) {
                i++;
            }
        }

        if (kind == TouchEventKind::TouchEventEnd) {
            for (size_t i = 0; i < m_globalPointingEventListener.size(); i++) {
                m_globalPointingEventListener[i]
                    ->executionContext()
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
            nd->onGlobalPointingEvent(x, y, data.timeStamp(), newKind);
            if (std::find(m_globalPointingEventListener.begin(),
                          m_globalPointingEventListener.end(),
                          nd) != m_globalPointingEventListener.end()) {
                i++;
            }
        }

        if (kind == MouseEventKind::MouseEventUp) {
            for (size_t i = 0; i < m_globalPointingEventListener.size(); i++) {
                m_globalPointingEventListener[i]
                    ->executionContext()
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
                                       Nullable<Node*> node)
{
    STARFISH_ASSERT(data != nullptr);

    if (mainBrowsingContext()) {
        mainBrowsingContext()->dispatchCompositionEvent(kind, data, node);
    }
}

void WebView::addGlobalPointingEventInterceptListener(EventTarget* node)
{
    STARFISH_ASSERT(node != nullptr);

    size_t sizeBefore = m_globalPointingEventListener.size();
    if (sizeBefore == 0) {
        MouseData mdata(MouseButtonValue::NoButton,
                        MouseButtonsValue::NoButtonDown,
                        m_lastMouseMovePoint.x(), m_lastMouseMovePoint.y(), 0);
        mdata.setDefaultPrevented();

        node->executionContext()
            ->document()
            ->browsingContext()
            ->dispatchMouseEvent(MouseEventKind::MouseEventUp, mdata);
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
    STARFISH_ASSERT(node != nullptr);

    auto iter = std::find(m_globalPointingEventListener.begin(),
                          m_globalPointingEventListener.end(), node);
    if (iter != m_globalPointingEventListener.end()) {
        m_globalPointingEventListener.erase(iter);
    }
}

void WebView::putURLIntoActiveImageURLsInRenderingSet(const std::string& url)
{
    Locker<Mutex> locker(*m_activeImageURLsInRenderingMutex);
    m_activeImageURLsInRendering.insert(url);
}

bool WebView::isThereURLInActiveImageURLsInRenderingSet(const std::string& url)
{
    Locker<Mutex> locker(*m_activeImageURLsInRenderingMutex);
    return m_activeImageURLsInRendering.find(url) !=
           m_activeImageURLsInRendering.end();
}

void WebView::clearActiveImageURLsInRenderingSet()
{
    Locker<Mutex> locker(*m_activeImageURLsInRenderingMutex);
    std::unordered_set<std::string>().swap(m_activeImageURLsInRendering);
}

bool WebView::areThereMoreThanThreeImageURLsInRenderingSet()
{
    Locker<Mutex> locker(*m_activeImageURLsInRenderingMutex);
    return m_activeImageURLsInRendering.size() > 3;
}

void WebView::accessActiveImageURLsInRenderingSet(
    void (*callback)(const std::string& url, NULLABLE void* data),
    NULLABLE void* data)
{
    STARFISH_ASSERT(callback != nullptr);

    Locker<Mutex> locker(*m_activeImageURLsInRenderingMutex);
    auto iter = m_activeImageURLsInRendering.begin();
    while (iter != m_activeImageURLsInRendering.end()) {
        callback(*iter, data);
        iter++;
    }
}

void WebView::putImageIntoBoxShadowCache(FrameBox* box, size_t idx,
                                         NativeImageData* image)
{
    m_boxShadowCachePerRendering[std::make_pair(box, idx)] = image;
}

Nullable<NativeImageData*> WebView::isThereImageInBoxShadowCache(FrameBox* box,
                                                                 size_t idx)
{
    Nullable<NativeImageData*> data =
        m_boxShadowCachePerRendering[std::make_pair(box, idx)];
    return data;
}

#if defined(STARFISH_ENABLE_INSPECTOR)
void WebView::setupInspector(uint32_t portNumber)
{
    STARFISH_ASSERT(m_inspector == nullptr);
    m_inspector = new Inspector(this);
    m_inspector->run(portNumber);
}
#endif
} // namespace Starfish
