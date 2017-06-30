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
#include "WebView.h"

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
#include "core/dom/Traverse.h"
#include "core/dom/HTMLIFrameElement.h"

// #define STARFISH_ENABLE_TIMER

namespace StarFish {

BrowsingContext* BrowsingContext::create(StarFish* starFish, WebView* webView)
{
    STARFISH_ASSERT(webView);
    return new BrowsingContext(starFish, webView);
}

BrowsingContext* BrowsingContext::create(HTMLIFrameElement* sourceElement)
{
    return new BrowsingContext(
        sourceElement->starFish(),
        sourceElement->document()->browsingContext()->webView(), sourceElement);
}

BrowsingContext::BrowsingContext(StarFish* starFish, WebView* webView,
                                 HTMLIFrameElement* source)
    : StarFishHoldable(starFish)
    , m_webView(webView)
    , m_window(nullptr)
    , m_parentBrowsingContext(source ? source->document()->browsingContext()
                                     : nullptr)
    , m_sourceElement(source)
#if defined(STARFISH_TIZEN_TV) && defined(STARFISH_ENABLE_AVPLAY)
    , m_webapis(nullptr)
#endif
    , m_touchDownPoint(0, 0)
    , m_ctrlKeyDown(0)
    , m_shiftKeyDown(0)
    , m_altKeyDown(0)
    , m_metaKeyDown(0)
{
    initFlags();
}

void BrowsingContext::initFlags()
{
    m_needsStyleRecalc = false;
    m_needsStyleRecalcForWholeDocument = false;
    m_needsFrameTreeBuild = false;
    m_needsLayout = false;

    m_hasRootElementBackground = false;
    m_hasBodyElementBackground = false;
    m_isRunning = true;
    m_pendingStyleSheetCount = 0;
}

void BrowsingContext::navigate(ResourceURL* url)
{
    close();
    initFlags();

    m_isActive = true;

    StarFishEnterer enter(m_starFish);

    // TODO: Use location to open a new document
    if (isMainBrowsingContext()) {
        m_window = Window::create(m_starFish, this, url,
                                  starFish()->platformWindow()->width(),
                                  starFish()->platformWindow()->height());
    } else {
        if (m_sourceElement->frame()) {
            m_window = Window::create(m_starFish, this, url,
                                      (uint32_t)m_sourceElement->frame()
                                          ->asFrameBox()
                                          ->contentWidth(),
                                      (uint32_t)m_sourceElement->frame()
                                          ->asFrameBox()
                                          ->contentHeight());
        } else {
            m_window = Window::create(m_starFish, this, url,
                                      STARFISH_DEFAULT_IFRAME_WIDTH,
                                      STARFISH_DEFAULT_IFRAME_HEIGHT);
        }
    }

    m_window->document()->open();
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

Document* BrowsingContext::document()
{
    return window()->document();
}

ScriptBindingInstance* BrowsingContext::scriptBindingInstance()
{
    return window()->scriptBindingInstance();
}

bool BrowsingContext::layoutIfNeeds()
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

                authorSheet->clearStyleRules();
                authorSheet->collectRulesFromImportedSheet(
                    authorSheet->importRules());
                authorSheet->collectStyleRules(authorSheet->childRules(),
                                               authorSheet->url());

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
            webView()->clearStackingContext(true);
// create frame tree
#ifdef STARFISH_ENABLE_TIMER
            ProfilerTimer t("create frame tree");
#endif
            FrameTreeBuilder::buildFrameTree(document());
            m_needsLayout = true;
            m_needsFrameTreeBuild = false;
        }
    }

    if (m_needsLayout) {
        webView()->clearStackingContext(true);
// lay out frame tree
#ifdef STARFISH_ENABLE_TIMER
        ProfilerTimer t("lay out frame tree");
#endif

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
        m_needsLayout = false;
        return true;
    }
    return false;
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

