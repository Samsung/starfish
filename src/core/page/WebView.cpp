/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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
#include "core/modules/message_loop/Timer.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/util/URL.h"

#include "core/dom/HTMLDocument.h"
#include "core/dom/HTMLBodyElement.h"
#include "core/dom/HTMLCollection.h"
#include "core/dom/HTMLHtmlElement.h"

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

// #define STARFISH_ENABLE_TIMER

#if defined(STARFISH_ENABLE_TEST)
#if defined(PORT_GRAPHIC_BACKEND_EFL)
#include <Elementary.h>
extern Evas_Object* g_imgBufferForScreehShot;
#elif defined(PORT_GRAPHIC_BACKEND_GENERAL_BUFFER)
#include <cairo.h>
extern unsigned char* g_imgBufferForScreehShot;
#endif
#endif

namespace StarFish {

WebView* WebView::create(StarFish* starFish)
{
    return new WebView(starFish);
}

WebView::WebView(StarFish* starFish)
    : StarFishHoldable(starFish)
    , m_mainBrowsingContext(nullptr)
    , m_storageNamespaceProvider(nullptr)
    , m_localStorageNamespace(nullptr)
    , m_sessionStorageNamespace(nullptr)
    , m_historyManager(HistoryManager::create(this))
    , m_seed((unsigned int)time(NULL))
    , m_inRendering(false)
    , m_needsRendering(false)
    , m_needsPainting(false)
    , m_needsComposite(false)
{
    m_scriptEngineInstance = new ScriptEngineInstance(starFish);
    initRenderingFlags();
    initStorage();
}

void WebView::close()
{
    mainBrowsingContext()->close();
    m_scriptEngineInstance->close();
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

void WebView::navigate(ResourceURL* url, HistoryManager::Action type)
{
    clearBlobURLStore();
    initRenderingFlags();

    m_mainBrowsingContext = BrowsingContext::create(starFish(), this);

    switch (type) {
    case HistoryManager::Action::Add:
        m_historyManager->push(url);
        break;
    case HistoryManager::Action::Replace:
        m_historyManager->replace(url);
        break;
    case HistoryManager::Action::Intact:
    default:
        break;
    }

    m_mainBrowsingContext->navigate(url);
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

String* WebView::blobURLStoreToString(BlobURLStore store, String* origin)
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
}

void WebView::layoutIfNeeds()
{
    bool didLayout = m_mainBrowsingContext->layoutIfNeeds();

    for (size_t i = 0; i < m_browsingContextsNeedsLayout.size(); i++) {
        didLayout =
            m_browsingContextsNeedsLayout[i]->layoutIfNeeds() || didLayout;
    }
    m_browsingContextsNeedsLayout.clear();

    if (didLayout) {
        {
#ifdef STARFISH_ENABLE_TIMER
            ProfilerTimer t("computeStackingContextProperties");
#endif
            m_mainBrowsingContext->document()
                ->frame()
                ->establishesStackingContextIfNeeds();
            if (m_mainBrowsingContext->document()->frame()->firstChild()) {
                m_rootStackingContext = m_mainBrowsingContext->document()
                                            ->frame()
                                            ->firstChild()
                                            ->asFrameBox()
                                            ->stackingContext();
                m_rootStackingContext->computeStackingContextProperties();
            }

            // STARFISH_LOG_INFO("computeStackingContextProperties end composite
            // %d\n", (int)m_rootStackingContext->needsOwnBuffer());
        }
#ifdef STARFISH_ENABLE_TEST
        if (m_starFish->startUpFlag() &
            StarFishStartUpFlag::enableFrameTreeDump) {
            FrameTreeBuilder::dumpFrameTree(m_mainBrowsingContext->document());
        }
#endif
    }
}

void WebView::rendering()
{
    if (!m_needsRendering) {
        return;
    }

    if (mainBrowsingContext()->hasPendingStyleSheet() &&
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
        starFish()->timer()->addTimer(
            0.01, mainBrowsingContext()->window(),
            [](Window* wnd, void* data) {
                wnd->browsingContext()->webView()->setNeedsRendering();
            },
            mainBrowsingContext()->window(), false);

        Canvas* canvas = starFish()->platformWindow()->preparePainting(true);
#ifndef STARFISH_TIZEN
        canvas->clearColor(Unit::Color(255, 255, 255, 255));
#endif
        delete canvas;
        return;
    }

    uint64_t currentTick = tickCount();
    m_lastRenderingTime = currentTick;
    m_inRendering = true;
#ifdef STARFISH_ENABLE_TIMER
    ProfilerTimer renderingTimer("BrowsingContext::rendering");
#endif

    layoutIfNeeds();

    {
        size_t bufSiz = m_backStackingContextBufferUpWhileReCompsite.size();
        for (size_t i = 0; i < bufSiz; i++) {
            m_backStackingContextBufferUpWhileReCompsite[i]
                ->detachNativeBuffer();
        }
        m_backStackingContextBufferUpWhileReCompsite.clear();
    }

    if (m_needsPainting) {
#ifdef STARFISH_ENABLE_TIMER
        ProfilerTimer t("painting");
#endif
        // painting
        Canvas* canvas = starFish()->platformWindow()->preparePainting(true);

        if (mainBrowsingContext()->document()->frame()->firstChild()) {
            m_needsComposite = m_rootStackingContext->needsOwnBuffer();
        } else {
            m_needsComposite = false;
        }

        if (!m_needsComposite) {
            starFish()->platformWindow()->paintWindowBackground(canvas);
        }

        {
            PaintingContext ctx(canvas);
            ctx.m_paintingStage = PaintingStageEnd;
            mainBrowsingContext()->document()->frame()->paint(ctx);
        }
        m_needsPainting = false;

        delete canvas;
#ifdef STARFISH_TIZEN_WEARABLE
        evas_object_raise(eflWindow->m_dummyBox);
#endif

#ifdef STARFISH_ENABLE_TEST
        if (m_starFish->startUpFlag() &
            StarFishStartUpFlag::enableStackingContextDump) {
            if (mainBrowsingContext()->document()->frame()->firstChild()) {
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

                std::function<void(StackingContext*, int)> dumpSC = [&dumpSC](
                    StackingContext* ctx, int depth) {
                    for (int i = 0; i < depth; i++) {
                        printf("  ");
                    }

                    auto fr = ctx->visibleRect();

                    std::string className;
                    for (unsigned i = 0; i < ctx->owner()
                                                 ->node()
                                                 ->asHTMLElement()
                                                 ->classNames()
                                                 .size();
                         i++) {
                        className += ctx->owner()
                                         ->node()
                                         ->asHTMLElement()
                                         ->classNames()[i]
                                         .string()
                                         ->utf8Data();
                        className += " ";
                    }

                    printf(
                        "StackingContext[%p, node %p %s id:%s className:%s "
                        ", frame %p, buf %p %d %d %d %d]\n",
                        ctx, ctx->owner()->node(),
                        ctx->owner()->node()->localName()->utf8Data(),
                        ctx->owner()->node()->asHTMLElement()->id()->utf8Data(),
                        className.data(), ctx->owner(), ctx->buffer(),
                        (int)fr.x(), (int)fr.y(), (int)fr.width(),
                        (int)fr.height());

                    auto iter = ctx->childContexts().begin();
                    while (iter != ctx->childContexts().end()) {
                        int32_t num = iter->first;

                        for (int i = 0; i < depth + 1; i++) {
                            printf("  ");
                        }

                        printf("z-index: %d\n", (int)num);

                        auto iter2 = iter->second->begin();
                        while (iter2 != iter->second->end()) {
                            dumpSC(*iter2, depth + 2);
                            iter2++;
                        }

                        iter++;
                    }
                };

                dumpSC(ctx, 0);
            }
        }
#endif
    }

    if (m_needsComposite) {
#ifdef STARFISH_ENABLE_TIMER
        ProfilerTimer t("composite");
#endif
        if (mainBrowsingContext()->document()->frame()->firstChild() &&
            m_rootStackingContext->needsOwnBuffer()) {
            Canvas* canvas =
                starFish()->platformWindow()->preparePainting(false);
            starFish()->platformWindow()->paintWindowBackground(canvas);
            mainBrowsingContext()
                ->document()
                ->frame()
                ->firstChild()
                ->asFrameBox()
                ->stackingContext()
                ->compositeStackingContext(canvas);

            delete canvas;
#ifdef STARFISH_TIZEN_WEARABLE
            evas_object_raise(eflWindow->m_dummyBox);
#endif
        }
        m_needsComposite = false;
    }

    m_needsRendering = false;
    m_inRendering = false;

#if defined(STARFISH_ENABLE_TEST)
    {
        const char* path = getenv("SCREEN_SHOT");
        if (path && strlen(path) && g_fireOnloadEvent) {
#if defined(PORT_GRAPHIC_BACKEND_EFL)
            evas_object_image_save(g_imgBufferForScreehShot, path, NULL, NULL);

            // int writeImage(char* filename, int width, int height, void
            // *buffer)
            // writeImage(path, width(), height(),
            // evas_object_image_data_get(g_imgBufferForScreehShot,
            // EINA_FALSE));
            if (getenv("EXIT_AFTER_SCREEN_SHOT") &&
                strlen(getenv("EXIT_AFTER_SCREEN_SHOT"))) {
                exit(0);
            }

#elif defined(PORT_GRAPHIC_BACKEND_GENERAL_BUFFER)
            cairo_surface_t* png_buffer;
            png_buffer = cairo_image_surface_create_for_data(
                g_imgBufferForScreehShot, CAIRO_FORMAT_ARGB32,
                starFish()->platformWindow()->width(),
                starFish()->platformWindow()->height(),
                cairo_format_stride_for_width(
                    CAIRO_FORMAT_ARGB32,
                    starFish()->platformWindow()->width()));

            cairo_surface_write_to_png(png_buffer, path);
            cairo_surface_destroy(png_buffer);

            if (getenv("EXIT_AFTER_SCREEN_SHOT") &&
                strlen(getenv("EXIT_AFTER_SCREEN_SHOT"))) {
                exit(0);
            }

#endif
            g_surfaceForScreehShot->detachNativeBuffer();
            g_surfaceForScreehShot = nullptr;
        }
    }
#endif
}

void WebView::clearStackingContext(bool backupBuffer)
{
    if (m_rootStackingContext) {
        StackingContext* ctx = m_rootStackingContext;
        std::function<void(StackingContext*)> clearSC =
            [&](StackingContext* ctx) {
                if (backupBuffer) {
                    if (ctx->needsOwnBuffer() && ctx->buffer()) {
                        m_backStackingContextBufferUpWhileReCompsite.push_back(
                            ctx->buffer());
                    }
                    ctx->owner()->clearStackingContextIfNeeds(false);
                } else {
                    ctx->owner()->clearStackingContextIfNeeds();
                }
                auto iter = ctx->childContexts().begin();
                while (iter != ctx->childContexts().end()) {
                    auto iter2 = iter->second->begin();
                    while (iter2 != iter->second->end()) {
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
}
