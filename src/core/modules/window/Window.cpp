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

#include "StarFish.h"

#include "core/animation/Animation.h"
#include "binding/ScriptBindingInstance.h"
#include "core/dom/FocusEvent.h"
#include "core/dom/Element.h"
#ifdef STARFISH_ENABLE_MULTI_PAGE
#include "core/dom/HTMLAnchorElement.h"
#endif
#include "core/dom/HTMLDocument.h"
#include "core/dom/HTMLBodyElement.h"
#include "core/dom/HTMLCollection.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/dom/MouseEvent.h"
#include "core/dom/KeyboardEvent.h"
#include "core/dom/TouchEvent.h"
#include "core/dom/Traverse.h"
#include "core/page/History.h"
#include "core/page/Navigator.h"
#include "core/page/Location.h"
#include "core/page/Screen.h"
#include "core/page/SecurityOriginData.h"
#include "core/storage/Storage.h"
#include "core/storage/StorageNamespace.h"
#include "browser/storage/WebStorageNamespaceProvider.h"
#if defined(STARFISH_TIZEN_TV) && defined(STARFISH_ENABLE_AVPLAY)
#include "core/extra/WebApis.h"
#endif
#include "core/layout/Frame.h"
#include "core/layout/FrameBox.h"
#include "core/layout/FrameBlockBox.h"
#include "core/layout/FrameTreeBuilder.h"
#include "core/layout/StackingContext.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/window/Window.h"
#include "core/modules/message_loop/Timer.h"

#if defined(PORT_GRAPHIC_BACKEND_EFL) && defined(STARFISH_ENABLE_TEST)
#include <Elementary.h>
Evas_Object* g_imgBufferForScreehShot;
#endif

#ifdef STARFISH_ENABLE_TEST
#include <sys/ioctl.h>
#include <net/if.h>

bool g_fireOnloadEvent = false;
bool g_forceRendering = false;
StarFish::CanvasSurface* g_surfaceForScreehShot;
#endif

// #define STARFISH_ENABLE_TIMER

namespace StarFish {

Window::Window(StarFish* starFish)
    : EventTarget(nullptr)
    , m_starFish(starFish)
    , m_scriptBindingInstance(nullptr)
    , m_history(nullptr)
    , m_navigator(nullptr)
    , m_location(nullptr)
    , m_screen(nullptr)
    , m_animationExecutor(nullptr)
    , m_localStorageNamespace(nullptr)
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
    initFlags();
}

void Window::initFlags()
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

void Window::navigate(URL* url)
{
    close();
    initFlags();
    STARFISH_LOG_INFO("Window::navigate %s\n", url->urlString()->utf8Data());

    // WindowImplEFL* eflWindow = (WindowImplEFL*)this;
    m_isActive = true;

    m_scriptBindingInstance = new ScriptBindingInstance();
    StarFishEnterer enter(m_starFish);
    m_scriptBindingInstance->initBinding(m_starFish);
    scriptObjectSlowCase();

    m_history = new History(m_starFish);
    m_navigator = new Navigator(m_starFish);
    m_location = new Location(m_starFish);
#if defined(STARFISH_TIZEN_TV) && defined(STARFISH_ENABLE_AVPLAY)
    m_webapis = new WebApis(m_starFish);
#endif

    initStorage(url);

    m_document = new HTMLDocument(this, scriptBindingInstance(), url,
                                  String::createASCIIString("UTF-8"), true);

    m_document->open();
}

void Window::initStorage(URL* url)
{
    if (!m_localStorageNamespace) {
        // TODO: The name of disk storage file name should be auto-generated
        StorageNamespaceProvider* storageProvider =
            WebStorageNamespaceProvider::create(
                m_starFish, String::createASCIIString("./cache/cache.db"));
        m_localStorageNamespace =
            storageProvider->createLocalStorageNamespace();
        m_sessionStorageNamespace =
            storageProvider->createSessionStorageNamespace();
    }
}

Storage* Window::localStorage()
{
    URL* url = m_document->documentURI();
    SecurityOriginData* origin = new SecurityOriginData(
        url->protocol(), url->host(), String::parseInt(url->port()));
    return m_localStorageNamespace->storage(origin);
}

Storage* Window::sessionStorage()
{
    URL* url = m_document->documentURI();
    SecurityOriginData* origin = new SecurityOriginData(
        url->protocol(), url->host(), String::parseInt(url->port()));
    return m_sessionStorageNamespace->storage(origin);
}

