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
#include "browser/history/HistoryManager.h"
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
#include "core/style/StyleRule.h"
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
    , m_activeNodeTarget(nullptr)
    , m_documentVersionWhenComputingActiveNodeSet(0)
    , m_hoveredNodeTarget(nullptr)
    , m_documentVersionWhenComputingHoveredNodeSet(0)
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

void BrowsingContext::navigate(ResourceURL* url, HistoryManager::Action type)
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

    switch (type) {
    case HistoryManager::Action::Add:
        webView()->historyManager()->push(url);
        break;
    case HistoryManager::Action::Replace:
        webView()->historyManager()->replace(url);
        break;
    case HistoryManager::Action::Intact:
    default:
        break;
    }

    m_window->document()->open();
}

struct NavigateData : public gc {
    ResourceURL* url;
    HistoryManager::Action type;
};

void BrowsingContext::navigateAsync(ResourceURL* url,
                                    HistoryManager::Action type)
{
    NavigateData* data = new NavigateData();
    data->url = url;
    data->type = type;
    starFish()->messageLoop()->addIdlerWithNoScriptInstanceEntering(
        this,
        [](size_t a, void* data, void* data2) {
            ((BrowsingContext*)data)
                ->navigate(((NavigateData*)data2)->url,
                           ((NavigateData*)data2)->type);
        },
        this, data);
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

            auto viewportDependentResult =
                &document()
                     ->styleResolver()
                     .viewportDependentMediaQueryResults();
            auto deviceDependentResult =
                &document()->styleResolver().deviceDependentMediaQueryResults();
            viewportDependentResult->clear();
            deviceDependentResult->clear();

            size_t sheets = document()->styleResolver().sheets().size();
            for (size_t i = 1; i < sheets; i++) {
                CSSStyleSheet* authorSheet =
                    document()->styleResolver().sheets()[i];

                authorSheet->parseSheetIfneeds();

                const MediaQueryEvaluator& evaluator =
                    document()->styleResolver().mediaQueryEvaluator();
                if (authorSheet->mediaQuerySet() &&
                    !authorSheet->matchesMediaQueries(
                        evaluator, authorSheet->mediaQuerySet(),
                        viewportDependentResult, deviceDependentResult)) {
                    continue;
                }

                authorSheet->clearStyleRules();
                authorSheet->collectRulesFromImportedSheet(
                    authorSheet->importRules(), viewportDependentResult,
                    deviceDependentResult);
                authorSheet->collectStyleRules(
                    authorSheet->childRules(), authorSheet->url(),
                    viewportDependentResult, deviceDependentResult);

                size_t rules = authorSheet->rules().size();
                for (size_t j = 0; j < rules; j++) {
                    authorSheet->rules()[j].first->setOrder(j);
                    document()
                        ->styleResolver()
                        .styleSheetWithStyleRules()
                        ->addToRuleSet(authorSheet->rules()[j]);
                }
            }
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
            if (!document()->rootElement()->body()) {
                return;
            }

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
    Traverse::collectDescendants(col, document(),
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

    m_activeNodeSet.clear();
    m_activeNodeTarget = nullptr;
    m_documentVersionWhenComputingActiveNodeSet = 0;

    m_hoveredNodeSet.clear();
    m_hoveredNodeTarget = nullptr;
    m_documentVersionWhenComputingHoveredNodeSet = 0;

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
    unRegisterNeedsLayoutInWebView();
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

    if (window() && document() && document()->frame()) {
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

static bool updateEventNodeSet(Document* document, Node* n,
                               GCUnorderedSet<Node*>& set, Node** target,
                               size_t* version, Node::NodeState state,
                               Node::DynamicRestyleFlags flag)
{
    Node* t = n->nearestParentElement();
    if (*target != n || set.find(t) != set.end() ||
        *version != document->domVersion()) {
        GCUnorderedSet<Node*> newSet;
        while (t) {
            newSet.insert(t);
            t = t->parentNode();
        }

        auto iter = set.begin();
        while (iter != set.end()) {
            // new(X) old(O)
            if (newSet.find(*iter) == newSet.end()) {
                (*iter)->setState(state, flag, false);
            }
            iter++;
        }

        iter = newSet.begin();
        while (iter != newSet.end()) {
            // new(O) old(X)
            if (set.find(*iter) == set.end()) {
                (*iter)->setState(state, flag, true);
            }
            iter++;
        }

        set = std::move(newSet);
        *target = n;
        *version = document->domVersion();
        return true;
    }
    return false;
}

bool BrowsingContext::setActiveNode(Node* n)
{
    return updateEventNodeSet(
        document(), n, m_activeNodeSet, &m_activeNodeTarget,
        &m_documentVersionWhenComputingActiveNodeSet, Node::NodeStateActive,
        Node::ChildrenOrSiblingsAffectedByActive);
}

void BrowsingContext::releaseActiveNode()
{
    if (!m_activeNodeTarget) {
        return;
    }

    auto iter = m_activeNodeSet.begin();
    while (iter != m_activeNodeSet.end()) {
        (*iter)->setState(Node::NodeStateActive,
                          Node::ChildrenOrSiblingsAffectedByActive, false);
        iter++;
    }
    m_activeNodeSet.clear();
    m_activeNodeTarget = nullptr;
    m_documentVersionWhenComputingActiveNodeSet = 0;
}

bool BrowsingContext::setHoveredNode(Node* n)
{
    return updateEventNodeSet(
        document(), n, m_hoveredNodeSet, &m_hoveredNodeTarget,
        &m_documentVersionWhenComputingHoveredNodeSet, Node::NodeStateHovered,
        Node::ChildrenOrSiblingsAffectedByHover);
}

void BrowsingContext::releaseHoveredNode()
{
    if (!m_hoveredNodeTarget) {
        return;
    }

    auto iter = m_hoveredNodeSet.begin();
    while (iter != m_hoveredNodeSet.end()) {
        (*iter)->setState(Node::NodeStateHovered,
                          Node::ChildrenOrSiblingsAffectedByHover, false);
        iter++;
    }
    m_hoveredNodeSet.clear();
    m_hoveredNodeTarget = nullptr;
    m_documentVersionWhenComputingHoveredNodeSet = 0;
}

static TouchEvent* createTouchEvent(Document* document, String* name,
                                    TouchData* touches, size_t count)
{
    TouchEvent* event = new TouchEvent(document, name, touches, count);
    event->setBubbles(true);
    event->setCancelable(true);
    event->setView(document->window());
    return event;
}

static MouseEvent* createMouseEvent(Document* document, String* name,
                                    MouseData& data)
{
    MouseEvent* event = new MouseEvent(document, name, data);
    event->setBubbles(true);
    event->setCancelable(true);
    event->setView(document->window());
    return event;
}

bool BrowsingContext::isInnerIFrameEvent(Node* targetNode, double& posX,
                                         double& posY)
{
    if (targetNode->isHTMLIFrameElement() &&
        targetNode->asHTMLIFrameElement()->browsingContext() &&
        targetNode->asHTMLIFrameElement()->frame()) {
        auto iframe = targetNode->asHTMLIFrameElement();
        auto fb = iframe->frame()->asFrameBox();
        auto absPoint = fb->absolutePoint(document()->frame()->asFrameBox());
        double newPosX = posX - (double)absPoint.x();
        double newPosY = posY - (double)absPoint.y();
        double contentX = (double)(fb->paddingLeft() + fb->borderLeft());
        double contentY = (double)(fb->paddingTop() + fb->borderTop());
        if (contentX <= newPosX && newPosX <= contentX + fb->contentWidth() &&
            contentY <= newPosY && newPosY <= contentY + fb->contentHeight()) {
            posX = newPosX;
            posY = newPosY;
            return true;
        }
    }
    return false;
}

void BrowsingContext::handleActiveAndFocus(PlatformWindow::MouseEventKind kind,
                                           Node* targetNode, double posX,
                                           double posY)
{
    if (kind == PlatformWindow::MouseEventDown) { // TouchEventStart
        m_touchDownPoint = Unit::Location(posX, posY);
        setActiveNode(targetNode);
        setFocusedNode(targetNode);
    } else if (kind == PlatformWindow::MouseEventUp) { // TouchEventEnd
        releaseActiveNode();
    }
}

void BrowsingContext::handleHover(PlatformWindow::MouseEventKind kind,
                                  Node* targetNode, unsigned char button,
                                  unsigned char buttons, double posX,
                                  double posY)
{
    if (kind != PlatformWindow::MouseEventMove) {
        return;
    }

    Node* oldTarget = m_hoveredNodeTarget;
    if (setHoveredNode(targetNode)) {
        Node* newTarget = m_hoveredNodeTarget;
        if (newTarget != oldTarget) {
            if (oldTarget) {
                String* name =
                    starFish()->staticStrings()->m_mouseout.localName();
                MouseData data(button, buttons, posX, posY);
                Event* e = createMouseEvent(document(), name, data);
                Node* t = oldTarget->nearestParentElement();
                t = t ? t : document();
                document()->window()->dispatchEvent(t ? t : document(), e);
            }
            String* name = starFish()->staticStrings()->m_mouseover.localName();
            MouseData data(button, buttons, posX, posY);
            Event* e = createMouseEvent(document(), name, data);
            Node* t = newTarget->nearestParentElement();
            t = t ? t : document();
            document()->window()->dispatchEvent(t ? t : document(), e);
        }
    }
}

void BrowsingContext::dispatchTouchEvent(PlatformWindow::TouchEventKind kind,
                                         TouchData* touches, size_t count)
{
    if (!m_isRunning) {
        return;
    }
    if (kind == PlatformWindow::TouchEventCancel) {
        releaseActiveNode();
        releaseHoveredNode();
        return;
    }
    if (count < 1) {
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
    for (size_t i = 0; i < count; i++) {
        TouchData& touchData = touches[i];
        Node* node = hitTest(touchData.clientX(), touchData.clientY());
        touchData.setTarget(node);
        if (!targetNode) {
            targetNode = node;
            targetX = touchData.clientX();
            targetY = touchData.clientY();
        }
        if (checkRelease &&
            ((abs(m_touchDownPoint.x() - touchData.clientX()) > 30) ||
             (abs(m_touchDownPoint.y() - touchData.clientY()) > 30))) {
            releaseActiveNode();
            checkRelease = false;
        }
    }
    if (!targetNode) {
        return;
    }
    // Handle event inside iframe
    double newX = targetX;
    double newY = targetY;
    if (isInnerIFrameEvent(targetNode, newX, newY)) {
        TouchData newData(newX, newY);
        targetNode->asHTMLIFrameElement()
            ->browsingContext()
            ->dispatchTouchEvent(kind, &newData, 1);
        handleActiveAndFocus((PlatformWindow::MouseEventKind)kind, targetNode,
                             targetX, targetY);
        return;
    }
    // Dispatch events
    String* name = String::emptyString;
    switch (kind) {
    case PlatformWindow::TouchEventStart: {
        // Dispatch touchstart event
        name = starFish()->staticStrings()->m_touchstart.localName();
        Event* e = createTouchEvent(document(), name, touches, count);
        Node* t = targetNode->nearestParentElement();
        t = t ? t : document();
        document()->window()->dispatchEvent(t, e);
        break;
    }
    case PlatformWindow::TouchEventMove: {
        // Dispatch touchmove event
        name = starFish()->staticStrings()->m_touchmove.localName();
        Event* e = createTouchEvent(document(), name, touches, count);
        Node* t = targetNode->nearestParentElement();
        t = t ? t : document();
        document()->window()->dispatchEvent(t, e);
        break;
    }
    case PlatformWindow::TouchEventEnd: {
        Node* t = targetNode->nearestParentElement();
        if (m_activeNodeTarget == targetNode) {
            // Dispatch click event
            Node* t = targetNode->nearestParentElement();
            t = t ? t : document();
            name = starFish()->staticStrings()->m_click.localName();
            MouseData clickData(MouseData::MouseButtonValue::LeftButton,
                                MouseData::MouseButtonsValue::LeftButtonDown,
                                targetX, targetY);
            Event* click = createMouseEvent(document(), name, clickData);
            document()->window()->dispatchEvent(t, click);
        }
        // Dispatch touchend event
        name = starFish()->staticStrings()->m_touchend.localName();
        Event* e = createTouchEvent(document(), name, touches, count);
        document()->window()->dispatchEvent(t, e);
        break;
    }
    default:
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    // Handle properties
    handleActiveAndFocus((PlatformWindow::MouseEventKind)kind, targetNode,
                         targetX, targetY);
}

void BrowsingContext::dispatchMouseEvent(PlatformWindow::MouseEventKind kind,
                                         MouseData& data)
{
    if (!m_isRunning) {
        return;
    }
    // MouseEventEnter/MouseEventOut are not supported yet
    if (kind >= PlatformWindow::MouseEventEnter) {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        return;
    }

    // STARFISH_LOG_INFO("BrowsingContext::dispatchMouseEvent %d %f %f\n",
    // (int)kind, data.clientX(), data.clientY());

    // Hit test to validate event position
    Node* targetNode = hitTest((float)data.clientX(), (float)data.clientY());
    if (!targetNode) {
        return;
    }
    double targetX = data.clientX();
    double targetY = data.clientY();
    double newX = targetX;
    double newY = targetY;

    // Handle event inside iframe
    if (isInnerIFrameEvent(targetNode, newX, newY)) {
        MouseData newData(data.button(), data.buttons(), newX, newY);
        targetNode->asHTMLIFrameElement()
            ->browsingContext()
            ->dispatchMouseEvent(kind, newData);
        handleActiveAndFocus(kind, targetNode, targetX, targetY);
        handleHover(kind, targetNode, data.button(), data.buttons(), targetX,
                    targetY);
        return;
    }
    // Dispatch events
    String* name = String::emptyString;
    switch (kind) {
    case PlatformWindow::MouseEventDown: {
        Node* t = targetNode->nearestParentElement();
        t = t ? t : document();
        // Dispatch mousedown event
        name = starFish()->staticStrings()->m_mousedown.localName();
        Event* e = createMouseEvent(document(), name, data);
        document()->window()->dispatchEvent(t, e);
        break;
    }
    case PlatformWindow::MouseEventMove: {
        Node* t = targetNode->nearestParentElement();
        t = t ? t : document();
        // Dispatch mousemove event
        name = starFish()->staticStrings()->m_mousemove.localName();
        Event* e = createMouseEvent(document(), name, data);
        document()->window()->dispatchEvent(t, e);
        break;
    }
    case PlatformWindow::MouseEventUp: {
        // Check whether it is skippable or not
        Node* t = targetNode->nearestParentElement();
        t = t ? t : document();
        // Dispatch mouseup event
        name = starFish()->staticStrings()->m_mouseup.localName();
        Event* mouseup = createMouseEvent(document(), name, data);
        document()->window()->dispatchEvent(t, mouseup);

        if (m_activeNodeTarget == targetNode) {
            // Dispatch click event
            name = starFish()->staticStrings()->m_click.localName();
            Event* click = createMouseEvent(document(), name, data);
            document()->window()->dispatchEvent(t, click);
        }
        break;
    }
    default:
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    // Handle properties
    handleActiveAndFocus(kind, targetNode, targetX, targetY);
    handleHover(kind, targetNode, data.button(), data.buttons(), targetX,
                targetY);
}

void BrowsingContext::dispatchKeyEvent(PlatformWindow::KeyEventKind kind,
                                       KeyboardData& data)
{
    // Set target
    // 1) currently focused element if possible
    // or 2) body element if possible
    // or 3) root element
    EventTarget* target = m_focusedNode;
    if (!target && document()->body()) {
        target = document()->body();
    }
    if (!target && document()->rootElement()) {
        target = document()->rootElement();
    }
    if (!target) {
        return;
    }
    // Dispatch event
    String* eventType = String::emptyString;
    if (kind == PlatformWindow::KeyEventKind::KeyEventUp) {
        eventType = starFish()->staticStrings()->m_keyup.localName();
    } else {
        // kind == KeyEventKind::KeyEventDown
        eventType = starFish()->staticStrings()->m_keydown.localName();
    }
    KeyboardEvent* e = new KeyboardEvent(document(), eventType, data);
    e->setBubbles(true);
    e->setCancelable(true);
    e->setView(document()->window());
    document()->window()->dispatchEvent(target, e);
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
    if (isMainBrowsingContext()) {
        return;
    }

    auto& v = m_webView->m_browsingContextsNeedsLayout;
    if (v.end() == std::find(v.begin(), v.end(), this)) {
        v.push_back(this);
    }
}

void BrowsingContext::unRegisterNeedsLayoutInWebView()
{
    if (isMainBrowsingContext()) {
        return;
    }
    auto& v = m_webView->m_browsingContextsNeedsLayout;
    auto iter = std::find(v.begin(), v.end(), this);
    if (iter != v.end()) {
        v.erase(iter);
    }
}
}
