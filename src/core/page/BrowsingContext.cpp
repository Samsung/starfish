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
#include "StarFish.h"

#include "BrowsingContext.h"

#include "binding/ScriptBindingInstance.h"
#include "core/dom/Document.h"
#include "core/dom/FocusEvent.h"
#include "core/dom/HTMLDocument.h"
#include "core/dom/HTMLBodyElement.h"
#include "core/dom/HTMLCollection.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/dom/EventTarget.h"
#include "core/dom/MouseEvent.h"
#include "core/dom/KeyboardEvent.h"
#include "core/dom/TouchEvent.h"
#include "core/page/Location.h"
#include "core/page/Window.h"
#include "core/style/CSSStyleSheet.h"
#include "core/layout/Frame.h"
#include "core/layout/FrameBlockBox.h"
#include "core/layout/FrameTreeBuilder.h"
#include "core/layout/StackingContext.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/message_loop/Timer.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/util/URL.h"
#include "platform/window/PlatformWindow.h"
#include "core/animation/Animation.h"

#if defined(STARFISH_ENABLE_TEST)
#if defined(PORT_GRAPHIC_BACKEND_EFL)
#include <Elementary.h>
extern Evas_Object* g_imgBufferForScreehShot;
#elif defined(PORT_GRAPHIC_BACKEND_DALI)
#include <cairo.h>
#include <dali-toolkit/dali-toolkit.h>
extern unsigned char* g_imgBufferForScreehShot;
#endif
#endif

#ifdef STARFISH_ENABLE_TEST
extern bool g_fireOnloadEvent;
extern bool g_forceRendering;
extern StarFish::CanvasSurface* g_surfaceForScreehShot;
#endif

// #define STARFISH_ENABLE_TIMER