void Window::setHistory(URL* url)
{
    if (!m_history) {
        m_history = new History(m_starFish);
    }
    m_history->setHistory(ScriptValue(ScriptValue::ESNull), String::emptyString,
                          url);
}

Screen* Window::screen()
{
    if (!m_screen) {
        m_screen = new Screen(m_starFish);
    }
    return m_screen;
}

float Window::devicePixelRatio()
{
    return starFish()->devicePixelRatio();
}

void Window::navigateAsync(URL* url)
{
    starFish()->messageLoop()->addIdlerWithNoScriptInstanceEntering(
        [](size_t a, void* data, void* data2) {
            ((Window*)data2)->setHistory((URL*)data);
            ((Window*)data2)->navigate((URL*)data);
        },
        url, this);
}

void Window::navigateAsyncWithoutSetHistory(URL* url)
{
    starFish()->messageLoop()->addIdlerWithNoScriptInstanceEntering(
        [](size_t a, void* data, void* data2) {
            ((Window*)data2)->navigate((URL*)data);
        },
        url, this);
}

void Window::layoutIfNeeds()
{
    if (m_needsStyleRecalc || m_needsStyleRecalcForWholeDocument) {
        if (m_needsStyleRecalcForWholeDocument) {
#ifdef STARFISH_ENABLE_TIMER
            Timer t("parse sheet");
#endif
            CSSStyleSheet* uaSheet = document()->styleResolver().sheets()[0];

            uaSheet->parseSheetIfneeds();
            uaSheet->collectRulesForSheet(uaSheet->allRules());
            uaSheet->sortRulesBySpecificity();

            document()->styleResolver().removeAllRules();

            size_t sheets = document()->styleResolver().sheets().size();
            for (size_t i = 1; i < sheets; i++) {
                CSSStyleSheet* authorSheet =
                    document()->styleResolver().sheets()[i];

                authorSheet->parseSheetIfneeds();

                if (authorSheet->ownerRule()) {
                    authorSheet->collectRulesForImportedSheet();
                } else {
                    authorSheet->collectRulesForSheet(authorSheet->allRules());
                }

                size_t rules = authorSheet->rules().size();
                for (size_t j = 0; j < rules; j++) {
                    document()->styleResolver().allRules()->addRule(
                        authorSheet->rules()[j]);
                }
            }
            document()->styleResolver().allRules()->sortRulesBySpecificity();
        }

// resolve style
#ifdef STARFISH_ENABLE_TIMER
        Timer t("resolve style");
#endif
        document()->styleResolver().resolveDOMStyle(
            m_document, m_needsStyleRecalcForWholeDocument);
        m_needsStyleRecalc = false;
        m_needsStyleRecalcForWholeDocument = false;

#ifdef STARFISH_ENABLE_TEST
        if (m_starFish->startUpFlag() &
            StarFishStartUpFlag::enableComputedStyleDump) {
            // dump style
            document()->styleResolver().dumpDOMStyle(m_document);
        }
#endif
    }

    if (m_needsFrameTreeBuild) {
        if (m_document->frame()) {
            clearStackingContext(true);

// create frame tree
#ifdef STARFISH_ENABLE_TIMER
            Timer t("create frame tree");
#endif
            FrameTreeBuilder::buildFrameTree(m_document);
            m_needsFrameTreeBuild = false;
        }
    }

    if (m_needsLayout) {
// lay out frame tree
#ifdef STARFISH_ENABLE_TIMER
        Timer t("lay out frame tree");
#endif
        clearStackingContext(true);

        LayoutContext ctx(starFish(), m_document->frame()
                                          ->asFrameBox()
                                          ->asFrameBlockBox()
                                          ->asFrameDocument());
        m_document->frame()->layout(ctx,
                                    Frame::LayoutWantToResolve::ResolveAll);

#ifndef NDEBUG
        {
            LayoutContext ctx(starFish(), m_document->frame()
                                              ->asFrameBox()
                                              ->asFrameBlockBox()
                                              ->asFrameDocument());
            m_document->frame()->layout(ctx,
                                        Frame::LayoutWantToResolve::ResolveAll);
        }
#endif
        {
#ifdef STARFISH_ENABLE_TIMER
            Timer t("computeStackingContextProperties");
#endif
            m_document->frame()->establishesStackingContextIfNeeds();
            if (m_document->frame()->firstChild()) {
                m_rootStackingContext = m_document->frame()
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
            FrameTreeBuilder::dumpFrameTree(m_document);
        }
#endif
    }
}

void Window::rendering()
{
    if (m_pendingStyleSheetCount && document() &&
        document()->resourceLoader().isDocumentInOpenState() &&
        ((timestamp() - document()->resourceLoader().documentOpenTime()) <
         1000)) {
        m_needsRendering = false;
        setTimeout([](Window* wnd, void* data) { wnd->setNeedsRendering(); },
                   100, nullptr);

        Canvas* canvas = preparePainting(true);
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
    Timer renderingTimer("Window::rendering");
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
        Canvas* canvas = preparePainting(true);

        if (m_document->frame()->firstChild()) {
            m_needsComposite = m_rootStackingContext->needsOwnBuffer();
        } else {
            m_needsComposite = false;
        }

        if (!m_needsComposite) {
            paintWindowBackground(canvas);
        }

        {
            PaintingContext ctx(canvas);
            ctx.m_paintingStage = PaintingStageEnd;
            m_document->frame()->paint(ctx);
        }
        m_needsPainting = false;

        delete canvas;
#ifdef STARFISH_TIZEN_WEARABLE
        evas_object_raise(eflWindow->m_dummyBox);
#endif

#ifdef STARFISH_ENABLE_TEST
        if (m_starFish->startUpFlag() &
            StarFishStartUpFlag::enableStackingContextDump) {
            if (m_document->frame()->firstChild()) {
                STARFISH_ASSERT(m_document->frame()
                                    ->firstChild()
                                    ->asFrameBox()
                                    ->isRootElement());
                StackingContext* ctx = m_document->frame()
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
        if (m_document->frame()->firstChild() &&
            m_rootStackingContext->needsOwnBuffer()) {
            Canvas* canvas = preparePainting(false);
            paintWindowBackground(canvas);
            m_document->frame()
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

#if defined(PORT_GRAPHIC_BACKEND_EFL) && defined(STARFISH_ENABLE_TEST)
    {
        const char* path = getenv("SCREEN_SHOT");
        if (path && strlen(path) && g_fireOnloadEvent) {
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

            g_surfaceForScreehShot->detachNativeBuffer();
            g_surfaceForScreehShot = nullptr;
        }
    }
#endif
}

void Window::paintWindowBackground(Canvas* canvas)
{
#ifdef STARFISH_TIZEN
    if (!document()->m_tizenWidgetTransparentBackground) {
        canvas->clearColor(Unit::Color(255, 255, 255, 255));
    }
#else
    canvas->clearColor(Unit::Color(255, 255, 255, 255));
#endif

    if (m_hasRootElementBackground || m_hasBodyElementBackground) {
        LayoutRect colorRect(0, 0, width(), height());
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
            LayoutRect imgRect(0, 0, width(), height());
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

void Window::markHasPendingStyleSheet()
{
    STARFISH_LOG_INFO("Window::markHasPendingStyleSheet\n");
    m_pendingStyleSheetCount++;
}

void Window::unmarkHasPendingStyleSheet()
{
    STARFISH_LOG_INFO("Window::unmarkHasPendingStyleSheet\n");
    if (m_pendingStyleSheetCount > 0) {
        m_pendingStyleSheetCount--;
        setNeedsRendering();
    }
}

void Window::clearStackingContext(bool backupBuffer)
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

#ifdef STARFISH_ENABLE_TEST
void Window::setNetworkState(bool state)
{
    int sockfd;
    struct ifreq ifr;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0) {
        return;
    }

    memset(&ifr, 0, sizeof ifr);
    strncpy(ifr.ifr_name, "eth0", IFNAMSIZ);

    if (state) {
        ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
    } else {
        ifr.ifr_flags |= ~IFF_RUNNING;
        // ifr.ifr_flags |= ~IFF_UP;
    }

    ioctl(sockfd, SIOCSIFFLAGS, &ifr);
}

void Window::forceDisableOnloadCapture()
{
    setenv("SCREEN_SHOT", "", 1);
}

void Window::simulateClick(float x, float y)
{
    dispatchTouchEvent(x, y, Window::TouchEventStart, true);
    dispatchTouchEvent(x, y, Window::TouchEventEnd, true);
}

void Window::simulateVisibilitychange(bool show)
{
    if (show) {
        m_starFish->resume();
    } else {
        m_starFish->pause();
    }
}

void Window::testStart()
{
    ESValue v = ESVMInstance::currentInstance()->globalObject()->get(
        ESString::create("testStart"));
    if (!v.isUndefined()) {
        callScriptFunction(v, {}, 0,
                           ESVMInstance::currentInstance()->globalObject());
    }
}
#endif

void Window::close()
{
    STARFISH_LOG_INFO("Window::close\n");
    clearEventListeners();

    if (m_navigator) {
        m_navigator->close();
    }

    m_focusedNode = nullptr;
    m_relatedTarget = nullptr;
    m_cssTarget = nullptr;

    m_activeNodes.clear();
    m_activeNodes.shrink_to_fit();
    m_hoveredNodes.clear();
    m_hoveredNodes.shrink_to_fit();

    if (m_location) {
        m_location->close();
    }

    if (m_document) {
        StarFishEnterer enter(m_starFish);
        m_document->close();
        delete m_document;
        m_document = nullptr;
    }

    if (m_scriptBindingInstance) {
        if (true) {
            StarFishEnterer enter(m_starFish);
            m_scriptBindingInstance->close();
        }
        delete m_scriptBindingInstance;
        m_scriptBindingInstance = nullptr;
    }

    m_isActive = false;

    m_starFish->timer()->clear();
    clearResources();

    m_starFish->messageLoop()->clearPendingIdlers();
    m_starFish->clearBlobURLStore();
}

void Window::setWholeDocumentNeedsStyleRecalc()
{
    m_needsStyleRecalcForWholeDocument = true;
    setNeedsRendering();
}

uint32_t Window::setTimeout(WindowSetTimeoutHandler handler, int32_t delay,
                            void* data)
{
    STARFISH_RELEASE_ASSERT(m_isActive);
    return m_starFish->timer()->addTimer(delay, handler, data, false);
}

void Window::clearTimeout(int32_t id)
{
    STARFISH_RELEASE_ASSERT(m_isActive);
    m_starFish->timer()->removeTimer(id);
}

uint32_t Window::setInterval(WindowSetTimeoutHandler handler, int32_t delay,
                             void* data)
{
    STARFISH_RELEASE_ASSERT(m_isActive);
    return m_starFish->timer()->addTimer(delay, handler, data, true);
}

void Window::clearInterval(int32_t id)
{
    STARFISH_RELEASE_ASSERT(m_isActive);
    m_starFish->timer()->removeTimer(id);
}

uint32_t Window::requestAnimationFrame(WindowSetTimeoutHandler handler,
                                       void* data)
{
    STARFISH_RELEASE_ASSERT(m_isActive);
    return m_starFish->timer()->addAnimator(handler, data);
}

void Window::cancelAnimationFrame(int32_t reqID)
{
    m_starFish->timer()->removeWindowAnimator(reqID);
}

Node* Window::hitTest(float x, float y)
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

void Window::setFocusedNode(Node* n)
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
            e = new FocusEvent(eventType, FocusEventInit());
            EventTarget::dispatchEvent(t->asNode(), e);
        }
        if (t->isHTMLElement() && !t->isHTMLBodyElement()) {
            eventType = starFish()->staticStrings()->m_focusout.localName();
            e = new FocusEvent(eventType, FocusEventInit(true));
            EventTarget::dispatchEvent(t->asNode(), e);
        }
    }
    m_relatedTarget = t;
    releaseFocusedNode();

    t = m;
    if (t) {
        if (t->isHTMLElement()) {
            eventType = starFish()->staticStrings()->m_focus.localName();
            e = new FocusEvent(eventType, FocusEventInit());
            EventTarget::dispatchEvent(t->asNode(), e);
        }
        if (t->isHTMLElement() && !t->isHTMLBodyElement()) {
            eventType = starFish()->staticStrings()->m_focusin.localName();
            e = new FocusEvent(eventType, FocusEventInit(true));
            EventTarget::dispatchEvent(t->asNode(), e);
        }
    }
    m_focusedNode = t;
}

void Window::releaseFocusedNode()
{
    if (m_relatedTarget) {
        m_relatedTarget->setState(Node::NodeStateFocused,
                                  Node::ChildrenOrSiblingsAffectedByFocus,
                                  false);
    }
}

void Window::setActiveNode(Node* n)
{
    Node* t = n->nearestParentElement();
    while (t) {
        t->setState(Node::NodeStateActive,
                    Node::ChildrenOrSiblingsAffectedByActive, true);
        m_activeNodes.push_back(t);
        t = t->parentNode();
    }
}

void Window::releaseActiveNode()
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

void Window::setHoveredNode(Node* n)
{
    Node* t = n->nearestParentElement();
    while (t) {
        t->setState(Node::NodeStateHovered,
                    Node::ChildrenOrSiblingsAffectedByHover, true);
        m_hoveredNodes.push_back(t);
        t = t->parentNode();
    }
}

void Window::releaseHoveredNode()
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

void Window::processUrlFragment(String* name)
{
    Node* n = document()->getElementById(name);
    if (n) {
        setCSSTarget(n);
        return;
    }

    Node* anchor = Traverse::findDescendant(document(), [&](Node* child) {
        if (child->isHTMLAnchorElement() &&
            child->asHTMLAnchorElement()->name().localName()->equals(name)) {
            return true;
        } else {
            return false;
        }
    });

    if (anchor) {
        setCSSTarget(anchor);
    }
}

void Window::setCSSTarget(Node* n)
{
    releaseCSSTarget();

    m_cssTarget = n;
    if (m_cssTarget) {
        m_cssTarget->setState(Node::NodeStateTarget, Node::NotAffected, true);
    }
}

void Window::releaseCSSTarget()
{
    if (m_cssTarget) {
        m_cssTarget->setState(Node::NodeStateTarget, Node::NotAffected, false);
    }
}

DEFINE_EVENT_LISTENER(Window, abort);
DEFINE_EVENT_LISTENER(Window, canplay);
DEFINE_EVENT_LISTENER(Window, canplaythrough);
DEFINE_EVENT_LISTENER(Window, click);
DEFINE_EVENT_LISTENER(Window, durationchange);
DEFINE_EVENT_LISTENER(Window, emptied);
DEFINE_EVENT_LISTENER(Window, ended);
DEFINE_EVENT_LISTENER(Window, error);
DEFINE_EVENT_LISTENER(Window, focus);
DEFINE_EVENT_LISTENER(Window, keydown);
DEFINE_EVENT_LISTENER(Window, keyup);
DEFINE_EVENT_LISTENER(Window, load);
DEFINE_EVENT_LISTENER(Window, loadeddata);
DEFINE_EVENT_LISTENER(Window, loadedmetadata);
DEFINE_EVENT_LISTENER(Window, loadstart);
DEFINE_EVENT_LISTENER(Window, mouseover);
DEFINE_EVENT_LISTENER(Window, pause);
DEFINE_EVENT_LISTENER(Window, play);
DEFINE_EVENT_LISTENER(Window, playing);
DEFINE_EVENT_LISTENER(Window, progress);
DEFINE_EVENT_LISTENER(Window, ratechange);
DEFINE_EVENT_LISTENER(Window, seeked);
DEFINE_EVENT_LISTENER(Window, seeking);
DEFINE_EVENT_LISTENER(Window, stalled);
DEFINE_EVENT_LISTENER(Window, suspend);
DEFINE_EVENT_LISTENER(Window, timeupdate);
DEFINE_EVENT_LISTENER(Window, volumechange);
DEFINE_EVENT_LISTENER(Window, waiting);

DEFINE_EVENT_LISTENER(Window, unload);

void Window::dispatchTouchEvent(float x, float y, TouchEventKind kind,
                                bool isMobile)
{
    if (!m_isRunning) {
        return;
    }

    if (kind == TouchEventStart) { // or MouseEventDown
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
            e = new TouchEvent(eventType, UIEventInit(true, true));
        } else {
            eventType = starFish()->staticStrings()->m_mousedown.localName();
            e = new MouseEvent(eventType, MouseEventInit(true, true));
        }

        if (m_activeNodes.size() > 0) {
            EventTarget::dispatchEvent(m_activeNodes[0], e);
        } else {
            EventTarget::dispatchEvent(m_document, e);
        }

    } else if (kind == TouchEventMove) { // or MouseEventMove
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
                Event* e =
                    new MouseEvent(eventType, MouseEventInit(true, true));

                if (t) {
                    EventTarget::dispatchEvent(t, e);
                } else {
                    EventTarget::dispatchEvent(m_document, e);
                }
            }
        }

        String* eventType;
        Event* e;
        if (isMobile) {
            eventType = starFish()->staticStrings()->m_touchmove.localName();
            e = new TouchEvent(eventType, UIEventInit(true, true));
        } else {
            eventType = starFish()->staticStrings()->m_mousemove.localName();
            e = new MouseEvent(eventType, MouseEventInit(true, true));
        }

        if (t) {
            EventTarget::dispatchEvent(t, e);
        } else {
            EventTarget::dispatchEvent(m_document, e);
        }
    } else if (kind == TouchEventCancel) {
        releaseActiveNode();
        releaseHoveredNode();
    } else {
        STARFISH_ASSERT(kind == TouchEventEnd); // or MouseEventUp

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
                e = new TouchEvent(eventType, UIEventInit(true, true));
                eventType2 =
                    starFish()->staticStrings()->m_touchend.localName();
                e2 = new TouchEvent(eventType2, UIEventInit(true, true));
            } else {
                e = new MouseEvent(eventType, MouseEventInit(true, true));
                eventType2 = starFish()->staticStrings()->m_mouseup.localName();
                e2 = new MouseEvent(eventType2, MouseEventInit(true, true));
            }

            if (t) {
                EventTarget::dispatchEvent(t, e);
                EventTarget::dispatchEvent(t, e2);
            } else {
                EventTarget::dispatchEvent(m_document, e);
                EventTarget::dispatchEvent(m_document, e2);
            }
        }

        releaseActiveNode();
    }
}

