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

#include <SkMatrix.h>

#include "StarfishConfig.h"
#include "Starfish.h"

#include "BrowsingContext.h"
#include "WebView.h"

#include "core/dom/Document.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/FocusEvent.h"
#include "browser/history/HistoryManager.h"
#include "core/dom/HTMLDocument.h"
#include "core/dom/HTMLBodyElement.h"
#include "core/dom/HTMLCollection.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/dom/HTMLInputElement.h"
#include "core/dom/HTMLDialogElement.h"
#include "core/dom/HTMLImageElement.h"
#include "core/dom/HTMLMapElement.h"
#ifdef STARFISH_ENABLE_MULTIMEDIA
#include "core/dom/HTMLMediaElement.h"
#endif
#include "core/dom/EventTarget.h"
#include "core/dom/MouseEvent.h"
#include "core/dom/KeyboardEvent.h"
#include "core/dom/PointerEvent.h"
#include "platform/event/PlatformKeyEventData.h"
#include "core/dom/TouchEvent.h"
#include "core/dom/CompositionEvent.h"
#include "core/dom/Scrolling.h"
#include "core/page/Location.h"
#include "core/page/Window.h"
#include "core/style/CSSStyleSheet.h"
#include "core/style/CSSStyleDeclaration.h"
#include "core/style/FontFaceSrcData.h"
#include "core/style/StyleRule.h"
#include "core/layout/Frame.h"
#include "core/layout/FrameBlockBox.h"
#include "core/layout/FrameDocument.h"
#include "core/layout/FrameTreeBuilder.h"
#include "core/layout/StackingContext.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/Compositor.h"
#include "core/modules/message_loop/Timer.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/threading/Thread.h"
#include "core/util/URL.h"
#include "core/modules/renderer/Renderer.h"
#include "core/animation/AnimationTask.h"
#include "core/animation/AnimationExecutor.h"
#include "core/dom/Traverse.h"
#include "core/dom/HTMLIFrameElement.h"
#include "core/dom/InputEvent.h"
#include "core/dom/svg/SVGAnimationElement.h"
#include "binding/ScriptBindingInstance.h"
#include "platform/loader/ResourceLoader.h"

// Matches Chrome/Android ViewConfiguration.getTouchSlop() default of 8dp.
// Coordinates here are already DPR-divided CSS pixels, so this value is
// device-independent. Increase if jitter still leaks; decrease if short
// swipes are not recognized as moves.
#ifndef STARFISH_TOUCH_SLOP_PX
// Chrome/Android phone default is 8dp; TV touch panels have coarser precision
// so the default is raised to 20px (override via -DSTARFISH_TOUCH_SLOP_PX=N).
#define STARFISH_TOUCH_SLOP_PX 20.0
#endif