namespace StarFish {

BrowsingContext* BrowsingContext::create(StarFish* starFish, WebView* webView)
{
    STARFISH_ASSERT(webView);
    return new BrowsingContext(starFish, webView);
}

BrowsingContext::BrowsingContext(StarFish* starFish, WebView* webView)
    : StarFishHoldable(starFish)
    , m_webView(webView)
    , m_document(nullptr)
#if defined(STARFISH_TIZEN_TV) && defined(STARFISH_ENABLE_AVPLAY)
    , m_webapis(nullptr)
#endif
    , m_rootStackingContext(nullptr)
    , m_touchDownPoint(0, 0)
    , m_ctrlKeyDown(0)
    , m_shiftKeyDown(0)
    , m_altKeyDown(0)
    , m_metaKeyDown(0)
{
    m_parentBrowsingContext = nullptr;
    initFlags();
}

void BrowsingContext::initFlags()
{
    m_needsRendering = false;
    m_inRendering = false;
    m_needsStyleRecalc = false;
    m_needsStyleRecalcForWholeDocument = false;
    m_needsFrameTreeBuild = false;
    m_needsLayout = false;
    m_needsPainting = false;
    m_needsComposite = false;

    m_hasRootElementBackground = false;
    m_hasBodyElementBackground = false;
    m_isRunning = true;
    m_pendingStyleSheetCount = 0;
    m_lastRenderingTime = 0;
}

void BrowsingContext::navigate(ResourceURL* url)
{
    close();
    initFlags();

    m_isActive = true;

    StarFishEnterer enter(m_starFish);

    // TODO: Use location to open a new document
    Window* window = Window::create(m_starFish, this, url);
    m_document = window->document();
    m_document->open();
}

void BrowsingContext::navigateAsync(ResourceURL* url)
{
    starFish()->messageLoop()->addIdlerWithNoScriptInstanceEntering(
        this,
        [](size_t a, void* data, void* data2) {
            ((BrowsingContext*)data)->navigate((ResourceURL*)data2);
        },
        this, url);
}

ScriptBindingInstance* BrowsingContext::scriptBindingInstance()
{
    if (document()) {
        STARFISH_ASSERT(document()->window());
        return document()->window()->scriptBindingInstance();
    } else {
        return nullptr;
    }
}

void BrowsingContext::layoutIfNeeds()
{
    if (m_needsStyleRecalc || m_needsStyleRecalcForWholeDocument) {
        if (m_needsStyleRecalcForWholeDocument) {
#ifdef STARFISH_ENABLE_TIMER
            ProfilerTimer t("parse sheet & collect rules");
#endif
            CSSStyleSheet* uaSheet = document()->styleResolver().sheets()[0];
            document()->styleResolver().removeAllRules();

            size_t sheets = document()->styleResolver().sheets().size();
            for (size_t i = 1; i < sheets; i++) {
                CSSStyleSheet* authorSheet =
                    document()->styleResolver().sheets()[i];

                authorSheet->parseSheetIfneeds();

                if (authorSheet->ownerRule()) {
                    authorSheet->collectRulesForImportedSheet();
                } else {
                    authorSheet->collectStyleRules(authorSheet->allRules(),
                                                   authorSheet->url());
                }

                size_t rules = authorSheet->rules().size();
                for (size_t j = 0; j < rules; j++) {
                    document()
                        ->styleResolver()
                        .styleSheetWithStyleRules()
                        ->addStyleRule(authorSheet->rules()[j]);
                }
            }
            document()
                ->styleResolver()
                .styleSheetWithStyleRules()
                ->sortStyleRulesBySpecificity();
        }

// resolve style
#ifdef STARFISH_ENABLE_TIMER
        ProfilerTimer t("resolve style");
#endif
        document()->styleResolver().resolveDOMStyle(
            document(), m_needsStyleRecalcForWholeDocument);
        m_needsStyleRecalc = false;
        m_needsStyleRecalcForWholeDocument = false;

#ifdef STARFISH_ENABLE_TEST
        if (m_starFish->startUpFlag() &
            StarFishStartUpFlag::enableComputedStyleDump) {
            // dump style
            document()->styleResolver().dumpDOMStyle(document());
        }
#endif
    }

    if (m_needsFrameTreeBuild) {
        if (document()->frame()) {
            clearStackingContext(true);

// create frame tree
#ifdef STARFISH_ENABLE_TIMER
            ProfilerTimer t("create frame tree");
#endif
            FrameTreeBuilder::buildFrameTree(document());
            m_needsFrameTreeBuild = false;
        }
    }

    if (m_needsLayout) {
// lay out frame tree
#ifdef STARFISH_ENABLE_TIMER
        ProfilerTimer t("lay out frame tree");
#endif
        clearStackingContext(true);

        LayoutContext ctx(starFish(), document()
                                          ->frame()
                                          ->asFrameBox()
                                          ->asFrameBlockBox()
                                          ->asFrameDocument());
        document()->frame()->layout(ctx,
                                    Frame::LayoutWantToResolve::ResolveAll);

#ifndef NDEBUG
        {
            LayoutContext ctx(starFish(), document()
                                              ->frame()
                                              ->asFrameBox()
                                              ->asFrameBlockBox()
                                              ->asFrameDocument());
            document()->frame()->layout(ctx,
                                        Frame::LayoutWantToResolve::ResolveAll);
        }
#endif
        {
#ifdef STARFISH_ENABLE_TIMER
            ProfilerTimer t("computeStackingContextProperties");
#endif
            document()->frame()->establishesStackingContextIfNeeds();
            if (document()->frame()->firstChild()) {
                m_rootStackingContext = document()
                                            ->frame()
                                            ->firstChild()
                                            ->asFrameBox()
                                            ->stackingContext();
                m_rootStackingContext->computeStackingContextProperties();
            }

            // STARFISH_LOG_INFO("computeStackingContextProperties end composite
            // %d\n", (int)m_rootStackingContext->needsOwnBuffer());
        }
        m_needsLayout = false;
#ifdef STARFISH_ENABLE_TEST
        if (m_starFish->startUpFlag() &
            StarFishStartUpFlag::enableFrameTreeDump) {
            FrameTreeBuilder::dumpFrameTree(document());
        }
#endif
    }
}

void BrowsingContext::rendering()
{
    if (m_pendingStyleSheetCount && document() &&
        document()->resourceLoader().isDocumentInOpenState() &&
        ((timestamp() - document()->resourceLoader().documentOpenTime()) <
         1000)) {
        m_needsRendering = false;
        document()->window()->setTimeout(
            [](Window* wnd, void* data) {
                wnd->browsingContext()->setNeedsRendering();
            },
            100, nullptr);

        Canvas* canvas = starFish()->platformWindow()->preparePainting(true);
#ifndef STARFISH_TIZEN
        canvas->clearColor(Unit::Color(255, 255, 255, 255));
#endif
        return;
    }

    if (!m_needsRendering) {
        return;
    }

    uint64_t currentTick = tickCount();
    m_lastRenderingTime = currentTick;
    m_inRendering = true;
    STARFISH_RELEASE_ASSERT(m_isActive);
#ifdef STARFISH_ENABLE_TIMER
    Timer renderingTimer("BrowsingContext::rendering");
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
        Timer t("painting");
#endif
        // painting
        Canvas* canvas = starFish()->platformWindow()->preparePainting(true);

        if (document()->frame()->firstChild()) {
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
            document()->frame()->paint(ctx);
        }
        m_needsPainting = false;

        delete canvas;
#ifdef STARFISH_TIZEN_WEARABLE
        evas_object_raise(eflWindow->m_dummyBox);
#endif

#ifdef STARFISH_ENABLE_TEST
        if (m_starFish->startUpFlag() &
            StarFishStartUpFlag::enableStackingContextDump) {
            if (document()->frame()->firstChild()) {
                STARFISH_ASSERT(document()
                                    ->frame()
                                    ->firstChild()
                                    ->asFrameBox()
                                    ->isRootElement());
                StackingContext* ctx = document()
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
        Timer t("composite");
#endif
        if (document()->frame()->firstChild() &&
            m_rootStackingContext->needsOwnBuffer()) {
            Canvas* canvas =
                starFish()->platformWindow()->preparePainting(false);
            starFish()->platformWindow()->paintWindowBackground(canvas);
            document()
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

#elif defined(PORT_GRAPHIC_BACKEND_DALI)
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
                Dali::Application* app =
                    (Dali::Application*)starFish()->nativeHandle();
                if (app) {
                    app->Quit();
                } else {
                    exit(0);
                }
            }

#endif
            g_surfaceForScreehShot->detachNativeBuffer();
            g_surfaceForScreehShot = nullptr;
        }
    }
#endif
}

void BrowsingContext::paintWindowBackground(Canvas* canvas)
{
#ifdef STARFISH_TIZEN
    if (!document()->tizenWidgetTransparentBackground()) {
        canvas->clearColor(Unit::Color(255, 255, 255, 255));
    }
#else
    canvas->clearColor(Unit::Color(255, 255, 255, 255));
#endif

    if (!document()->rootElement()) {
        return;
    }

    if (m_hasRootElementBackground || m_hasBodyElementBackground) {
        LayoutRect colorRect(0, 0, document()->window()->width(),
                             document()->window()->height());
        if (m_hasRootElementBackground) {
            FrameBox* rootRect =
                document()->rootElement()->frame()->asFrameBox();
            LayoutLocation rootRectPos =
                rootRect->absolutePoint(document()->frame()->asFrameBox());
            LayoutRect imgRect(rootRectPos.x() + rootRect->borderLeft(),
                               rootRectPos.y() + rootRect->borderTop(),
                               rootRect->width() - rootRect->borderWidth(),
                               rootRect->height() - rootRect->borderHeight());

            FrameBox::paintBackground(canvas,
                                      document()->rootElement()->style(),
                                      imgRect, colorRect, true);
        } else {
            LayoutRect imgRect(0, 0, document()->window()->width(),
                               document()->window()->height());
            if (document()->rootElement()->body()->frame()) {
                FrameBox* bodyRect =
                    document()->rootElement()->body()->frame()->asFrameBox();
                imgRect.setHeight(bodyRect->height() +
                                  bodyRect->marginHeight());
            }

            FrameBox::paintBackground(
                canvas, document()->rootElement()->body()->style(), imgRect,
                colorRect, true);
        }
    }
}

void BrowsingContext::markHasPendingStyleSheet()
{
    STARFISH_LOG_INFO("Window::markHasPendingStyleSheet\n");
    m_pendingStyleSheetCount++;
}

void BrowsingContext::unmarkHasPendingStyleSheet()
{
    STARFISH_LOG_INFO("Window::unmarkHasPendingStyleSheet\n");
    if (m_pendingStyleSheetCount > 0) {
        m_pendingStyleSheetCount--;
        setNeedsRendering();
    }
}

void BrowsingContext::clearStackingContext(bool backupBuffer)
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

void BrowsingContext::close()
{
    m_focusedNode = nullptr;
    m_relatedTarget = nullptr;

    m_activeNodes.clear();
    m_activeNodes.shrink_to_fit();
    m_hoveredNodes.clear();
    m_hoveredNodes.shrink_to_fit();

    if (document()) {
        StarFishEnterer enter(m_starFish);
        document()->window()->close();
        document()->close();

        if (scriptBindingInstance()) {
            {
                StarFishEnterer enter(m_starFish);
                scriptBindingInstance()->close();
            }
            document()->window()->deleteScriptBindingInstance();
        }

        if (document()->animationExecutor()->isAlive()) {
            document()->animationExecutor()->stopIfNeeds();
        }

        m_document = nullptr;
    }

    m_isActive = false;

    m_starFish->timer()->clear(this);
    m_starFish->platformWindow()->clearResources();
    m_starFish->messageLoop()->clearPendingIdlers(this);

    if (m_parentBrowsingContext == nullptr)
        m_starFish->clearBlobURLStore();
}

void BrowsingContext::setWholeDocumentNeedsStyleRecalc()
{
    m_needsStyleRecalcForWholeDocument = true;
    setNeedsRendering();
}

Node* BrowsingContext::hitTest(float x, float y)
{
    layoutIfNeeds();

    if (document() && document()->frame()) {
        Frame* frame = document()->frame()->hitTest(x, y, HitTestStageEnd);
        if (!frame) {
            return nullptr;
        }

        while (frame->isAnonymous()) {
            frame = frame->parent();
        }
#ifdef STARFISH_ENABLE_TEST
        if (m_starFish->startUpFlag() &
            StarFishStartUpFlag::enableHitTestDump) {
            printf("hitTest Result-> ");
            frame->node()->dump();
            puts("");
        }
#endif
        return frame->node();
    }

    return nullptr;
}

void BrowsingContext::setFocusedNode(Node* n)
{
    Node* m = n;
    while (!(m->isElement() && m->asElement()->isFocusable()) &&
           !m->isDocument()) {
        m = m->parentNode();
    }

    if (!m || m->isDocument()) {
        if (!document()->body()) {
            return;
        }
        m = document()->body()->asNode();
    }

    if (m_focusedNode == m) {
        return;
    }

    m->setState(Node::NodeStateFocused, Node::ChildrenOrSiblingsAffectedByFocus,
                true);

    Node* t = m_focusedNode;
    String* eventType;
    Event* e;
    if (t) {
        if (t->isHTMLElement()) {
            eventType = starFish()->staticStrings()->m_blur.localName();
            e = new FocusEvent(document(), eventType, FocusEventInit());
            document()->dispatchEvent(t->asNode(), e);
        }
        if (t->isHTMLElement() && !t->isHTMLBodyElement()) {
            eventType = starFish()->staticStrings()->m_focusout.localName();
            e = new FocusEvent(document(), eventType, FocusEventInit(true));
            document()->dispatchEvent(t->asNode(), e);
        }
    }
    m_relatedTarget = t;
    releaseFocusedNode();

    t = m;
    if (t) {
        if (t->isHTMLElement()) {
            eventType = starFish()->staticStrings()->m_focus.localName();
            e = new FocusEvent(document(), eventType, FocusEventInit());
            document()->dispatchEvent(t->asNode(), e);
        }
        if (t->isHTMLElement() && !t->isHTMLBodyElement()) {
            eventType = starFish()->staticStrings()->m_focusin.localName();
            e = new FocusEvent(document(), eventType, FocusEventInit(true));
            document()->dispatchEvent(t->asNode(), e);
        }
    }
    m_focusedNode = t;
}

void BrowsingContext::releaseFocusedNode()
{
    if (m_relatedTarget) {
        m_relatedTarget->setState(Node::NodeStateFocused,
                                  Node::ChildrenOrSiblingsAffectedByFocus,
                                  false);
    }
}

void BrowsingContext::setActiveNode(Node* n)
{
    Node* t = n->nearestParentElement();
    while (t) {
        t->setState(Node::NodeStateActive,
                    Node::ChildrenOrSiblingsAffectedByActive, true);
        m_activeNodes.push_back(t);
        t = t->parentNode();
    }
}

void BrowsingContext::releaseActiveNode()
{
    if (m_activeNodes.size() == 0) {
        return;
    }

    for (size_t i = 0; i < m_activeNodes.size(); i++) {
        m_activeNodes[i]->setState(Node::NodeStateActive,
                                   Node::ChildrenOrSiblingsAffectedByActive,
                                   false);
    }
    m_activeNodes.clear();
    m_activeNodes.shrink_to_fit();
}

void BrowsingContext::setHoveredNode(Node* n)
{
    Node* t = n->nearestParentElement();
    while (t) {
        t->setState(Node::NodeStateHovered,
                    Node::ChildrenOrSiblingsAffectedByHover, true);
        m_hoveredNodes.push_back(t);
        t = t->parentNode();
    }
}

void BrowsingContext::releaseHoveredNode()
{
    if (m_hoveredNodes.size() == 0) {
        return;
    }

    for (size_t i = 0; i < m_hoveredNodes.size(); i++) {
        m_hoveredNodes[i]->setState(Node::NodeStateHovered,
                                    Node::ChildrenOrSiblingsAffectedByHover,
                                    false);
    }
    m_hoveredNodes.clear();
    m_hoveredNodes.shrink_to_fit();
}

void BrowsingContext::dispatchTouchEvent(float x, float y,
                                         PlatformWindow::TouchEventKind kind,
                                         bool isMobile)
{
    if (!m_isRunning) {
        return;
    }

    if (kind == PlatformWindow::TouchEventStart) { // or MouseEventDown
        Node* node = hitTest(x, y);
        if (!node) {
            return;
        }
        m_touchDownPoint = Unit::Location(x, y);
        setActiveNode(node);
        setFocusedNode(node);

        String* eventType;
        Event* e;
        if (isMobile) {
            eventType = starFish()->staticStrings()->m_touchstart.localName();
            e = new TouchEvent(document(), eventType, UIEventInit(true, true));
        } else {
            eventType = starFish()->staticStrings()->m_mousedown.localName();
            e = new MouseEvent(document(), eventType,
                               MouseEventInit(true, true));
        }

        if (m_activeNodes.size() > 0) {
            document()->window()->dispatchEvent(m_activeNodes[0], e);
        } else {
            document()->window()->dispatchEvent(document(), e);
        }

    } else if (kind == PlatformWindow::TouchEventMove) { // or MouseEventMove
        if ((starFish()->deviceKind() & deviceKindUseTouchScreen) &&
            ((abs(m_touchDownPoint.x() - x) > 30) ||
             (abs(m_touchDownPoint.y() - y) > 30))) {
            releaseActiveNode();
        }
        Node* node = hitTest(x, y);
        if (!node) {
            return;
        }

        Node* t = node->nearestParentElement();
        if (!isMobile) {
            bool check = true;
            if (m_hoveredNodes.size() > 0) {
                check = (t != m_hoveredNodes[0]);
            }

            if (check) {
                releaseHoveredNode();
                setHoveredNode(node);

                String* eventType =
                    starFish()->staticStrings()->m_mouseover.localName();
                Event* e = new MouseEvent(document(), eventType,
                                          MouseEventInit(true, true));

                if (t) {
                    document()->window()->dispatchEvent(t, e);
                } else {
                    document()->window()->dispatchEvent(document(), e);
                }
            }
        }

        String* eventType;
        Event* e;
        if (isMobile) {
            eventType = starFish()->staticStrings()->m_touchmove.localName();
            e = new TouchEvent(document(), eventType, UIEventInit(true, true));
        } else {
            eventType = starFish()->staticStrings()->m_mousemove.localName();
            e = new MouseEvent(document(), eventType,
                               MouseEventInit(true, true));
        }

        if (t) {
            document()->window()->dispatchEvent(t, e);
        } else {
            document()->window()->dispatchEvent(document(), e);
        }
    } else if (kind == PlatformWindow::TouchEventCancel) {
        releaseActiveNode();
        releaseHoveredNode();
    } else {
        STARFISH_ASSERT(kind ==
                        PlatformWindow::TouchEventEnd); // or MouseEventUp

        Node* node = hitTest(x, y);
        if (!node) {
            return;
        }

        Node* t = node->nearestParentElement();
        bool check = false;
        if (m_activeNodes.size() > 0) {
            check = (t == m_activeNodes[0]);
        }

        if (check) {
            String* eventType =
                starFish()->staticStrings()->m_click.localName();
            Event* e;

            String* eventType2;
            Event* e2;
            if (isMobile) {
                e = new TouchEvent(document(), eventType,
                                   UIEventInit(true, true));
                eventType2 =
                    starFish()->staticStrings()->m_touchend.localName();
                e2 = new TouchEvent(document(), eventType2,
                                    UIEventInit(true, true));
            } else {
                e = new MouseEvent(document(), eventType,
                                   MouseEventInit(true, true));
                eventType2 = starFish()->staticStrings()->m_mouseup.localName();
                e2 = new MouseEvent(document(), eventType2,
                                    MouseEventInit(true, true));
            }

            if (t) {
                document()->window()->dispatchEvent(t, e);
                document()->window()->dispatchEvent(t, e2);
            } else {
                document()->window()->dispatchEvent(document(), e);
                document()->window()->dispatchEvent(document(), e2);
            }
        }

        releaseActiveNode();
    }
}

void BrowsingContext::dispatchMouseEvent(float x, float y,
                                         PlatformWindow::MouseEventKind kind)
{
    if (kind <= PlatformWindow::MouseEventUp) {
        dispatchTouchEvent(x, y, (PlatformWindow::TouchEventKind)kind, false);
    } else if (kind == PlatformWindow::MouseEventEnter) {
    } else {
        STARFISH_ASSERT(kind == PlatformWindow::MouseEventOut);
    }
}

void BrowsingContext::dispatchKeyEvent(String* key,
                                       PlatformWindow::KeyEventKind kind)
{
    String* eventType = String::emptyString;
    if (kind == PlatformWindow::KeyEventKind::KeyEventUp) {
        eventType = starFish()->staticStrings()->m_keyup.localName();
    } else {
        // kind == KeyEventKind::KeyEventDown
        eventType = starFish()->staticStrings()->m_keydown.localName();
    }
    KeyboardEventInit eventInit(true, true);
    eventInit.setKey(key);
    KeyboardEvent* e = new KeyboardEvent(document(), eventType, eventInit);

    if (e->ctrlKey()) {
        m_ctrlKeyDown = kind == PlatformWindow::KeyEventKind::KeyEventDown
                            ? m_ctrlKeyDown + 1
                            : m_ctrlKeyDown - 1;
        STARFISH_ASSERT(m_ctrlKeyDown >= 0);
    } else if (e->altKey()) {
        m_altKeyDown = kind == PlatformWindow::KeyEventKind::KeyEventDown
                           ? m_altKeyDown + 1
                           : m_altKeyDown - 1;
        STARFISH_ASSERT(m_altKeyDown >= 0);
    } else if (e->shiftKey()) {
        m_shiftKeyDown = kind == PlatformWindow::KeyEventKind::KeyEventDown
                             ? m_shiftKeyDown + 1
                             : m_shiftKeyDown - 1;
        STARFISH_ASSERT(m_shiftKeyDown >= 0);
    } else if (e->shiftKey()) {
        m_metaKeyDown = kind == PlatformWindow::KeyEventKind::KeyEventDown
                            ? m_metaKeyDown + 1
                            : m_metaKeyDown - 1;
        STARFISH_ASSERT(m_metaKeyDown >= 0);
    }
    if (m_ctrlKeyDown > 0) {
        e->setCtrlKey();
    }
    if (m_altKeyDown > 0) {
        e->setAltKey();
    }
    if (m_shiftKeyDown > 0) {
        e->setShiftKey();
    }
    if (m_metaKeyDown > 0) {
        e->setMetaKey();
    }

    // [Target]
    // 1) currently focused element if possible -> no focus concept
    // or 2) body element if possible
    // or 3) root element
    if (document()->rootElement()) {
        document()->window()->dispatchEvent(
            (document()->body() ? document()->body()->asNode()
                                : document()->rootElement()->asNode()),
            e);
    }
}

void BrowsingContext::pause()
{
    STARFISH_LOG_INFO("BrowsingContext::pause\n");
    if (!m_isRunning) {
        return;
    }

    m_isRunning = false;

    document()->setVisibilityState(VisibilityState::VisibilityStateHidden);

    document()->resourceLoader().cachePruning();
}

void BrowsingContext::resume()
{
    STARFISH_LOG_INFO("BrowsingContext::resume\n");
    if (m_isRunning) {
        m_needsRendering = true;
        m_needsPainting = true;
        rendering();
        return;
    }

    m_isRunning = true;
    m_needsRendering = true;
    m_needsPainting = true;
    rendering();

    document()->setVisibilityState(VisibilityState::VisibilityStateVisible);
}
}