void Window::dispatchMouseEvent(float x, float y, MouseEventKind kind)
{
    if (kind <= MouseEventUp) {
        dispatchTouchEvent(x, y, (TouchEventKind)kind, false);
    } else if (kind == MouseEventEnter) {
    } else {
        STARFISH_ASSERT(kind == MouseEventOut);
    }
}

void Window::dispatchKeyEvent(String* key, KeyEventKind kind)
{
    String* eventType = String::emptyString;
    if (kind == KeyEventKind::KeyEventUp) {
        eventType = starFish()->staticStrings()->m_keyup.localName();
    } else {
        // kind == KeyEventKind::KeyEventDown
        eventType = starFish()->staticStrings()->m_keydown.localName();
    }
    KeyboardEventInit eventInit(true, true);
    eventInit.setKey(key);
    KeyboardEvent* e = new KeyboardEvent(eventType, eventInit);

    if (e->ctrlKey()) {
        m_ctrlKeyDown = kind == KeyEventKind::KeyEventDown ? m_ctrlKeyDown + 1
                                                           : m_ctrlKeyDown - 1;
        STARFISH_ASSERT(m_ctrlKeyDown >= 0);
    } else if (e->altKey()) {
        m_altKeyDown = kind == KeyEventKind::KeyEventDown ? m_altKeyDown + 1
                                                          : m_altKeyDown - 1;
        STARFISH_ASSERT(m_altKeyDown >= 0);
    } else if (e->shiftKey()) {
        m_shiftKeyDown = kind == KeyEventKind::KeyEventDown
                             ? m_shiftKeyDown + 1
                             : m_shiftKeyDown - 1;
        STARFISH_ASSERT(m_shiftKeyDown >= 0);
    } else if (e->shiftKey()) {
        m_metaKeyDown = kind == KeyEventKind::KeyEventDown ? m_metaKeyDown + 1
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
        EventTarget::dispatchEvent((document()->body()
                                        ? document()->body()->asNode()
                                        : document()->rootElement()->asNode()),
                                   e);
    }
}

