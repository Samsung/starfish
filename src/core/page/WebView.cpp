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
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/Compositor.h"
#include "core/modules/message_loop/Timer.h"
#include "core/modules/message_loop/MessageLoop.h"
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

#ifdef STARFISH_ENABLE_TEST
extern bool g_fireOnloadEvent;
extern bool g_forceRendering;
extern StarFish::CanvasSurface* g_surfaceForScreehShot;
#endif

#if defined(PORT_GRAPHIC_BACKEND_EFL_CAIRO) ||      \
    defined(PORT_GRAPHIC_BACKEND_EFL) ||            \
    defined(PORT_GRAPHIC_BACKEND_GENERAL_BUFFER) || \
    defined(PORT_GRAPHIC_BACKEND_ANDROID)
#include <cairo.h>
#endif

namespace StarFish {

WebView* WebView::create(StarFish* starFish)
{
    return new WebView(starFish);
}

WebView::WebView(StarFish* starFish)
    : StarFishHoldable(starFish)
    , m_topLevelBrowsingContext(nullptr)
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
    , m_didCompositeBefore(false)
    , m_rootStackingContext(nullptr)
    , m_activeAnimatorForAnimationExecutor(SIZE_MAX)
{
    m_scriptEngineInstance = new ScriptEngineInstance(starFish);
    m_historyManager = HistoryManager::create(this);
    initRenderingFlags();
    initStorage();
}

void WebView::close()
{
    mainBrowsingContext()->dispose();
    m_scriptEngineInstance->dispose();
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

void WebView::navigate(ResourceURL* url, HistoryManager::Action type,
                       ResourceURL* referrerURL)
{
    clearBlobURLStore();
    initRenderingFlags();
    clearStack<102400>();
    m_navigateStartingTime = timestamp();
    if (m_topLevelBrowsingContext) {
        m_topLevelBrowsingContext->dispose();
    }
    m_topLevelBrowsingContext = BrowsingContext::create(starFish(), this);
    m_topLevelBrowsingContext->open(url, type, referrerURL);
    starFish()->callWebViewHandler(std::string("OnPageStarted"),
                                   url->urlString());
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

String* WebView::blobURLStoreToString(BlobURLStore store, String* origin)
{
    UTF8StringDataNonGCStd url = "blob:";
    url += origin->toUTF8NonGCString();
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

void WebView::layoutIfNeeds()
{
    INSTALL_PROFILE_TIMER(starFish(), "WebView::rendering::layoutIfNeeds");
    bool didLayout = false;

    while (true) {
        didLayout = didLayout | m_topLevelBrowsingContext->layoutIfNeeds();
        for (size_t i = 0; i < m_browsingContextsNeedsLayout.size(); i++) {
            didLayout =
                didLayout | m_browsingContextsNeedsLayout[i]->layoutIfNeeds();
        }
        m_browsingContextsNeedsLayout.clear();

        if (inRendering()) {
            for (size_t i = 0; i < m_browsingContextsHasPendingAnimation.size();
                 i++) {
                m_browsingContextsHasPendingAnimation[i]
                    ->document()
                    ->animationExecutor()
                    ->runPendingAnimation();
            }
            m_browsingContextsHasPendingAnimation.clear();
        }

        bool needsReLayout = false;
        for (size_t i = 0; i < m_didLayoutCallbacks.size(); i++) {
            if (m_didLayoutCallbacks[i].first(m_didLayoutCallbacks[i].second)) {
                for (size_t j = 0; j <= i; j++) {
                    m_didLayoutCallbacks.erase(m_didLayoutCallbacks.begin());
                }
                needsReLayout = true;
                break;
            }
        }

        if (needsReLayout) {
            continue;
        }

        m_didLayoutCallbacks.clear();

        if (didLayout) {
            m_needsEstablishesStackingContext = true;
        }
        break;
    }

    if (didLayout || !m_rootStackingContext ||
        m_needsEstablishesStackingContext) {
        clearStackingContext(true);
#ifdef STARFISH_ENABLE_TEST
        if (m_starFish->startUpFlag() &
            StarFishStartUpFlag::enableComputedStyleDump) {
            // dump style
            m_topLevelBrowsingContext->document()->styleResolver().dumpDOMStyle(
                m_topLevelBrowsingContext->document());
        }
        if (m_starFish->startUpFlag() &
            StarFishStartUpFlag::enableFrameTreeDump) {
            FrameTreeBuilder::dumpFrameTree(
                m_topLevelBrowsingContext->document(), 0);
        }
#endif

        m_topLevelBrowsingContext->document()
            ->frame()
            ->establishesStackingContextIfNeeds();
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
            if (m_topLevelBrowsingContext->document()->frame()->firstChild() &&
                m_rootStackingContext) {
                m_rootStackingContext->computeStackingContextProperties();
            }
            m_needsComputeStackingContextProperties = false;
        }

#ifdef STARFISH_ENABLE_TEST
        if (m_starFish->startUpFlag() &
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

                        if (ctx->needsGraphicsBuffer()) {
                            totalSurfaceBufferSize +=
                                (int)(ctx->visibleRect().width() *
                                      ctx->visibleRect().height() * 4);
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
                                "StackingContext[%p, node %p %s id:%s "
                                "className:%s"
                                ", frame %p, buf? %d painting %d opacity %f "
                                "visibleRect "
                                "%d "
                                "%d %d %d]",
                                ctx, element, utf8DataLog1.data(),
                                utf8DataLog2.data(), className.data(),
                                ctx->owner(), (int)ctx->needsGraphicsBuffer(),
                                (int)ctx->needsRepainting(),
                                ctx->owner()->style()->opacity(), (int)fr.x(),
                                (int)fr.y(), (int)fr.width(), (int)fr.height());
                        } else {
                            printf(
                                "StackingContext[%p, anonymous node"
                                ", frame %p, buf %d painting %d opacity %f "
                                "visibleRect %d "
                                "%d %d %d]",
                                ctx, ctx->owner(),
                                (int)ctx->needsGraphicsBuffer(),
                                (int)ctx->needsRepainting(),
                                ctx->owner()->style()->opacity(), (int)fr.x(),
                                (int)fr.y(), (int)fr.width(), (int)fr.height());
                        }

                        SkMatrix m = ctx->transformMatrix();
                        printf("matrix [%f %f %f][%f %f %f][%f %f %f]",
                               m.getScaleX(), m.getSkewX(), m.getTranslateX(),
                               m.getSkewY(), m.getScaleY(), m.getTranslateY(),
                               m.getPerspX(), m.getPerspY(), m.get(8));
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

    clearStack<102400>();
}

void WebView::addDidLayoutCallback(DidLayoutCallback cb, void* data)
{
    m_didLayoutCallbacks.push_back(std::make_pair(cb, data));
}

bool WebView::rendering(bool force)
{
    if (!m_needsRendering) {
        return false;
    }

    bool didPaintingOrCompositing = false;

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
        Canvas* canvas = starFish()->platformWindow()->preparePainting();
        mainBrowsingContext()->clearingBeforePaint(canvas);
        delete canvas;
        return false;
    }

    uint64_t currentTick = tickCount();
    m_lastRenderingTime = currentTick;
    m_inRendering = true;
    INSTALL_PROFILE_TIMER(starFish(), "WebView::rendering");

    layoutIfNeeds();

    if (m_needsPainting) {
        INSTALL_PROFILE_TIMER(starFish(), "painting");

        didPaintingOrCompositing = true;
        // painting
        Canvas* canvas = nullptr;

        if (m_rootStackingContext &&
            mainBrowsingContext()->document()->frame()->firstChild()) {
            m_needsComposite = m_rootStackingContext->needsGraphicsBuffer();
        } else {
            m_needsComposite = false;
        }

        {
            FrameBlockBox* mainFrame =
                mainBrowsingContext()->document()->frame()->asFrameBlockBox();
            if (!m_needsComposite) {
                canvas = starFish()->platformWindow()->preparePainting();
                canvas->save();
                canvas->translate(-mainFrame->scrollLeft(),
                                  -mainFrame->scrollTop());
                mainBrowsingContext()->paintWindowBackground(canvas);

                if (mainFrame->firstChild() && m_rootStackingContext) {
                    canvas->save();
                    canvas->translate(
                        mainFrame->firstChild()->asFrameBox()->x(),
                        mainFrame->firstChild()->asFrameBox()->y());
                    m_rootStackingContext->paintStackingContext(canvas, true);
                    canvas->restore();
                }
                canvas->restore();
                m_didCompositeBefore = false;
            } else {
                STARFISH_ASSERT(
                    m_rootStackingContext ==
                    mainFrame->firstChild()->asFrameBox()->stackingContext());
                m_rootStackingContext->paintStackingContext(nullptr, false);
            }
        }

        if (!m_needsComposite) {
            FrameBlockBox* mainFrame =
                mainBrowsingContext()->document()->frame()->asFrameBlockBox();
            mainBrowsingContext()->window()->scrolling()->paintScrollbars(
                canvas, mainFrame, mainFrame->appliedOverflowX(),
                mainFrame->appliedOverflowY());
        }

        m_needsPainting = false;
#ifdef STARFISH_ENABLE_VIRTUAL_CURSOR
        if (!m_needsComposite) {
            starFish()->platformWindow()->paintVirtualCursor(canvas);
        }
#endif

        delete canvas;
        clearStack<102400>();
    }

    {
        size_t bufSiz = m_backStackingContextBufferUpWhileReCompsite.size();
        for (size_t i = 0; i < bufSiz; i++) {
            // printf("drop canvas surface %d %d\n",
            // (int)m_backStackingContextBufferUpWhileReCompsite[i]->bufferWidth(),
            //         (int)m_backStackingContextBufferUpWhileReCompsite[i]->bufferHeight());
            m_backStackingContextBufferUpWhileReCompsite[i]
                ->detachNativeBuffer();
        }
        m_backStackingContextBufferUpWhileReCompsite.clear();
    }

    if (m_needsComposite) {
        INSTALL_PROFILE_TIMER(starFish(), "composite");
        didPaintingOrCompositing = true;

        if (mainBrowsingContext()->document()->frame()->firstChild() &&
            m_rootStackingContext->needsGraphicsBuffer()) {
            Compositor* compositor =
                starFish()->platformWindow()->prepareCompositor();
            FrameBlockBox* mainFrame =
                mainBrowsingContext()->document()->frame()->asFrameBlockBox();

            float devicePixelRatio = starFish()->screenInfo().deviceScaleFactor;
            compositor->translate(starFish()->posX() / devicePixelRatio,
                                  starFish()->posY() / devicePixelRatio);
            compositor->clip(Unit::Rect(
                0, 0, starFish()->platformWindow()->width() / devicePixelRatio,
                starFish()->platformWindow()->height() / devicePixelRatio));

            compositor->save();
            compositor->translate(-mainFrame->scrollLeft(),
                                  -mainFrame->scrollTop());
            Canvas* canvas = Compositor::createCanvasAdaptor(compositor);
            mainBrowsingContext()->paintWindowBackground(canvas);

            m_rootStackingContext->compositeStackingContext(compositor);

            compositor->restore();
            mainBrowsingContext()->window()->scrolling()->paintScrollbars(
                canvas, mainFrame, mainFrame->appliedOverflowX(),
                mainFrame->appliedOverflowY());

            m_didCompositeBefore = true;
#ifdef STARFISH_ENABLE_VIRTUAL_CURSOR
            starFish()->platformWindow()->paintVirtualCursor(canvas);
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
            starFish()->TestCompatibleMode() ==
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

        const char* path = getenv("SCREEN_SHOT");
        if (path && strlen(path) && g_fireOnloadEvent) {
            cairo_surface_t* png_buffer;
            png_buffer = cairo_image_surface_create_for_data(
                (unsigned char*)g_surfaceForScreehShot->data(),
                CAIRO_FORMAT_ARGB32, starFish()->platformWindow()->width(),
                starFish()->platformWindow()->height(),
                g_surfaceForScreehShot->bufferStride());

#if !defined(STARFISH_ANDROID)
            cairo_surface_write_to_png(png_buffer, path);
#endif
            cairo_surface_destroy(png_buffer);

            if (g_surfaceForScreehShot) {
                g_surfaceForScreehShot->detachNativeBuffer();
                starFish()->removePointerFromRootSet(g_surfaceForScreehShot);
            }
            g_surfaceForScreehShot = nullptr;

            if (getenv("EXIT_AFTER_SCREEN_SHOT") &&
                strlen(getenv("EXIT_AFTER_SCREEN_SHOT"))) {
                exit(0);
            }
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

    return didPaintingOrCompositing;
}

void WebView::clearStackingContext(bool backupBuffer)
{
    if (m_rootStackingContext) {
        StackingContext* ctx = m_rootStackingContext;
        std::function<void(StackingContext*)> clearSC =
            [&](StackingContext* ctx) {
                auto lp = ctx->owner()->layoutParent();
                if (lp) {
                    lp->updatePaintingFlags(ctx->owner());
                }
                ctx->owner()->clearStackingContextIfNeeds(!backupBuffer);
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
    m_lastRenderingTime = 0;
    m_inRendering = false;
    m_needsRendering = false;
    m_needsPainting = false;
    m_needsComposite = false;
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

void WebView::onIdle()
{
    if (m_topLevelBrowsingContext) {
        m_topLevelBrowsingContext->onIdle();
    }
}

void WebView::assignGraphicsBuffer(CanvasSurface** surfaceHolder,
                                   size_t visibleWidth, size_t visibleHeight)
{
    STARFISH_ASSERT(m_inRendering);
    if (!*surfaceHolder || ((*surfaceHolder)->width() != visibleWidth) ||
        ((*surfaceHolder)->height() != visibleHeight)) {
        if (*surfaceHolder) {
            m_backStackingContextBufferUpWhileReCompsite.push_back(
                *surfaceHolder);
        }

        *surfaceHolder = nullptr;

        for (size_t i = 0;
             i < m_backStackingContextBufferUpWhileReCompsite.size(); i++) {
            size_t savedW =
                m_backStackingContextBufferUpWhileReCompsite[i]->width();
            size_t savedH =
                m_backStackingContextBufferUpWhileReCompsite[i]->height();
            if (savedW == visibleWidth && savedH == visibleHeight) {
                (*surfaceHolder) =
                    m_backStackingContextBufferUpWhileReCompsite[i];
                m_backStackingContextBufferUpWhileReCompsite.erase(i);
                return;
            }
        }

        // find best nearset buffer
        size_t bestFitScore = SIZE_MAX;
        size_t bestFitIdx = SIZE_MAX;

        for (size_t i = 0;
             i < m_backStackingContextBufferUpWhileReCompsite.size(); i++) {
            size_t savedW =
                m_backStackingContextBufferUpWhileReCompsite[i]->bufferWidth();
            size_t savedH =
                m_backStackingContextBufferUpWhileReCompsite[i]->bufferHeight();

            if (savedW >= visibleWidth && savedH >= visibleHeight) {
                size_t areaA = savedW * savedH;
                size_t areaB = visibleWidth * visibleHeight;
                size_t score = areaA - areaB;

                if (score < (areaA * 0.1)) {
                    if (score < bestFitScore) {
                        bestFitScore = score;
                        bestFitIdx = i;
                    }
                }
            }
        }

        if (bestFitIdx != SIZE_MAX) {
            (*surfaceHolder) =
                m_backStackingContextBufferUpWhileReCompsite[bestFitIdx];
            m_backStackingContextBufferUpWhileReCompsite.erase(bestFitIdx);
            STARFISH_LOG_INFO(
                "WebView::assignGraphicsBuffer - reuse canvas surface %d %d -> "
                "%d %d\n",
                (int)visibleWidth, (int)visibleHeight,
                (int)(*surfaceHolder)->bufferWidth(),
                (int)(*surfaceHolder)->bufferHeight());
            (*surfaceHolder)->resize(visibleWidth, visibleHeight);
        }

        if (*surfaceHolder == nullptr) {
            STARFISH_LOG_INFO(
                "WebView::assignGraphicsBuffer - create canvas surface %d %d\n",
                (int)visibleWidth, (int)visibleHeight);
            INSTALL_PROFILE_TIMER(
                starFish(),
                "WebView::assignGraphicsBuffer - create canvas surface");
            (*surfaceHolder) = CanvasSurface::create(
                starFish()->platformWindow(), visibleWidth, visibleHeight);
        }
    }
}
}