namespace Starfish {

BrowsingContext* BrowsingContext::create(WebView* webView)
{
    STARFISH_ASSERT(webView);
    return new BrowsingContext(webView);
}

bool BrowsingContext::isScriptingEnabled()
{
#if defined(STARFISH_ENABLE_CDP)
    if (m_webView && m_webView->scriptExecutionDisabledByCDP()) {
        return false;
    }
#endif
    return m_isScriptingEnabled;
}

BrowsingContext* BrowsingContext::create(HTMLIFrameElement* sourceElement,
                                         bool isScriptingEnabled)
{
    return new BrowsingContext(sourceElement->webView(), sourceElement,
                               isScriptingEnabled);
}

BrowsingContext::BrowsingContext(WebView* webView, HTMLIFrameElement* source,
                                 bool isScriptingEnabled)
    : WebViewHoldable(webView)
    , m_webView(webView)
    , m_window(nullptr)
    , m_parentBrowsingContext(source ? source->document()->browsingContext()
                                     : nullptr)
    , m_sourceElement(source)
    , m_isScriptingEnabled(isScriptingEnabled)
    , m_pendingStyleSheetCount(0)
    , m_pendingRenderingCount(0)
    , m_touchDownPoint(0, 0)
    , m_touchSlopExceeded(false)
    , m_lastMouseMovePoint(std::numeric_limits<float>::quiet_NaN(),
                           std::numeric_limits<float>::quiet_NaN())
    , m_activeNodeTarget(nullptr)
    , m_documentVersionWhenComputingActiveNodeSet(0)
    , m_pointerCaptureTarget(nullptr)
    , m_hoveredNodeTarget(nullptr)
    , m_documentVersionWhenComputingHoveredNodeSet(0)
    , m_focusedNode(nullptr)
    , m_activeElement(nullptr)
    , m_name(String::emptyString)
    , m_styleResolveStartTick(0)
{
    initFlags();
}

void BrowsingContext::initFlags()
{
    m_needsStyleRecalc = false;
    m_styleRecalcRenderingSkipped = false;
    m_needsStyleRecalcForWholeDocument = false;
    m_needsStyleSheetsRecalc = true;
    m_needsFrameTreeBuild = false;
    m_needsLayout = false;

    m_keydownEventDefaultPrevented = false;
    m_compositionStartEventDefeaultPrevented = false;

    m_hasRootElementBackground = false;
    m_hasBodyElementBackground = false;
    m_pendingStyleSheetCount = 0;
}

ScriptBindingInstance* BrowsingContext::scriptBindingInstance()
{
    return window()->scriptBindingInstance();
}

void BrowsingContext::open(ResourceURL* url, HistoryManagerAction type,
                           ReferrerURL* referrerURL)
{
    initFlags();

    if (isTopLevelBrowsingContext()) {
        m_window = Window::create(this, url,
                                  webView()->renderer()->width() /
                                      webView()->screenInfo().devicePixelRatio,
                                  webView()->renderer()->height() /
                                      webView()->screenInfo().devicePixelRatio);
    } else {
        if (m_sourceElement->frame()) {
            m_window = Window::create(this, url,
                                      (uint32_t)m_sourceElement->frame()
                                          ->asFrameBox()
                                          ->contentWidth(),
                                      (uint32_t)m_sourceElement->frame()
                                          ->asFrameBox()
                                          ->contentHeight());
        } else {
            m_window = Window::create(this, url, STARFISH_DEFAULT_IFRAME_WIDTH,
                                      STARFISH_DEFAULT_IFRAME_HEIGHT);
        }
        m_window->document()->executionContext()->initContentSecurityPolicy(
            m_sourceElement->document()->contentSecurityPolicy());
    }

    if (webView()->useSpatialNavigation()) {
        constexpr char spatialNavagationJS[] =
#include "core/page/spatial-navigation-polyfill.js"
            ;
        webView()->evaluateJavaScript(
            String::createASCIIStringWithNoCopy(spatialNavagationJS));
    }

    m_window->document()->init(referrerURL);
    m_window->performance()->timing()->m_requestStart = timestamp();

    // STARFISH_LOG_INFO("BrowsingContext::open %s",
    // url->urlString()->toUTF8String().data());

    switch (type) {
    case HistoryManagerAction::Add:
        historyManager()->push(document(), url);
        break;
    case HistoryManagerAction::Replace:
        historyManager()->replace(document(), url);
        break;
    case HistoryManagerAction::Intact:
    default:
        break;
    }
}

HistoryManager* BrowsingContext::historyManager()
{
    if (isTopLevelBrowsingContext()) {
        return webView()->historyManager();
    } else {
        return m_sourceElement->m_historyManager;
    }
}

Document* BrowsingContext::document()
{
    return window()->document();
}

void BrowsingContext::resolveStyleIfNeeds()
{
    if (m_needsStyleRecalc || m_needsStyleRecalcForWholeDocument) {
        if (m_needsStyleSheetsRecalc) {
            INSTALL_PROFILE_TIMER("parse sheet & collect rules");

            m_needsStyleSheetsRecalc = false;
            document()->styleResolver().recalcRuleSetIfNeeds();

            Traverse::traverseIncludingShadowDOM(document(), [](Node* node) {
                if (node->isShadowRoot()) {
                    node->asShadowRoot()
                        ->styleResolver()
                        .recalcRuleSetIfNeeds();
                }
            });
        }

        // execute pending svg animation
        {
            auto s(std::move(
                document()->m_svgAnimateElementsNeedExecuteAnimation));
            for (auto e : s) {
                e->beginElementAt(0);
            }
        }

        // resolve style
        INSTALL_PROFILE_TIMER("resolve style");

        m_styleResolveStartTick = tickCount();
        document()->styleResolver().resolveDOMStyle(
            document(), m_needsStyleRecalcForWholeDocument);
        m_needsStyleRecalc = false;
        m_styleRecalcRenderingSkipped = false;
        m_needsStyleRecalcForWholeDocument = false;

        if (document()->animationExecutor()->activeTransitions().size() > 0) {
            auto& l = document()->animationExecutor()->activeTransitions();
            uint64_t currentTick = tickCount();
            bool canceled = false;
            for (size_t i = 0; i < l.size(); i++) {
                if ((l[i]->targetElement()->isInDocumentScope() == false) ||
                    (l[i]->targetElement()->style() == nullptr) ||
                    l[i]->targetElement()->style()->display() ==
                        DisplayValue::NoneDisplayValue) {
                    canceled = true;
                    l[i]->fireTransitionCancelEvent();
                    l[i]->detachFromElement();
                    l.erase(i);
                    i--;
                }
            }

            if (canceled == true) {
                webView()->updateActiveAnimationExecutorRegistration(
                    document()->animationExecutor());
            }

            if (webView()->inRendering() == false) {
                webView()->setNeedsRendering();
            }
        }

        if (document()->animationExecutor()->activeAnimations().size() > 0) {
            auto& animations =
                document()->animationExecutor()->activeAnimations();
            for (auto animationIter = animations.begin();
                 animationIter != animations.end(); animationIter++) {
                auto& animation = animationIter.value();
                uint64_t currentTick = tickCount();
                uint64_t cancelTick = 0;
                bool canceled = false;
                for (auto task = animation.begin(); task != animation.end();) {
                    Element* targetElement = (*task)->targetElement();
                    bool nullComputedStyleOrDisplayNone =
                        !targetElement->style() ||
                        targetElement->style()->display() ==
                            DisplayValue::NoneDisplayValue;

                    if (!targetElement->isInDocumentScope() ||
                        nullComputedStyleOrDisplayNone) {
                        if (nullComputedStyleOrDisplayNone &&
                            (*task)->animationType() ==
                                AnimationType::SVGAnimation) {
                            // SVG animation must be played even if the display
                            // is none if already started.
                            task++;
                            continue;
                        }
                        canceled = true;
                        (*task)->detachFromElement();
                        double progress = (*task)->fraction(currentTick);
                        cancelTick = (*task)->duration() * progress / 1000;
                        task = animation.erase(task);
                    } else {
                        task++;
                    }
                }
                if (canceled) {
                    ActiveElementAnimation* activeElementAnimation =
                        animationIter.key();
                    AnimationExecutor* animationExecutor =
                        document()->animationExecutor();
                    animationExecutor->fireKeyFramesAnimationEvent(
                        KeyFramesAnimationEventType::AnimationCancel,
                        activeElementAnimation->element(),
                        activeElementAnimation->name(), cancelTick);

                    webView()->updateActiveAnimationExecutorRegistration(
                        animationExecutor);
                }
                if (webView()->inRendering() == false) {
                    webView()->setNeedsRendering();
                }
            }
        }
    }
}

void BrowsingContext::buildFrameTreeIfNeeds()
{
    resolveStyleIfNeeds();
    if (m_needsFrameTreeBuild) {
        if (document()->frame()) {
            // create frame tree
            INSTALL_PROFILE_TIMER("create frame tree");

            FrameTreeBuilder::buildFrameTree(document());
            m_needsLayout = true;
            m_needsFrameTreeBuild = false;
            // A frame tree rebuild replaces FrameBox objects, so any existing
            // StackingContext tree now points at stale frames. Request a full
            // SC re-establish here (the rebuild site) rather than in
            // layoutIfNeeded, because getComputedStyle can trigger this
            // rebuild outside the layout path.
            webView()->setNeedsEstablishesStackingContext();
        }
    }
}

void BrowsingContext::computeLayoutPaintingDirty()
{
    INSTALL_PROFILE_TIMER("trace repaint region");

    bool gotPaintingDirty =
        m_layoutRepaintTracker.traceRepaintRegion(document()
                                                      ->frame()
                                                      ->asFrameBox()
                                                      ->asFrameBlockBox()
                                                      ->asFrameDocument());
    if (gotPaintingDirty) {
        setNeedsPainting();
    }
}

bool BrowsingContext::layoutIfNeeded()
{
    buildFrameTreeIfNeeds();

    bool ret = false;
    if (m_needsLayout) {
        // layout frame tree
        INSTALL_RECORDABLE_PROFILE_TIMER(ProfileKind::kLayout,
                                         "layout frame tree");

        LayoutContext ctx(starfish(), document()
                                          ->frame()
                                          ->asFrameBox()
                                          ->asFrameBlockBox()
                                          ->asFrameDocument());

        document()->frame()->layout(ctx,
                                    Frame::LayoutWantToResolve::ResolveAll);

        // compute scroll width & height of each FrameBlockBox if need
        document()->frame()->asFrameBlockBox()->computeScrollRectIfNeeded();
        document()->frame()->asFrameBox()->iterateChildFrameBox(
            [](FrameBox* fb) {
                if (fb->isFrameBlockBox()) {
                    fb->asFrameBlockBox()->computeScrollRectIfNeeded();
                }
            });

        registerDidLayoutInWebView();

        // A pure-geometry layout cannot change which boxes establish stacking
        // contexts: SC establishment is style-derived and re-requested via
        // ComputedStyleDamageEstablishesStackingContext, and frame tree
        // rebuilds request it in buildFrameTreeIfNeeds. Skipping the full SC
        // clear+rebuild here leaves only the property recompute per layout.
        static bool scEstGate = getenv("STARFISH_SC_EST_GATE") &&
                                *getenv("STARFISH_SC_EST_GATE") == '1';
        if (!scEstGate) {
            webView()->setNeedsEstablishesStackingContext();
        }
        webView()->setNeedsComputeStackingContextProperties();

        m_needsLayout = false;
        ret = true;
    }

    if (document()->animationExecutor()->activeAnimations().size() != 0) {
        auto& activeAnimations =
            document()->animationExecutor()->activeAnimations();
        auto iter = activeAnimations.begin();
        while (iter != activeAnimations.end()) {
            ActiveElementAnimation* activeElementAnimation = iter.key();
            GCVector<ActiveAnimationTask*>& animationTasks = iter.value();
            for (size_t i = 0; i < animationTasks.size(); i++) {
                ActiveAnimationTask* task = animationTasks[i];
                task->setIsForward(
                    activeElementAnimation->isForwardDirection(task));
                task->resolveUnresolvedAnimatedValues();
            }
            iter++;
        }
    }

    if (!webView()->hasActiveAnimationExecutor()) {
        document()->resourceLoader().cachePruning();
    }

    return ret;
}

template <typename T>
void BrowsingContext::clearingBeforePaint(T canvas)
{
    if (document()->browsingContext()->isTopLevelBrowsingContext()) {
        canvas->clearColor(webView()->baseBackgroundColor());
#ifdef STARFISH_TIZEN
        if (document()->tizenWidgetTransparentBackground()) {
            canvas->clearColor(Unit::Color(0, 0, 0, 0));
        }
#endif
    }
}

void BrowsingContext::paintWindowBackground(Canvas* canvas)
{
    if (!document()->rootElement()) {
        return;
    }

    if (m_hasRootElementBackground || m_hasBodyElementBackground) {
        if (m_hasRootElementBackground) {
            HTMLHtmlElement* root = document()->rootElement();
            FrameBox::paintBackground(canvas, nullptr, root);
        } else {
            HTMLBodyElement* body = document()->rootElement()->body();
            if (!body) {
                return;
            }

            FrameBox::paintBackground(canvas, nullptr, body);
        }
    }
}

std::pair<Optional<Element*>, Unit::Color>
BrowsingContext::hasWindowBackgroundColor()
{
    if (hasRootElementBackground() || hasBodyElementBackground()) {
        if (hasRootElementBackground() && !document()
                                               ->rootElement()
                                               ->style()
                                               ->backgroundColor()
                                               .isTransparent()) {
            HTMLHtmlElement* root = document()->rootElement();
            return std::make_pair(root, root->style()->backgroundColor());
        } else {
            HTMLBodyElement* body = document()->rootElement()->body();
            if (body && !body->style()->backgroundColor().isTransparent()) {
                return std::make_pair(body, body->style()->backgroundColor());
            }
        }
    }
    return std::make_pair(nullptr, Unit::Color());
}

bool BrowsingContext::rootStackingContextNeedsGraphicsBuffer()
{
    Document* domDocument = document();
    if (domDocument->rootElement() && domDocument->rootElement()->frame() &&
        domDocument->rootElement()->frame()->isFrameBlockBox() &&
        domDocument->rootElement()->frame()->asFrameBox()->stackingContext() &&
        domDocument->rootElement()
            ->frame()
            ->asFrameBox()
            ->stackingContext()
            ->needsGraphicsBuffer()) {
        return true;
    }
    return false;
}

template void BrowsingContext::clearingBeforePaint<Canvas*>(Canvas*);
template void BrowsingContext::clearingBeforePaint<Compositor*>(Compositor*);

void BrowsingContext::markHasPendingStyleSheet()
{
    // STARFISH_LOG_INFO("Window::markHasPendingStyleSheet");
    m_pendingStyleSheetCount++;
}

void BrowsingContext::unmarkHasPendingStyleSheet()
{
    // STARFISH_LOG_INFO("Window::unmarkHasPendingStyleSheet");
    if (m_pendingStyleSheetCount > 0) {
        m_pendingStyleSheetCount--;
        setNeedsRendering();
    }
}

void BrowsingContext::iterateChildContext(
    const std::function<void(BrowsingContext*)>& fn)
{
    if (document()) {
        GCVector<Element*> col;
        Traverse::collectDescendants(
            col, document(),
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
}

#ifdef STARFISH_ENABLE_MULTIMEDIA
void BrowsingContext::registerMediaElement(HTMLMediaElement* element)
{
    m_existingMediaElements.push_back(element);
}
#endif

void BrowsingContext::onIdle()
{
    if (document()) {
        document()->onIdle();
    }

    iterateChildContext([](BrowsingContext* ctx) { ctx->onIdle(); });
}

void BrowsingContext::dispose()
{
#ifdef STARFISH_ENABLE_MULTIMEDIA
    for (size_t i = 0; i < m_existingMediaElements.size(); i++) {
        m_existingMediaElements[i]->dispose();
    }
    m_existingMediaElements.clear();
#endif

    m_focusedNode = nullptr;
    m_activeElement = nullptr;

    m_activeNodeSet.clear();
    m_activeNodeTarget = nullptr;
    m_pointerCaptureTarget = nullptr;
    m_documentVersionWhenComputingActiveNodeSet = 0;

    m_hoveredNodeSet.clear();
    m_hoveredNodeTarget = nullptr;
    m_documentVersionWhenComputingHoveredNodeSet = 0;

    m_layoutRepaintTracker.dispose();

    auto& activeScrollingSet = webView()->activeScrollingSet();
    auto iter = activeScrollingSet.begin();
    while (iter != activeScrollingSet.end()) {
        if ((*iter)->target()->executionContext()->document() == document()) {
            iter = activeScrollingSet.erase(iter);
        } else {
            iter++;
        }
    }

    webView()->timer()->clear(m_window);

    if (m_window) {
        m_window->dispose();
    }

    if (isTopLevelBrowsingContext()) {
        auto& prevDrawnInfo = webView()->prevDrawnStackingContextInfo();
        auto iter = prevDrawnInfo.begin();
        while (iter != prevDrawnInfo.end()) {
            if (iter->second.graphicsBufferHolder) {
                iter->second.graphicsBufferHolder->detachNativeBuffers();
                iter.value().graphicsBufferHolder = nullptr;
            }
            iter++;
        }
        prevDrawnInfo.clear();

        webView()->renderer()->clearResources();
        m_webView->initRenderingFlags();
    } else {
        webView()->messageLoop()->clearPendingIdlers(m_window);
    }
    unregisterNeedsLayoutInWebView();
    unregisterDidLayoutInWebView();
}

void BrowsingContext::setWholeDocumentNeedsStyleRecalc()
{
    m_needsStyleRecalcForWholeDocument = true;
    setNeedsRendering();
    registerNeedsLayoutInWebView();
}

void BrowsingContext::setNeedsStyleSheetsRecalc()
{
    m_needsStyleSheetsRecalc = true;
    setNeedsRendering();
    registerNeedsLayoutInWebView();
}

void BrowsingContext::
    setNeedsStyleSheetsRecalcAndWholeDocumentNeedsStyleRecalc()
{
    m_needsStyleSheetsRecalc = true;
    m_needsStyleRecalcForWholeDocument = true;
    setNeedsRendering();
    registerNeedsLayoutInWebView();
}

void BrowsingContext::updateDefaultFontSize()
{
    auto doc = document();
    doc->styleResolver().m_mediumFontSize = webView()->defaultFontSize();
    doc->setStyle(doc->styleResolver().resolveDocumentStyle(doc));
    document()->styleResolver().setNeedsRecalcRuleSet();
    Traverse::traverseIncludingShadowDOM(document(), [&](Node* nd) {
        if (nd->isShadowRoot()) {
            nd->styleResolver().setNeedsRecalcRuleSet();
        }
    });
    setNeedsStyleSheetsRecalcAndWholeDocumentNeedsStyleRecalc();

    iterateChildContext(
        [](BrowsingContext* ctx) { ctx->updateDefaultFontSize(); });
}

Node* BrowsingContext::hitTest(float x, float y)
{
    webView()->layoutIfNeeded();

    if (window() && document() && document()->frame()) {
        Frame* frame = document()->frame()->hitTest(x, y, HitTestStageEnd);

        if (!frame) {
            return nullptr;
        }

        while (frame->isAnonymous()) {
            frame = frame->parent();
        }

        Node* imageArea = imageAreaForImage(frame, x, y);
#ifdef STARFISH_ENABLE_TEST
        if (webView()->startUpFlag() & StarfishStartUpFlag::enableHitTestDump) {
            printf("hitTest Result-> ");
            imageArea ? imageArea->dump() : frame->node()->dump();
            puts("");
        }
#endif
        return imageArea ? imageArea : frame->node();
    }

    return nullptr;
}

Node* BrowsingContext::focusedNode()
{
    return m_focusedNode;
}

void BrowsingContext::didFocusEvent()
{
#if defined(STARFISH_ENABLE_BODY_FOCUS_RING)
    {
        auto html = document()->html();
        if (html) {
            html->setNeedsStyleRecalc();
        }
    }
#endif
}

static void releaseFocusFN(BrowsingContext* ctx)
{
    ctx->releaseFocusedNode(nullptr, true);
    ctx->iterateChildContext(releaseFocusFN);
}

Node* BrowsingContext::imageAreaForImage(Frame* cb, float x, float y)
{
    STARFISH_ASSERT(cb && cb->node());

    if (!cb->node()->isHTMLImageElement() || !cb->isFrameBox()) {
        return nullptr;
    }

    HTMLMapElement* map =
        document()->imageMapElement(cb->node()->asHTMLImageElement()->usemap());
    if (!map) {
        return nullptr;
    }

    // x/y are page coordinates (see hitTest), so the image's offset must
    // reflect scrolled ancestors; absolutePoint() ignores ancestor scroll
    // offsets and would shift the area lookup for an image inside a scrolled
    // container. The document's own scroll is already part of the incoming
    // coordinates, so it must not be subtracted again.
    LayoutLocation l = cb->asFrameBox()->absolutePointIncludingScroll(
        document()->frame()->asFrameBox(), false);
    float newX = x - l.x().toFloat();
    float newY = y - l.y().toFloat();

    return map->areaIncludingPoint(cb, newX, newY);
}

// https://www.w3.org/TR/html5/editing.html#focusing-steps
void BrowsingContext::setFocusedNode(Node* n, bool byMouseEvent)
{
    // The focusing steps for an object new focus target that is either a
    // focusable area, or an element that is not a focusable area, or a browsing
    // context, are as follows. They can optionally be run with a fallback
    // target
    // and a string focus trigger.

    didFocusEvent();

    if (!n->isInDocumentScope() || !n->document()->browsingContext()) {
        return;
    }

    if (document()->isInertNode(n)) {
        return;
    }

    Element* e = n->isElement() ? n->asElement() : n->parentElement();

    if (!e) {
        // If document area is selected.
        releaseFocusedNode(nullptr);
        return;
    }
    if (e == m_focusedNode || !e->isFocusable()) {
        // If the element already has or can't get focus.
        if (e != m_focusedNode && !e->isFocusable() && byMouseEvent) {
            releaseFocusedNode(nullptr);
        }
        return;
    }

    {
        BrowsingContext* topBC = this;
        if (!isTopLevelBrowsingContext()) {
            topBC = parentBrowsingContext();
            while (!topBC->isTopLevelBrowsingContext()) {
                topBC = topBC->parentBrowsingContext();
            }
        }
        topBC->iterateChildContext(releaseFocusFN);

        //    3. If new focus target is a browsing context container with
        //    non-null
        //    nested browsing context, then set new focus target to the nested
        //    browsing context's active document, and redo this step.
        if (!isTopLevelBrowsingContext()) {
            BrowsingContext* bc = parentBrowsingContext();
            HTMLIFrameElement* focusTarget = sourceElement();
            while (bc && focusTarget) {
                bc->setFocusedNode(focusTarget->asNode(), byMouseEvent);
                bc = bc->parentBrowsingContext();
                focusTarget = nullptr;
                if (bc && !bc->isTopLevelBrowsingContext()) {
                    focusTarget = bc->sourceElement();
                }
            }
        }
    }

    if (e->isHTMLIFrameElement()) {
        // When a child browsing context is focused, its browsing context
        // container is also focused. For example, if the user moves the focus
        // to a text field in an iframe, the iframe is the element with focus in
        // the parent browsing context.
        // If an iframe is selected, the active element of this browsing context
        // should be the iframe element.
        releaseFocusedNode(nullptr);
        m_focusedNode = e->asNode();
        m_activeElement = e;

        if (m_focusedNode->asHTMLIFrameElement()->browsingContext()) {
            m_focusedNode->asHTMLIFrameElement()
                ->browsingContext()
                ->releaseFocusedNode(nullptr);
        }
        return;
    } else if (e->isHTMLBodyElement() || !e->isFocusable()) {
        // If the body or non-focusable elements are selected.
        releaseFocusedNode(nullptr);
        return;
    }
    // Set the related target for the focus/fucusin events.
    Node* relatedTarget = m_focusedNode && m_focusedNode->isHTMLIFrameElement()
                              ? nullptr
                              : m_focusedNode;

    // Run the unfocusing steps for this element.
    releaseFocusedNode(e);

    m_focusedNode = e->asNode();
    m_activeElement = e;

    e->setState(Node::NodeStateFocused, true);

    // focus event
    String* eventType = starfish()->staticStrings()->m_focus.localName();
    Event* event = new FocusEvent(document()->executionContext(), eventType,
                                  FocusEventInit(false, false, relatedTarget));
    document()->dispatchEventByUA(e->asNode(), event);

    // focusin event
    eventType = starfish()->staticStrings()->m_focusin.localName();
    event = new FocusEvent(document()->executionContext(), eventType,
                           FocusEventInit(true, false, relatedTarget));
    document()->dispatchEventByUA(e->asNode(), event);
}

// https://www.w3.org/TR/html5/editing.html#unfocusing-steps
void BrowsingContext::releaseFocusedNode(Node* n, bool resetActiveElement)
{
    didFocusEvent();

    if (m_focusedNode) {
        if (m_focusedNode->isHTMLIFrameElement()) {
            auto childBrowsingContext =
                m_focusedNode->asHTMLIFrameElement()->browsingContext();
            if (childBrowsingContext) {
                childBrowsingContext->releaseFocusedNode(nullptr, false);
            }
            m_focusedNode = nullptr;
            // active element
            if (resetActiveElement) {
                m_activeElement = nullptr;
            }
            return;
        } else if (m_focusedNode->isHTMLInputElement()) {
            m_focusedNode->asElement()
                ->ensureRareElementMembers()
                ->m_scrollLeft = 0;
        }

        m_focusedNode->setState(Node::NodeStateFocused, false);

        Node* relatedTarget = n == m_focusedNode ? nullptr : n;

        // blur event
        String* eventType = starfish()->staticStrings()->m_blur.localName();
        Event* event =
            new FocusEvent(document()->executionContext(), eventType,
                           FocusEventInit(false, false, relatedTarget));
        document()->dispatchEventByUA(m_focusedNode, event);

        // focusout event
        eventType = starfish()->staticStrings()->m_focusout.localName();
        event = new FocusEvent(document()->executionContext(), eventType,
                               FocusEventInit(true, false, relatedTarget));
        document()->dispatchEventByUA(m_focusedNode, event);

        m_focusedNode = nullptr;
    }

    // active element
    if (resetActiveElement) {
        m_activeElement = nullptr;
    }
}

Element* BrowsingContext::activeElement()
{
    return m_activeElement;
}

static bool updateEventNodeSet(Document* document, Node* n,
                               GCUnorderedSet<Node*>& set,
                               Optional<GCUnorderedSet<Node*>*> oldSet,
                               Node** target, size_t* version,
                               Node::NodeState state)
{
    Node* t = n->nearestParentElement();
    if (*target != n || set.find(t) != set.end() ||
        *version != document->domVersion()) {
        GCUnorderedSet<Node*> newSet;
        while (t) {
            newSet.insert(t);
            t = t->renderingParentNode();
        }

        auto iter = set.begin();
        while (iter != set.end()) {
            // new(X) old(O)
            if (newSet.find(*iter) == newSet.end()) {
                (*iter)->setState(state, false);
            }
            iter++;
        }

        iter = newSet.begin();
        while (iter != newSet.end()) {
            // new(O) old(X)
            if (set.find(*iter) == set.end()) {
                (*iter)->setState(state, true);
            }
            iter++;
        }

        if (oldSet) {
            *oldSet = std::move(set);
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
        document(), n, m_activeNodeSet, nullptr, &m_activeNodeTarget,
        &m_documentVersionWhenComputingActiveNodeSet, Node::NodeStateActive);
}

void BrowsingContext::releaseActiveNode()
{
    if (!m_activeNodeTarget) {
        return;
    }

    auto iter = m_activeNodeSet.begin();
    while (iter != m_activeNodeSet.end()) {
        (*iter)->setState(Node::NodeStateActive, false);
        iter++;
    }
    m_activeNodeSet.clear();
    m_activeNodeTarget = nullptr;
    m_documentVersionWhenComputingActiveNodeSet = 0;
}

bool BrowsingContext::setHoveredNode(
    Node* n, Optional<GCUnorderedSet<Node*>*> oldHoveredNodeSet)
{
    return updateEventNodeSet(document(), n, m_hoveredNodeSet,
                              oldHoveredNodeSet, &m_hoveredNodeTarget,
                              &m_documentVersionWhenComputingHoveredNodeSet,
                              Node::NodeStateHovered);
}

void BrowsingContext::releaseHoveredNode()
{
    if (!m_hoveredNodeTarget) {
        return;
    }

    auto iter = m_hoveredNodeSet.begin();
    while (iter != m_hoveredNodeSet.end()) {
        (*iter)->setState(Node::NodeStateHovered, false);
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
    MouseEvent* event =
        new MouseEvent(document->executionContext(), name, data);
    event->setBubbles(true);
    event->setCancelable(true);
    event->setView(document->window());
    return event;
}

static PointerEvent* createPointerEvent(Document* document, String* name,
                                        MouseData& data)
{
    PointerEvent* event =
        new PointerEvent(document->executionContext(), name, data);
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
        if (iframe->scrolling()->toASCIILower()->equals("no")) {
            return false;
        }

        auto fb = iframe->frame()->asFrameBox();
        // posX/posY are page coordinates of this browsing context, so the
        // iframe's offset must reflect scrolled ancestors (e.g. an
        // overflow:auto container that has been scrolled); absolutePoint()
        // ignores ancestor scroll offsets, which shifts (or entirely misses)
        // the coordinates handed to the inner browsing context. The
        // document's own scroll is already part of the incoming page
        // coordinates, so it must not be subtracted again.
        LayoutLocation absPoint;
        if (fb->style()->position() == PositionValue::FixedPositionValue) {
            // A fixed-position iframe (notably the UA-forced :fullscreen
            // style) is anchored to the viewport: ancestor scroll offsets do
            // not move it, so they must not be subtracted from its offset
            // (painting agrees - StackingContext::relativeLocation skips
            // ancestor scrolls for fixed owners). Its page-coordinate
            // position is the plain layout position plus the document
            // scroll that the incoming page coordinates already contain.
            absPoint = fb->absolutePoint(document()->frame()->asFrameBox());
            absPoint.setX(absPoint.x() + LayoutUnit(window()->scrollX(false)));
            absPoint.setY(absPoint.y() + LayoutUnit(window()->scrollY(false)));
        } else {
            absPoint = fb->absolutePointIncludingScroll(
                document()->frame()->asFrameBox(), false);
        }
        double newPosX = posX - (double)absPoint.x();
        double newPosY = posY - (double)absPoint.y();
        double contentX = (double)(fb->paddingLeft() + fb->borderLeft());
        double contentY = (double)(fb->paddingTop() + fb->borderTop());
        if (contentX <= newPosX && newPosX <= contentX + fb->contentWidth() &&
            contentY <= newPosY && newPosY <= contentY + fb->contentHeight()) {
            posX = newPosX - contentX;
            posY = newPosY - contentY;
            return true;
        }
    }
    return false;
}

void BrowsingContext::handleActiveAndFocus(MouseEventKind kind,
                                           Node* targetNode, double posX,
                                           double posY)
{
    if (kind == MouseEventKind::MouseEventDown) { // TouchEventStart
        m_touchDownPoint = Unit::Location(posX, posY);
        setActiveNode(targetNode);
        setFocusedNode(targetNode, true);
    } else if (kind == MouseEventKind::MouseEventUp) { // TouchEventEnd
        releaseActiveNode();
    }
}

void BrowsingContext::handleHover(MouseEventKind kind, Node* targetNode,
                                  unsigned char button, unsigned char buttons,
                                  double posX, double posY)
{
    if (kind != MouseEventKind::MouseEventMove) {
        return;
    }

    // Fast path: pointer stayed over the same hover target and the DOM has
    // not mutated since the hover set was last computed. In that case
    // setHoveredNode() would rebuild an identical set and the enter/over/
    // out/leave dispatch below is fully gated on (newTarget != oldTarget),
    // so it can produce no observable effect. Skip the redundant work.
    // This mirrors exactly the two conditions in updateEventNodeSet that
    // would otherwise leave the set unchanged; the third sub-condition there
    // (set.find(t) != set.end()) is the spurious one that forces a per-frame
    // rebuild, which we intentionally do not replicate. This is the common
    // case during a seek-bar drag (mousemove with button held while the
    // pointer stays on the same slider thumb).
    if (m_hoveredNodeTarget == targetNode &&
        m_documentVersionWhenComputingHoveredNodeSet ==
            document()->domVersion()) {
        return;
    }

    GCUnorderedSet<Node*> oldhoveredNodeSet;
    Node* oldTarget = m_hoveredNodeTarget;
    if (setHoveredNode(targetNode, &oldhoveredNodeSet)) {
        Node* newTarget = m_hoveredNodeTarget;
        if (newTarget != oldTarget) {
            Node* newElement = newTarget->nearestParentElement();
            Node* oldElement =
                oldTarget ? oldTarget->nearestParentElement() : nullptr;
            auto ts = timestamp();

            if (newElement && newElement->isElement()) {
                MouseData data(button, buttons, posX, posY, 0, ts, oldElement);
                Element* enterTarget = newElement->asElement();
                GCVector<Element*> enterList;
                while (enterTarget) {
                    if (!oldhoveredNodeSet.contains(enterTarget)) {
                        enterList.push_back(enterTarget);
                    }
                    enterTarget = enterTarget->parentElement();
                }

                for (auto iter = enterList.rbegin(); iter != enterList.rend();
                     iter++) {
                    Event* e = createMouseEvent(
                        document(),
                        starfish()->staticStrings()->m_mouseenter.localName(),
                        data);
                    e->setCancelable(false);
                    e->setBubbles(false);
                    (*iter)->dispatchEventByUA(newElement, e, true);
                }
            }

            {
                String* name =
                    starfish()->staticStrings()->m_mouseover.localName();
                MouseData data(button, buttons, posX, posY, 0, ts, oldElement);
                Event* e = createMouseEvent(document(), name, data);
                document()->window()->dispatchEventByUA(
                    newElement ? newElement : document(), e);
            }

            {
                String* name =
                    starfish()->staticStrings()->m_mouseout.localName();
                MouseData data(button, buttons, posX, posY, 0, ts, newElement);
                Event* e = createMouseEvent(document(), name, data);
                document()->window()->dispatchEventByUA(
                    oldElement ? oldElement : document(), e);
            }

            if (oldElement && oldElement->isElement()) {
                MouseData data(button, buttons, posX, posY, 0, ts, newElement);
                Element* leaveTarget = oldElement->asElement();
                while (leaveTarget) {
                    if (!(leaveTarget->state() & Node::NodeStateHovered)) {
                        Event* e =
                            createMouseEvent(document(),
                                             starfish()
                                                 ->staticStrings()
                                                 ->m_mouseleave.localName(),
                                             data);
                        e->setCancelable(false);
                        e->setBubbles(false);
                        leaveTarget->dispatchEventByUA(oldElement, e, true);
                    }
                    leaveTarget = leaveTarget->parentElement();
                }
            }
        }
    }
}

bool BrowsingContext::dispatchTouchEvent(TouchEventKind kind,
                                         TouchData* touches, size_t count)
{
    if (kind == TouchEventKind::TouchEventCancel) {
        releaseActiveNode();
        releaseHoveredNode();
        // Dispatch the DOM touchcancel so JS handlers can clean up gesture
        // state (e.g. remove ripple animations, cancel drag logic).
        if (count >= 1) {
            const double scrollOffsetX = window()->scrollX(false);
            const double scrollOffsetY = window()->scrollY(false);
            std::vector<TouchData> pageTouches(touches, touches + count);
            for (size_t i = 0; i < count; i++) {
                pageTouches[i].setClientX(pageTouches[i].clientX() +
                                          scrollOffsetX);
                pageTouches[i].setClientY(pageTouches[i].clientY() +
                                          scrollOffsetY);
            }
            Node* cancelTarget =
                hitTest(pageTouches[0].clientX(), pageTouches[0].clientY());
            Node* t =
                cancelTarget ? cancelTarget->nearestParentElement() : nullptr;
            if (!t && m_pointerCaptureTarget &&
                m_pointerCaptureTarget->isConnected()) {
                t = m_pointerCaptureTarget;
            }
            if (!t) {
                t = document();
            }
            String* cancelName =
                starfish()->staticStrings()->m_touchcancel.localName();
            Event* cancelEvent = createTouchEvent(document(), cancelName,
                                                  pageTouches.data(), count);
            document()->window()->dispatchEventByUA(t, cancelEvent);
        }
        m_pointerCaptureTarget = nullptr;
        return false;
    }
    if (count < 1) {
        return false;
    }
    // hitTest() and isInnerIFrameEvent() operate in page coordinates:
    // dispatchMouseEvent() and dispatchMouseWheelEvent() both add the
    // document scroll offset to the incoming window coordinates before hit
    // testing, but the touch path never did. With the document scrolled,
    // every touch hit-tested against a point shifted up/left by the scroll
    // offset (most visibly on a fullscreen element, which sits at the
    // viewport origin regardless of scroll). Mirror the mouse conversion;
    // downstream consumers (m_touchDownPoint, the synthesized click's
    // MouseData, createTouchEvent) all see the same page-coordinate
    // convention the mouse path establishes.
    // Convert on a local copy: callers reuse one TouchData array across
    // dispatches (Window::simulateClick and the CDP input domain pass the
    // same array to TouchEventStart and TouchEventEnd), so adding the
    // scroll offset in place would compound it on the second dispatch.
    std::vector<TouchData> pageTouches(touches, touches + count);
    touches = pageTouches.data();
    const double scrollOffsetX = window()->scrollX(false);
    const double scrollOffsetY = window()->scrollY(false);
    for (size_t i = 0; i < count; i++) {
        touches[i].setClientX(touches[i].clientX() + scrollOffsetX);
        touches[i].setClientY(touches[i].clientY() + scrollOffsetY);
    }
    // Handle touch informations
    // - Do the hitTest for all `Touch` informations and set target for each.
    // - Decide representative target (= first non-empty target).
    // - Check whether touch position moved away from original position
    //   to release active nodes.
    bool checkRelease = kind == TouchEventKind::TouchEventMove &&
                        (webView()->deviceKind() & deviceKindUseTouchScreen);
    Node* targetNode = nullptr;
    double targetX = 0;
    double targetY = 0;
    double targetScreenX = 0;
    double targetScreenY = 0;
    for (size_t i = 0; i < count; i++) {
        TouchData& touchData = touches[i];
        Node* node = hitTest(touchData.clientX(), touchData.clientY());
        touchData.setTarget(node);
        if (!targetNode) {
            targetNode = node;
            targetX = touchData.clientX();
            targetY = touchData.clientY();
            targetScreenX = touchData.screenX();
            targetScreenY = touchData.screenY();
        }
        if (checkRelease &&
            ((std::abs(m_touchDownPoint.x() - touchData.clientX()) > 30) ||
             (std::abs(m_touchDownPoint.y() - touchData.clientY()) > 30))) {
            releaseActiveNode();
            checkRelease = false;
        }
    }
    if (!targetNode) {
        return false;
    }
    // Handle event inside iframe
    bool clickableEvent = (kind == TouchEventKind::TouchEventEnd) &&
                          !webView()->scrollOccurredDuringGesture() &&
                          targetNode && m_activeNodeTarget &&
                          (targetNode == m_activeNodeTarget ||
                           targetNode->isDescendantOf(m_activeNodeTarget));
    double newX = targetX;
    double newY = targetY;
    if (isInnerIFrameEvent(targetNode, newX, newY)) {
        clickableEvent = false;
        handleActiveAndFocus((MouseEventKind)kind, targetNode, targetX,
                             targetY);

        TouchData newData(newX, newY, targetScreenX, targetScreenY);
        bool innerReturn = targetNode->asHTMLIFrameElement()
                               ->browsingContext()
                               ->dispatchTouchEvent(kind, &newData, 1);

        // Touch events do not bubble across iframe boundaries. Mirror the
        // mouse-event logic above: if the iframe is still the hit target,
        // the event was fully handled inside the iframe and must not be
        // re-dispatched in the parent browsing context.
        Node* afterTarget = hitTest(targetX, targetY);
        if (afterTarget == targetNode) {
            // JS touch events stay inside the iframe (isolation above), but
            // scrolling is a UA default action that must still chain to a
            // scrollable ancestor in this browsing context when the iframe's
            // own content did not consume the gesture as a scroll — otherwise
            // a touch-drag over an iframe that covers a scroll container can
            // never scroll the parent. The wheel path already chains this way.
            // Feed the gesture to the ancestor scrollers' default handlers only
            // (no JS listeners), and only until some scroller claims the
            // gesture; once claimed, the WebView global pointing-event
            // intercept drives continuation. scrollOccurredDuringGesture is
            // shared across the parent/child browsing contexts (same WebView),
            // so a true value means the inner content already owns the scroll —
            // leave it be.
            if (!webView()->scrollOccurredDuringGesture()) {
                String* scrollName;
                if (kind == TouchEventKind::TouchEventStart) {
                    scrollName =
                        starfish()->staticStrings()->m_touchstart.localName();
                } else if (kind == TouchEventKind::TouchEventMove) {
                    scrollName =
                        starfish()->staticStrings()->m_touchmove.localName();
                } else {
                    scrollName =
                        starfish()->staticStrings()->m_touchend.localName();
                }
                Event* scrollEvent =
                    createTouchEvent(document(), scrollName, touches, count);
                bool scrollHandled = false;
                for (Node* n = targetNode; n && !scrollHandled;
                     n = n->parentNode()) {
                    scrollHandled = n->handleDefaultEvent(scrollEvent);
                }
                if (!scrollHandled) {
                    document()->window()->handleDefaultEvent(scrollEvent);
                }
            }
            return innerReturn;
        }
        if (!afterTarget) {
            return false;
        }
        targetNode = afterTarget;
    }

    bool returnValue = false;
    // Pointer Events compat for touch input. The mouse path dispatches
    // pointerdown/pointermove/pointerup alongside the mouse events, but the
    // touch path only produced touch events plus a synthesized click, so
    // pointer-driven UI never reacted to touch (e.g. the YouTube player's
    // progress bar seeks from pointerdown: a mouse click on it seeks, a touch
    // tap on it did nothing, while its legacy touchmove scrubbing kept
    // working). Mirror the mouse path: same MouseData shape as the
    // synthesized click, same pointer-capture retargeting for move/up, and
    // implicit capture release on pointerup.
    // https://w3c.github.io/pointerevents/#compatibility-mapping-with-mouse-events
    MouseData pointerData(MouseButtonValue::LeftButton,
                          MouseButtonsValue::LeftButtonDown, targetX, targetY,
                          1);
    pointerData.setScreenX(targetScreenX);
    pointerData.setScreenY(targetScreenY);
    // Dispatch events
    String* name = String::emptyString;
    switch (kind) {
    case TouchEventKind::TouchEventStart: {
        webView()->setScrollOccurredDuringGesture(false);
        m_touchDownPoint = Unit::Location(targetX, targetY);
        m_touchSlopExceeded = false;
        // Dispatch touchstart event
        name = starfish()->staticStrings()->m_touchstart.localName();
        Event* e = createTouchEvent(document(), name, touches, count);
        Node* t = targetNode->nearestParentElement();
        t = t ? t : document();
        returnValue = !document()->window()->dispatchEventByUA(t, e);
        Event* pe = createPointerEvent(
            document(), starfish()->staticStrings()->m_pointerdown.localName(),
            pointerData);
        document()->window()->dispatchEventByUA(t, pe);
        break;
    }
    case TouchEventKind::TouchEventMove: {
        bool scrollAlreadyOccurred = webView()->scrollOccurredDuringGesture();
        // Once scroll has started, don't dispatch touchmove to JS — the touch
        // sequence was already cancelled via touchcancel (matches Chrome).
        if (scrollAlreadyOccurred) {
            break;
        }
        // Suppress touchmove below the touch-slop threshold so that slight
        // finger jitter during a tap does not cancel ripple animations or
        // other gesture-start logic (Chrome uses ~8 CSS px). The check is
        // one-shot per gesture: once the slop is exceeded the gesture is a
        // drag, and every subsequent touchmove must be dispatched even if it
        // passes back within slop distance of the touch-down point (e.g.
        // scrubbing a seek bar back and forth across the starting position).
        if (!m_touchSlopExceeded) {
            const double dx = targetX - (double)m_touchDownPoint.x();
            const double dy = targetY - (double)m_touchDownPoint.y();
            const double slopPx = STARFISH_TOUCH_SLOP_PX;
            if (dx * dx + dy * dy < slopPx * slopPx) {
                break;
            }
            m_touchSlopExceeded = true;
        }
        // Dispatch touchmove event
        name = starfish()->staticStrings()->m_touchmove.localName();
        Event* e = createTouchEvent(document(), name, touches, count);
        Node* t = targetNode->nearestParentElement();
        t = t ? t : document();
        returnValue = !document()->window()->dispatchEventByUA(t, e);
        // If scroll just started during this touchmove dispatch, fire
        // touchcancel to let JS clean up (matches Chrome behavior).
        if (!scrollAlreadyOccurred &&
            webView()->scrollOccurredDuringGesture()) {
            name = starfish()->staticStrings()->m_touchcancel.localName();
            Event* cancel = createTouchEvent(document(), name, touches, count);
            document()->window()->dispatchEventByUA(t, cancel);
            releaseActiveNode();
            break;
        }
        // Pointer capture retargeting, and the same listener gate the mouse
        // path uses to skip the redundant dispatch per high-frequency move.
        Node* pt =
            (m_pointerCaptureTarget && m_pointerCaptureTarget->isConnected())
                ? m_pointerCaptureTarget
                : t;
        if (pt->hasListenerForTypeOnPath(
                starfish()->staticStrings()->m_pointermove.localName())) {
            Event* pe = createPointerEvent(
                document(),
                starfish()->staticStrings()->m_pointermove.localName(),
                pointerData);
            document()->window()->dispatchEventByUA(pt, pe);
        }
        break;
    }
    case TouchEventKind::TouchEventEnd: {
        // Chrome clears hover and active on touch end — neither persists
        // across touch sequences (no :hover/:active maintained by touch).
        releaseHoveredNode();
        releaseActiveNode();
        Node* t = targetNode->nearestParentElement();
        t = t ? t : document();
        // Spec order: touchend → pointerup → click.
        name = starfish()->staticStrings()->m_touchend.localName();
        Event* e = createTouchEvent(document(), name, touches, count);
        returnValue = !document()->window()->dispatchEventByUA(t, e);
        Node* pt =
            (m_pointerCaptureTarget && m_pointerCaptureTarget->isConnected())
                ? m_pointerCaptureTarget
                : t;
        Event* pe = createPointerEvent(
            document(), starfish()->staticStrings()->m_pointerup.localName(),
            pointerData);
        document()->window()->dispatchEventByUA(pt, pe);
        // Implicit pointer capture release on pointerup.
        m_pointerCaptureTarget = nullptr;
        if (clickableEvent) {
            name = starfish()->staticStrings()->m_click.localName();
            MouseData clickData(MouseButtonValue::LeftButton,
                                MouseButtonsValue::LeftButtonDown, targetX,
                                targetY, 1);
            clickData.setScreenX(targetScreenX);
            clickData.setScreenY(targetScreenY);
            Event* click = createMouseEvent(document(), name, clickData);
            document()->window()->dispatchEventByUA(t, click);
        }
        break;
    }
    default:
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }
    // Handle properties
    handleActiveAndFocus((MouseEventKind)kind, targetNode, targetX, targetY);
    return returnValue;
}

bool BrowsingContext::dispatchMouseEvent(MouseEventKind kind, MouseData data)
{
    // MouseEventEnter/MouseEventOut are not supported yet
    if (kind >= MouseEventKind::MouseEventEnter) {
        STARFISH_UNSUPPORTED("MouseEvent: MouseEventEnter, MouseEventOut");
        return false;
    }

    bool mouseMoved = false;
    if (m_lastMouseMovePoint.x() != data.screenX() ||
        m_lastMouseMovePoint.y() != data.screenY()) {
        m_lastMouseMovePoint.setX(data.screenX());
        m_lastMouseMovePoint.setY(data.screenY());
        mouseMoved = true;
    }

    if (kind == MouseEventKind::MouseEventMove && !mouseMoved) {
        return true;
    }

    // STARFISH_LOG_INFO("BrowsingContext::dispatchMouseEvent %d %f %f %d",
    // (int)kind, data.clientX(), data.clientY(), (int)data.buttons());

    // https://drafts.csswg.org/cssom-view/#ref-for-dom-mouseevent-pagex
    // Cache the scroll offset: window()->scrollX/scrollY(false) traverses
    // frameDocument->scrollLeft()/scrollTop() on each call and returns the same
    // value for all four uses here. Reading once each halves the
    // accessor/frame- traversal cost per (coalesced) mousemove.
    // clientX()/clientY() are read BEFORE the setters mutate them, preserving
    // the original two-line semantics.
    const double scrollOffsetX = window()->scrollX(false);
    const double scrollOffsetY = window()->scrollY(false);
    const double baseClientX = data.clientX();
    const double baseClientY = data.clientY();
    data.setPageX(baseClientX + scrollOffsetX);
    data.setPageY(baseClientY + scrollOffsetY);

    data.setClientX(baseClientX + scrollOffsetX);
    data.setClientY(baseClientY + scrollOffsetY);

    // Pointer-capture fast path for moves: when an element has captured the
    // pointer (e.g. the YouTube seek-bar thumb), a mousemove's hit-test result
    // is unconditionally discarded in favour of m_pointerCaptureTarget (see the
    // override below) and handleActiveAndFocus()/handleHover() are driven from
    // that captor, not from the hit-tested node. The only other consumer of the
    // hit-test on a move is the iframe-delegation check below, which still runs
    // when the captor is itself an iframe. So skip hitTest() here and avoid its
    // layoutIfNeeded() pass, which otherwise runs for every coalesced mousemove
    // during a drag. mouseup remains authoritative (it still hit-tests below),
    // so the final cursor/drag position is unaffected.
    bool usedPointerCaptureTarget =
        (kind == MouseEventKind::MouseEventMove && m_pointerCaptureTarget &&
         m_pointerCaptureTarget->isConnected());
    Node* targetNode;
    if (usedPointerCaptureTarget) {
        targetNode = m_pointerCaptureTarget;
    } else {
        // Hit test to validate event position
        targetNode = hitTest((float)data.clientX(), (float)data.clientY());
        if (!targetNode) {
            return false;
        }
    }
    double targetX = data.clientX();
    double targetY = data.clientY();
    double newX = targetX;
    double newY = targetY;

    bool clickableEvent = (kind == MouseEventKind::MouseEventUp) &&
                          !data.isDefaultPrevented() &&
                          !webView()->scrollOccurredDuringGesture() &&
                          targetNode && m_activeNodeTarget &&
                          (targetNode == m_activeNodeTarget ||
                           targetNode->isDescendantOf(m_activeNodeTarget));

    // Handle event inside iframe. When the move was retargeted to a captured
    // element, skip the iframe-delegation check unless the captor is itself an
    // iframe (an iframe can capture the pointer via
    // Element::setPointerCapture). For the common non-iframe captor (e.g. a
    // seek-bar thumb) this elides one virtual isHTMLIFrameElement() call per
    // coalesced move; if an iframe is captured, delegation into the inner
    // browsing context is preserved.
    if ((!usedPointerCaptureTarget || targetNode->isHTMLIFrameElement()) &&
        isInnerIFrameEvent(targetNode, newX, newY)) {
        clickableEvent = false;
        handleActiveAndFocus(kind, targetNode, targetX, targetY);
        handleHover(kind, targetNode, data.button(), data.buttons(), targetX,
                    targetY);

        Node* iframeNode = targetNode;
        MouseData newData(data.button(), data.buttons(), newX, newY,
                          data.screenX(), data.screenY(), 0);
        bool innerReturn = iframeNode->asHTMLIFrameElement()
                               ->browsingContext()
                               ->dispatchMouseEvent(kind, newData);

        // Mouse events do not bubble across iframe boundaries. If the iframe
        // is still attached and still owns the hit position, the event was
        // fully handled inside the iframe and must not be re-dispatched in
        // the parent browsing context (otherwise window-level listeners in
        // the parent would also fire for clicks inside the iframe, with
        // coordinates referring to the parent's viewport).
        Node* afterTarget =
            hitTest((float)data.clientX(), (float)data.clientY());
        if (afterTarget == iframeNode) {
            return innerReturn;
        }

        // The iframe was detached or replaced synchronously by a handler.
        // Fall back to dispatching at the new target in the parent context.
        if (!afterTarget) {
            return false;
        }
        targetNode = afterTarget;
    }

    bool returnValue = false;
    // Dispatch events
    String* name = String::emptyString;
    Node* t = targetNode->nearestParentElement();
    t = t ? t : document();

    // Pointer capture: once an element captures the pointer (setPointerCapture
    // inside a pointerdown handler), subsequent move/up events are retargeted
    // to it regardless of the fresh hit-test, so a drag keeps reaching the
    // captor after the pointer leaves its box (e.g. a thin slider thumb such
    // as the YouTube seek bar).
    // https://w3c.github.io/pointerevents/#pointer-capture
    if (m_pointerCaptureTarget && m_pointerCaptureTarget->isConnected() &&
        (kind == MouseEventKind::MouseEventMove ||
         kind == MouseEventKind::MouseEventUp)) {
        t = m_pointerCaptureTarget;
    }
    switch (kind) {
    case MouseEventKind::MouseEventDown: {
        webView()->setScrollOccurredDuringGesture(false);
        // Dispatch mousedown event
        name = starfish()->staticStrings()->m_mousedown.localName();
        MouseData downData(data);
        downData.setRelatedTarget(nullptr);
        Event* me = createMouseEvent(document(), name, downData);
        returnValue = !document()->window()->dispatchEventByUA(t, me);
        Event* pe = createPointerEvent(
            document(), starfish()->staticStrings()->m_pointerdown.localName(),
            downData);
        document()->window()->dispatchEventByUA(t, pe);
        break;
    }
    case MouseEventKind::MouseEventMove: {
        // Once scroll has started, don't dispatch mousemove to JS.
        if (webView()->scrollOccurredDuringGesture()) {
            break;
        }
        // Dispatch mousemove event
        name = starfish()->staticStrings()->m_mousemove.localName();
        // Reuse the by-value `data` local instead of allocating a separate
        // MouseData copy per (throttled, high-frequency) move. relatedTarget
        // is never read again after this branch, so mutating it here is safe.
        data.setRelatedTarget(nullptr);
        Event* me = createMouseEvent(document(), name, data);
        returnValue = !document()->window()->dispatchEventByUA(t, me);
        // Only dispatch the high-frequency pointermove if some node on the
        // (possibly captured) event path actually listens for it; otherwise
        // skip the redundant second full 3-phase dispatch per move.
        if (t->hasListenerForTypeOnPath(
                starfish()->staticStrings()->m_pointermove.localName())) {
            Event* pe = createPointerEvent(
                document(),
                starfish()->staticStrings()->m_pointermove.localName(), data);
            document()->window()->dispatchEventByUA(t, pe);
        }
        break;
    }
    case MouseEventKind::MouseEventUp: {
        // Dispatch mouseup event
        name = starfish()->staticStrings()->m_mouseup.localName();
        MouseData upData(data);
        upData.setRelatedTarget(nullptr);
        Event* me = createMouseEvent(document(), name, upData);
        returnValue = !document()->window()->dispatchEventByUA(t, me);
        Event* pe = createPointerEvent(
            document(), starfish()->staticStrings()->m_pointerup.localName(),
            upData);
        document()->window()->dispatchEventByUA(t, pe);

        // Implicit pointer capture release on pointerup.
        m_pointerCaptureTarget = nullptr;

        if (clickableEvent) {
            // Dispatch click event
            name = starfish()->staticStrings()->m_click.localName();
            Event* click = createMouseEvent(document(), name, data);
            document()->window()->dispatchEventByUA(t, click);
        }
        break;
    }
    default:
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }
    // Handle properties
    handleActiveAndFocus(kind, t, targetX, targetY);
    handleHover(kind, t, data.button(), data.buttons(), targetX, targetY);
    return returnValue;
}

bool BrowsingContext::dispatchMouseWheelEvent(float screenX, float screenY,
                                              int z, bool isVerticalWheelEvent)
{
    double wx = window()->scrollX(false) + screenX;
    double wy = window()->scrollY(false) + screenY;
    // Hit test to validate event position
    Node* targetNode = hitTest(wx, wy);
    if (!targetNode) {
        return false;
    }

    // Handle event inside iframe
    if (isInnerIFrameEvent(targetNode, wx, wy)) {
        if (targetNode->asHTMLIFrameElement()
                ->browsingContext()
                ->dispatchMouseWheelEvent(wx, wy, z, isVerticalWheelEvent)) {
            return true;
        }
    }

    bool useEventInDOMTree = false;
    Node* node = targetNode;
    while (node) {
        if (node->isElement() && node->frame() &&
            node->frame()->shouldApplyOverflow()) {
            Element* e = node->asElement();
            if (isVerticalWheelEvent) {
                if (e->appliedOverflowY() >= OverflowValue::AutoOverflow) {
                    if (e->frame()->isFrameBlockBox()) {
                        if (e->frame()
                                ->asFrameBlockBox()
                                ->hasBiggerContentThanFrameHeight()) {
                            double t = e->scrollTop();
                            double scrollBefore = t;
                            t += z * 30;
                            e->setScrollTop(t);
                            if (scrollBefore != e->scrollTop()) {
                                useEventInDOMTree = true;
                            }
                            break;
                        }
                    }
                }
            } else {
                if (e->appliedOverflowX() >= OverflowValue::AutoOverflow) {
                    if (e->frame()->isFrameBlockBox()) {
                        if (e->frame()
                                ->asFrameBlockBox()
                                ->hasBiggerContentThanFrameWidth()) {
                            double t = e->scrollLeft();
                            double scrollBefore = t;
                            t += z * 30;
                            e->setScrollLeft(t);
                            if (scrollBefore != e->scrollLeft()) {
                                useEventInDOMTree = true;
                            }
                            break;
                        }
                    }
                }
            }
        }
        node = node->parentElement();
    }

    if (useEventInDOMTree) {
        return true;
    }

    double sx = window()->scrollX();
    double sy = window()->scrollY();
    auto ao = document()->appliedOverflow();
    OverflowValue ox = ao.first;
    OverflowValue oy = ao.second;

    if (isVerticalWheelEvent) {
        if (oy >= OverflowValue::AutoOverflow) {
            sy += z * 15;
        }
    } else {
        if (ox >= OverflowValue::AutoOverflow) {
            sx += z * 15;
        }
    }

    return window()->scrollTo(sx, sy);
}

void BrowsingContext::dispatchKeyEvent(KeyEventKind kind,
                                       PlatformKeyEventData& pkdata)
{
    // Set target
    // 1) currently focused element if possible
    // or 2) body element if possible
    // or 3) root element
    Node* target = m_focusedNode;
    if (!target) {
        if (document()->body()) {
            target = document()->body();
        } else if (document()->rootElement()) {
            target = document()->rootElement();
        } else {
            return;
        }
    } else if (target && target->isHTMLIFrameElement()) {
        if (target->asHTMLIFrameElement()->browsingContext()) {
            if (target->asHTMLIFrameElement()->frame()) {
                target->asHTMLIFrameElement()
                    ->browsingContext()
                    ->dispatchKeyEvent(kind, pkdata);
            }
        }
        return;
    }
    // Dispatch event
    String* eventType = String::emptyString;
    if (kind == KeyEventKind::KeyEventUp) {
        eventType = starfish()->staticStrings()->m_keyup.localName();
        setKeydownEventDefaultPrevented(false);
    } else if (kind == KeyEventKind::KeyEventPress) {
        if (pkdata.keyValue() != LWE::EnterKey &&
            !String::isASCIIPrintableKey(pkdata.keyValue())) {
            return;
        } else if (keydownEventDefaultPrevented()) {
            return;
        }

        eventType = starfish()->staticStrings()->m_keypress.localName();
    } else {
        // kind == KeyEventKind::KeyEventDown
        eventType = starfish()->staticStrings()->m_keydown.localName();
    }

    if (kind != KeyEventKind::KeyEventPress) {
        // For keydown or keyup events, the value of charCode is 0.
        pkdata.setCharCode(0);
    }

    pkdata.setEventModifierData(webView()->renderer()->eventModifierData());

    KeyboardEventInit kinitData(pkdata);
    KeyboardEvent* e =
        new KeyboardEvent(document()->executionContext(), eventType, kinitData);
    e->setBubbles(true);
    e->setCancelable(true);
    e->setView(document()->window());
    document()->window()->dispatchEventByUA(target, e);

    bool shouldDispatchInputEvent = true;
    bool isTextEditable = target->isHTMLTextEditable();

    if (!e->defaultPrevented()) {
        if (kind == KeyEventKind::KeyEventDown) {
            if (e->keyValue() == KeyValue::TabKey) {
                shouldDispatchInputEvent = false;
                if (e->shiftKey()) {
                    focusNavigation(false);
                } else {
                    focusNavigation();
                }
                e->defaultPrevented();
            } else if (e->keyValue() == KeyValue::EnterKey ||
                       e->keyValue() == KeyValue::SpaceKey) {
                if (e->keyValue() == KeyValue::EnterKey) {
                    shouldDispatchInputEvent = false;
                }
                if (target->isHTMLButtonElement() ||
                    (target->isHTMLInputElement() &&
                     target->asHTMLInputElement()->hasActivationBehavior())) {
                    String* eventType =
                        starfish()->staticStrings()->m_click.localName();
                    Node* t = target;
                    if (t) {
                        t = t->nearestParentElement();
                        document()->window()->dispatchEventByUA(
                            t, new Event(document()->executionContext(),
                                         eventType, EventInit(true, true)));
                        e->defaultPrevented();
                    }
                }
            } else if (e->keyValue() >= KeyValue::ArrowDownKey &&
                       e->keyValue() <= KeyValue::ArrowRightKey) {
                shouldDispatchInputEvent = false;
                if (!isTextEditable) {
#if defined(STARFISH_ANDROID)
                    focusNavigationWithArrow(e);
                    e->defaultPrevented();
#else
                    double sx = window()->scrollX(false);
                    double sy = window()->scrollY(false);
                    auto ao = document()->appliedOverflow();
                    OverflowValue ox = ao.first;
                    OverflowValue oy = ao.second;

                    if (e->keyValue() == KeyValue::ArrowDownKey &&
                        oy >= OverflowValue::AutoOverflow) {
                        sy += 15;
                    } else if (e->keyValue() == KeyValue::ArrowUpKey &&
                               oy >= OverflowValue::AutoOverflow) {
                        sy -= 15;
                    } else if (e->keyValue() == KeyValue::ArrowRightKey &&
                               ox >= OverflowValue::AutoOverflow) {
                        sx += 15;
                    } else if (e->keyValue() == KeyValue::ArrowLeftKey &&
                               ox >= OverflowValue::AutoOverflow) {
                        sx -= 15;
                    }

                    window()->scrollToWithoutLayout(sx, sy);
#endif
                }
            }
        }
    }

    // After editing, this 'oninput' event is called.
    if (kind == KeyEventKind::KeyEventDown && isTextEditable &&
        shouldDispatchInputEvent && target->isHTMLFormControl()) {
        InputEvent* event = new InputEvent(document()->executionContext(),
                                           String::createASCIIString("input"));
        event->setCancelable(false);
        event->setBubbles(true);
        event->setComposed(true);
        event->setData(target->asHTMLFormControl()->value());
        event->setInputType(String::createASCIIString("insertText"));

        document()->window()->dispatchEventByUA(target, event);
    }
}

#if defined(STARFISH_ANDROID)
void BrowsingContext::focusNavigationWithArrow(KeyboardEvent* e)
{
    if (!(e->keyValue() >= KeyValue::ArrowDownKey &&
          e->keyValue() <= KeyValue::ArrowRightKey)) {
        return;
    }

    LayoutUnit x = 0, y = 0;
    switch (e->keyValue()) {
    case KeyValue::ArrowUpKey:
        y = -1;
        break;
    case KeyValue::ArrowDownKey:
        y = 1;
        break;
    case KeyValue::ArrowRightKey:
        x = 1;
        break;
    case KeyValue::ArrowLeftKey:
        x = -1;
        break;
    default:
        return;
    }

    const auto& focusRing = document()->focusRing();

    Node* node = focusedNode();

    if (!node) {
        focusNavigation();
        return;
    }

    if (!node->frame()) {
        focusNavigation();
        return;
    }

    if (!node->frame()->isFrameBox()) {
        focusNavigation();
        return;
    }

    FrameBox* target = node->frame()->asFrameBox();
    LayoutLocation targetLoc =
        target->absolutePoint(document()->frame()->asFrameDocument());
    Element* next = nullptr;
    LayoutUnit maxdist = SIZE_MAX;

    for (size_t i = 0; i < focusRing.size(); i++) {
        if (focusRing[i] == node) {
            continue;
        }

        if (!focusRing[i]) {
            continue;
        }

        if (!focusRing[i]->frame()) {
            continue;
        }

        if (!focusRing[i]->frame()->isFrameBox()) {
            continue;
        }

        FrameBox* box = focusRing[i]->frame()->asFrameBox();
        LayoutLocation boxLoc =
            box->absolutePoint(document()->frame()->asFrameDocument());
        LayoutUnit pX = targetLoc.x() + target->width() / 2;
        LayoutUnit pY = targetLoc.y() + target->height() / 2;

        LayoutUnit cX = std::max(
            std::min(pX.toDouble(), (boxLoc.x() + box->width()).toDouble()),
            boxLoc.x().toDouble());
        LayoutUnit cY = std::max(
            std::min(pY.toDouble(), (boxLoc.y() + box->height()).toDouble()),
            boxLoc.y().toDouble());

        LayoutUnit diffX = cX - pX;
        LayoutUnit diffY = cY - pY;
        LayoutUnit dist =
            sqrt((diffX * diffX).toDouble() + (diffY * diffY).toDouble());

        if (!dist) {
            dist = 1;
        }

        diffX = diffX / dist;
        diffY = diffY / dist;

        LayoutUnit dot = acos((x * diffX).toDouble() + (y * diffY).toDouble());
        LayoutUnit frustum = 3.14 * 45 / 180;
        if (maxdist > dist && (dot <= frustum && dot >= 0)) {
            maxdist = dist;
            next = focusRing[i];
        }
    }

    if (next) {
        next->scrollIntoViewIfNeeded();
        setFocusedNode(next, false);
    }
}
#endif

void BrowsingContext::focusNavigation(bool forward)
{
    const auto& focusRing = document()->focusRing();

    Node* node = focusedNode();

    size_t current = 0;
    for (size_t i = 0; i < focusRing.size(); i++) {
        if (focusRing[i] == node) {
            current = i;
            break;
        }
    }

    if (forward) {
        current++;
    } else {
        current--;
    }

    if (current == SIZE_MAX) {
        if (isTopLevelBrowsingContext()) {
            current = focusRing.size() - 1;
        } else {
            parentBrowsingContext()->focusNavigation(false);
            return;
        }
    }

    if (current == focusRing.size()) {
        if (isTopLevelBrowsingContext()) {
            current = 0;
        } else {
            parentBrowsingContext()->focusNavigation(true);
            return;
        }
    }

    if (focusRing[current]) {
        focusRing[current]->scrollIntoViewIfNeeded();
        setFocusedNode(focusRing[current], false);
    } else {
        releaseFocusedNode(nullptr);
    }
}

void BrowsingContext::dispatchCompositionEvent(CompositionEventKind kind,
                                               String* data,
                                               Optional<Node*> node)
{
    // Set target
    // 1) currently focused element if possible
    // or 2) body element if possible
    // or 3) root element
    Node* target;
    if (node.hasValue()) {
        target = node.value();
    } else {
        target = m_focusedNode;
    }
    if (!target) {
        if (document()->body()) {
            target = document()->body();
        } else if (document()->rootElement()) {
            target = document()->rootElement();
        } else {
            return;
        }
    } else if (target && target->isHTMLIFrameElement()) {
        if (target->asHTMLIFrameElement()->browsingContext()) {
            if (target->asHTMLIFrameElement()->frame()) {
                target->asHTMLIFrameElement()
                    ->browsingContext()
                    ->dispatchCompositionEvent(kind, data, nullptr);
            }
        }
        return;
    }

    if (keydownEventDefaultPrevented()) {
        return;
    }

    // Dispatch event
    String* eventType = String::emptyString;
    if (kind == CompositionEventKind::CompositionEventStart) {
        eventType = starfish()->staticStrings()->m_compositionstart.localName();
    } else if (kind == CompositionEventKind::CompositionEventUpdate) {
        if (compositionStartEventDefaultPrevented()) {
            return;
        }
        eventType =
            starfish()->staticStrings()->m_compositionupdate.localName();
    } else {
        STARFISH_ASSERT(kind == CompositionEventKind::CompositionEventEnd);
        eventType = starfish()->staticStrings()->m_compositionend.localName();
        setCompositionStartEventDefeaultPrevented(false);
    }
    CompositionEvent* e =
        new CompositionEvent(document()->executionContext(), eventType, data);
    e->setBubbles(true);
    if (kind == CompositionEventKind::CompositionEventStart) {
        e->setCancelable(true);
    } else {
        e->setCancelable(false);
    }
    e->setComposed(true);
    e->setView(document()->window());
    document()->window()->dispatchEventByUA(target, e);
}

void BrowsingContext::pause()
{
    document()->setVisibilityState(VisibilityState::VisibilityStateHidden);

    iterateChildContext([](BrowsingContext* ctx) {
        STARFISH_ASSERT(ctx);
        ctx->pause();
    });
}

void BrowsingContext::resume()
{
    document()->setVisibilityState(VisibilityState::VisibilityStateVisible);

    iterateChildContext([](BrowsingContext* ctx) { ctx->resume(); });
}

void BrowsingContext::setNeedsFullLayout()
{
    if (document()->frame() != nullptr) {
        FrameBox* fb = document()->frame()->asFrameBox();
        fb->markNeedsLayout();
        fb->iterateChildFrameBox([](FrameBox* fb) {
            STARFISH_ASSERT(fb);
            fb->markNeedsLayout();
        });

        setNeedsLayout();
    }
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
    if (isTopLevelBrowsingContext()) {
        return;
    }

    auto& v = m_webView->m_browsingContextsNeedsLayout;
    if (v.end() == std::find(v.begin(), v.end(), this)) {
        v.push_back(this);
    }
}

void BrowsingContext::unregisterNeedsLayoutInWebView()
{
    if (isTopLevelBrowsingContext()) {
        return;
    }
    auto& v = m_webView->m_browsingContextsNeedsLayout;
    auto iter = std::find(v.begin(), v.end(), this);
    if (iter != v.end()) {
        v.erase(iter);
    }
}

void BrowsingContext::registerDidLayoutInWebView()
{
    auto& v = m_webView->m_browsingContextsDidLayout;
    if (v.end() == std::find(v.begin(), v.end(), this)) {
        v.push_back(this);
    }
}

void BrowsingContext::unregisterDidLayoutInWebView()
{
    auto& v = m_webView->m_browsingContextsNeedsLayout;
    auto iter = std::find(v.begin(), v.end(), this);
    if (iter != v.end()) {
        v.erase(iter);
    }
}

bool BrowsingContext::isDescendantOf(BrowsingContext* ancester)
{
    if (ancester) {
        BrowsingContext* parent = m_parentBrowsingContext;
        while (parent) {
            if (parent == ancester) {
                return true;
            }
            parent = parent->parentBrowsingContext();
        }
    }
    return false;
}
} // namespace Starfish