CSSStyleDeclaration* Window::getComputedStyle(Element* element)
{
    return element->getComputedStyle();
}

CSSStyleDeclaration* Window::getComputedStyle(Element* element,
                                              String* pseudoElt)
{
    return element->getComputedStyle();
}

// https://html.spec.whatwg.org/multipage/browsers.html#named-access-on-the-window-object
HTMLCollection* Window::namedAccess(String* name)
{
    // TODO
    // when child browser context(ex- iframe) implemented, we should
    // re-implement this block
    if (document()) {
        return document()->namedAccess(name);
    } else {
        return nullptr;
    }
}

void Window::pause()
{
    STARFISH_LOG_INFO("Window::pause\n");
    if (!m_isRunning) {
        return;
    }

    m_isRunning = false;

    document()->setVisibilityState(VisibilityState::VisibilityStateHidden);

    document()->resourceLoader().cachePruning();
}

void Window::resume()
{
    STARFISH_LOG_INFO("Window::resume\n");
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

void Window::screenShot(std::string filePath)
{
    bool oldNeedsPainting = m_needsPainting;
    bool oldOnLoad = g_fireOnloadEvent;
    g_fireOnloadEvent = true;
    g_forceRendering = true;
    setNeedsPainting();
    setenv("SCREEN_SHOT", filePath.data(), 1);
    rendering();
    setenv("SCREEN_SHOT", "", 1);
    g_fireOnloadEvent = oldOnLoad;
    g_forceRendering = false;

    m_needsPainting = oldNeedsPainting;
    setNeedsRendering();
}
}