void BrowsingContext::iterateChildContext(
    const std::function<void(BrowsingContext*)>& fn)
{
    GCVector<Element*> col;
    Traverse::getherDescendant(col, document(),
                               [](Node* nd) -> bool {
                                   if (nd->isHTMLIFrameElement()) {
                                       return true;
                                   }
                                   return false;
                               },
                               false);

    for (size_t i = 0; i < col.size(); i++) {
        if (col[i]->asHTMLIFrameElement()->browsingContext()) {
            fn(col[i]->asHTMLIFrameElement()->browsingContext());
        }
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

    if (m_window) {
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
    }

    m_isActive = false;

    if (isMainBrowsingContext()) {
        m_starFish->timer()->clear(nullptr);
        m_starFish->platformWindow()->clearResources();
        m_starFish->messageLoop()->clearPendingIdlers(nullptr);

        m_webView->initRenderingFlags();
    } else {
        m_starFish->timer()->clear(this);
        m_starFish->messageLoop()->clearPendingIdlers(this);
    }
}

void BrowsingContext::setWholeDocumentNeedsStyleRecalc()
{
    m_needsStyleRecalcForWholeDocument = true;
    setNeedsRendering();
    registerNeedsLayoutInWebView();
}

Node* BrowsingContext::hitTest(float x, float y)
{
    webView()->layoutIfNeeds();

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
            e = new FocusEvent(document(), eventType);
            document()->dispatchEvent(t->asNode(), e);
        }
        if (t->isHTMLElement() && !t->isHTMLBodyElement()) {
            eventType = starFish()->staticStrings()->m_focusout.localName();
            FocusEventInit init;
            init.setBubbles(true);
            e = new FocusEvent(document(), eventType, init);
            document()->dispatchEvent(t->asNode(), e);
        }
    }
    m_relatedTarget = t;
    releaseFocusedNode();

    t = m;
    if (t) {
        if (t->isHTMLElement()) {
            eventType = starFish()->staticStrings()->m_focus.localName();
            e = new FocusEvent(document(), eventType);
            document()->dispatchEvent(t->asNode(), e);
        }
        if (t->isHTMLElement() && !t->isHTMLBodyElement()) {
            eventType = starFish()->staticStrings()->m_focusin.localName();
            FocusEventInit init;
            init.setBubbles(true);
            e = new FocusEvent(document(), eventType, init);
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

static TouchEvent* createTouchEvent(Document* document, String* eventName,
                                    TouchEventInit& init)
{
    TouchEvent* event = new TouchEvent(document, eventName, init);
    event->setBubbles(true);
    event->setCancelable(true);
    event->setView(document->window());
    return event;
}

static MouseEvent* createMouseEvent(Document* document, String* eventName,
                                    MouseEventInit& init)
{
    MouseEvent* event = new MouseEvent(document, eventName, init);
    event->setBubbles(true);
    event->setCancelable(true);
    event->setView(document->window());
    return event;
}

static bool isIFrameEvent(Node* targetNode)
{
    if (targetNode->isHTMLIFrameElement() &&
        targetNode->asHTMLIFrameElement()->browsingContext() &&
        targetNode->asHTMLIFrameElement()->frame()) {
        return true;
    }
    return false;
}

void BrowsingContext::dispatchTouchEvent(PlatformWindow::TouchEventKind kind,
                                         TouchEventInit& init)
{
    if (!m_isRunning) {
        return;
    }
    if (kind == PlatformWindow::TouchEventCancel) {
        releaseActiveNode();
        releaseHoveredNode();
        return;
    }
    // If there is no `Touch` information, return.
    if (init.touchInits().size() == 0) {
        return;
    }
    // Handle touch informations
    // - Do the hitTest for all `Touch` informations and set target for each.
    // - Decide representative target (= first non-empty target).
    // - Check whether touch position moved away from original position
    //   to release active nodes.
    bool checkRelease = kind == PlatformWindow::TouchEventMove &&
                        (starFish()->deviceKind() & deviceKindUseTouchScreen);
    Node* targetNode = nullptr;
    double targetX = 0;
    double targetY = 0;
    size_t len = init.touchInits().size();
    for (size_t i = 0; i < len; i++) {
        TouchInit& touchInit = init.touchInits()[i];
        Node* node = hitTest(touchInit.clientX(), touchInit.clientY());
        touchInit.setTarget(node);
        if (!targetNode) {
            targetNode = node;
            targetX = touchInit.clientX();
            targetY = touchInit.clientY();
        }
        if (checkRelease &&
            ((abs(m_touchDownPoint.x() - touchInit.clientX()) > 30) ||
             (abs(m_touchDownPoint.y() - touchInit.clientY()) > 30))) {
            releaseActiveNode();
            checkRelease = false;
        }
    }
    if (!targetNode) {
        return;
    }
    // Handle event inside iframe
    if (isIFrameEvent(targetNode)) {
        auto iframe = targetNode->asHTMLIFrameElement();
        auto absPoint = iframe->frame()->asFrameBox()->absolutePoint(
            document()->frame()->asFrameBox());
        // NOTE Pass only targetX/Y information to iframe.
        //      discard rest informations.
        TouchEventInit newInit;
        newInit.touchInits().emplace_back(targetX - (double)absPoint.x(),
                                          targetY - (double)absPoint.y());
        iframe->browsingContext()->dispatchTouchEvent(kind, newInit);
        return;
    }
    // Dispatch events
    String* eventName = String::emptyString;
    switch (kind) {
    case PlatformWindow::TouchEventStart: {
        m_touchDownPoint = Unit::Location(targetX, targetY);
        setActiveNode(targetNode);
        setFocusedNode(targetNode);
        // Dispatch touchstart event
        eventName = starFish()->staticStrings()->m_touchstart.localName();
        Event* e = createTouchEvent(document(), eventName, init);
        Node* t = m_activeNodes.size() > 0 ? m_activeNodes[0] : document();
        document()->window()->dispatchEvent(t, e);
        break;
    }
    case PlatformWindow::TouchEventMove: {
        // Dispatch touchmove event
        eventName = starFish()->staticStrings()->m_touchmove.localName();
        Event* e = createTouchEvent(document(), eventName, init);
        Node* t = targetNode->nearestParentElement();
        t = t ? t : document();
        document()->window()->dispatchEvent(t, e);
        break;
    }
    case PlatformWindow::TouchEventEnd: {
        Node* t = targetNode->nearestParentElement();
        bool check = false;
        if (m_activeNodes.size() > 0) {
            check = (t == m_activeNodes[0]);
        }
        if (check) {
            // Dispatch click event
            t = t ? t : document();
            eventName = starFish()->staticStrings()->m_click.localName();
            MouseEventInit clickInit(targetX, targetY);
            Event* click = createMouseEvent(document(), eventName, clickInit);
            document()->window()->dispatchEvent(t, click);

            // Dispatch touchend event
            eventName = starFish()->staticStrings()->m_touchend.localName();
            Event* touchend = createTouchEvent(document(), eventName, init);
            document()->window()->dispatchEvent(t, touchend);
        }
        releaseActiveNode();
        break;
    }
    default:
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

void BrowsingContext::dispatchMouseEvent(PlatformWindow::MouseEventKind kind,
                                         MouseEventInit& init)
{
    if (!m_isRunning) {
        return;
    }
    // MouseEventEnter/MouseEventOut are not supported yet
    if (kind >= PlatformWindow::MouseEventEnter) {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        return;
    }
    // Hit test to validate event position
    Node* targetNode = hitTest(init.clientX(), init.clientY());
    if (!targetNode) {
        return;
    }
    // Handle event inside iframe
    if (isIFrameEvent(targetNode)) {
        auto iframe = targetNode->asHTMLIFrameElement();
        auto absPoint = iframe->frame()->asFrameBox()->absolutePoint(
            document()->frame()->asFrameBox());
        MouseEventInit newInit(init.clientX() - (double)absPoint.x(),
                               init.clientY() - (double)absPoint.y());
        iframe->browsingContext()->dispatchMouseEvent(kind, newInit);
        return;
    }
    // Dispatch events
    String* eventName = String::emptyString;
    switch (kind) {
    case PlatformWindow::MouseEventDown: {
        m_touchDownPoint = Unit::Location(init.clientX(), init.clientY());
        setActiveNode(targetNode);
        setFocusedNode(targetNode);
        // Dispatch mousedown event
        eventName = starFish()->staticStrings()->m_mousedown.localName();
        Event* e = createMouseEvent(document(), eventName, init);
        Node* t = m_activeNodes.size() > 0 ? m_activeNodes[0] : document();
        document()->window()->dispatchEvent(t, e);
        break;
    }
    case PlatformWindow::MouseEventMove: {
        Node* t = targetNode->nearestParentElement();
        // Dispatch mouseover event if necessary
        bool check = true;
        if (m_hoveredNodes.size() > 0) {
            check = (t != m_hoveredNodes[0]);
        }
        t = t ? t : document();
        if (check) {
            releaseHoveredNode();
            setHoveredNode(targetNode);

            eventName = starFish()->staticStrings()->m_mouseover.localName();
            Event* e = createMouseEvent(document(), eventName, init);
            document()->window()->dispatchEvent(t, e);
        }
        // Dispatch mousemove event
        eventName = starFish()->staticStrings()->m_mousemove.localName();
        Event* e = createMouseEvent(document(), eventName, init);
        document()->window()->dispatchEvent(t, e);
        break;
    }
    case PlatformWindow::MouseEventUp: {
        // Check whether it is skippable or not
        Node* t = targetNode->nearestParentElement();
        bool check = false;
        if (m_activeNodes.size() > 0) {
            check = (t == m_activeNodes[0]);
        }
        if (check) {
            // Dispatch click event
            t = t ? t : document();
            eventName = starFish()->staticStrings()->m_click.localName();
            Event* click = createMouseEvent(document(), eventName, init);
            document()->window()->dispatchEvent(t, click);

            // Dispatch mouseup event
            eventName = starFish()->staticStrings()->m_mouseup.localName();
            Event* mouseup = createMouseEvent(document(), eventName, init);
            document()->window()->dispatchEvent(t, mouseup);
        }
        releaseActiveNode();
        break;
    }
    default:
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
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
    KeyboardEvent* e = new KeyboardEvent(document(), eventType);
    e->setBubbles(true);
    e->setCancelable(true);
    e->setKey(key);

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

    iterateChildContext([](BrowsingContext* ctx) { ctx->pause(); });
}

void BrowsingContext::resume()
{
    STARFISH_LOG_INFO("BrowsingContext::resume\n");
    if (m_isRunning) {
        setNeedsPainting();
        return;
    }

    m_isRunning = true;
    setNeedsPainting();

    document()->setVisibilityState(VisibilityState::VisibilityStateVisible);

    iterateChildContext([](BrowsingContext* ctx) { ctx->resume(); });
}

void BrowsingContext::setNeedsPainting()
{
    m_webView->setNeedsPainting();
}

void BrowsingContext::setNeedsComposite()
{
    m_webView->setNeedsComposite();
}

void BrowsingContext::setNeedsRendering()
{
    m_webView->setNeedsRendering();
}

void BrowsingContext::registerNeedsLayoutInWebView()
{
    if (isMainBrowsingContext())
        return;
    auto& v = m_webView->m_browsingContextsNeedsLayout;
    if (v.end() == std::find(v.begin(), v.end(), this)) {
        v.push_back(this);
    }
}
}
