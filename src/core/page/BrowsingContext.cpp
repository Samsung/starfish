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
#ifdef STARFISH_ENABLE_MULTIMEDIA
#include "core/dom/HTMLMediaElement.h"
#endif
#include "core/dom/EventTarget.h"
#include "core/dom/MouseEvent.h"
#include "core/dom/KeyboardEvent.h"
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
#include "platform/window/PlatformWindow.h"
#include "core/animation/AnimationTask.h"
#include "core/dom/Traverse.h"
#include "core/dom/HTMLIFrameElement.h"
#include "platform/loader/ResourceLoader.h"
#include "core/dom/InputEvent.h"

namespace Starfish {

BrowsingContext* BrowsingContext::create(WebView* webView)
{
    STARFISH_ASSERT(webView);
    return new BrowsingContext(webView);
}

BrowsingContext* BrowsingContext::create(HTMLIFrameElement* sourceElement)
{
    return new BrowsingContext(sourceElement->webView(), sourceElement);
}

BrowsingContext::BrowsingContext(WebView* webView, HTMLIFrameElement* source)
    : WebViewHoldable(webView)
    , m_webView(webView)
    , m_window(nullptr)
    , m_parentBrowsingContext(source ? source->document()->browsingContext()
                                     : nullptr)
    , m_sourceElement(source)
    , m_pendingStyleSheetCount(0)
    , m_pendingRenderingCount(0)
    , m_touchDownPoint(0, 0)
    , m_lastMouseMovePoint(std::numeric_limits<float>::quiet_NaN(),
                           std::numeric_limits<float>::quiet_NaN())
    , m_activeNodeTarget(nullptr)
    , m_documentVersionWhenComputingActiveNodeSet(0)
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
                                  webView()->platformWindow()->width() /
                                      webView()->screenInfo().devicePixelRatio,
                                  webView()->platformWindow()->height() /
                                      webView()->screenInfo().devicePixelRatio);
    } else {
        if (m_sourceElement->frame()) {
            m_window =
                Window::create(this, url, (uint32_t)m_sourceElement->frame()
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

    m_window->document()->init(referrerURL);

    // STARFISH_LOG_INFO("BrowsingContext::open %s\n",
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

class WebFontLoadChecker : public ResourceClient {
public:
    WebFontLoadChecker(Resource* res, String* wf)
        : ResourceClient(res)
        , m_familyName(wf)
    {
    }
    virtual void didLoadFailed()
    {
        ResourceClient::didLoadFailed();
        resource()
            ->loader()
            ->document()
            ->browsingContext()
            ->setWholeDocumentNeedsStyleRecalc();
        resource()->loader()->document()->fontSelector()->clearCache(
            m_familyName);
        resource()->loader()->document()->setNeedsFrameTreeBuildWithoutSelf();
        STARFISH_LOG_INFO("WebFont %s is failed to load..\n",
                          m_familyName->toUTF8NonGCString().data());
    }

    virtual void didLoadFinished()
    {
        ResourceClient::didLoadFinished();
        resource()
            ->loader()
            ->document()
            ->browsingContext()
            ->setWholeDocumentNeedsStyleRecalc();
        resource()->loader()->document()->fontSelector()->clearCache(
            m_familyName);
        // we needs to rebuild frame tree due to considering pseudo-elements
        resource()->loader()->document()->setNeedsFrameTreeBuildWithoutSelf();
        STARFISH_LOG_INFO("WebFont %s is downloaded\n",
                          m_familyName->toUTF8NonGCString().data());
        resource()->loader()->document()->updateCanvasWebFontState();
    }

    virtual void didLoadCanceled()
    {
        ResourceClient::didLoadCanceled();
    }

    String* m_familyName;
};

void BrowsingContext::resolveStyleIfNeeds()
{
    if (m_needsStyleRecalc || m_needsStyleRecalcForWholeDocument) {
        if (m_needsStyleSheetsRecalc) {
            INSTALL_PROFILE_TIMER("parse sheet & collect rules");

            m_needsStyleSheetsRecalc = false;
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
            document()->m_webFontList.clear();
            size_t offset = 0;
            std::vector<CSSStyleDeclaration*> webFonts;

            // We can use non gc vector
            // because CSSStyleSheets has string reference to each
            // CSSStyleDeclaration
            for (size_t i = 0; i < sheets; i++) {
                CSSStyleSheet* sheet = document()->styleResolver().sheets()[i];

                if (i > 0) { // i == 0 is UA-sheet
                    sheet->parseSheetIfneeds();

                    const MediaQueryEvaluator& evaluator =
                        document()->styleResolver().mediaQueryEvaluator();
                    if (sheet->mediaQuerySet() &&
                        !sheet->matchesMediaQueries(
                            evaluator, sheet->mediaQuerySet(),
                            viewportDependentResult, deviceDependentResult)) {
                        continue;
                    }

                    sheet->clearStyleRules();
                    sheet->clearKeyframesRules();
                    sheet->collectRulesFromImportedSheet(
                        sheet->importRules(), webFonts, viewportDependentResult,
                        deviceDependentResult);
                    sheet->collectStyleRules(
                        sheet->childRules(), webFonts, sheet->url(),
                        viewportDependentResult, deviceDependentResult);
                }

                size_t rules = sheet->styleRules().size();
                for (size_t j = 0; j < rules; j++) {
                    sheet->styleRules()[j].first->setOrder(j + offset);
                    document()->styleResolver().addToRuleSet(
                        sheet->styleRules()[j]);
                }
                offset += rules;

                size_t keyframes = sheet->keyframes().size();
                for (size_t k = 0; k < keyframes; k++) {
                    document()->styleResolver().addToKeyframesRule(
                        sheet->keyframes()[k]);
                }
            }

#if !defined(PORT_CANVAS_BACKEND_MOCK)
            for (size_t i = 0; i < webFonts.size(); i++) {
                CSSStyleDeclaration* decl = webFonts[i];
                if (!decl->hasCSSValuePair(
                        CSSStyleValuePair::KeyKind::FontFamily) ||
                    decl->getCSSValuePair(
                            CSSStyleValuePair::KeyKind::FontFamily)
                            .valueKind() !=
                        CSSStyleValuePair::ValueKind::KeywordValueKind ||
                    !decl->hasCSSValuePair(CSSStyleValuePair::KeyKind::Src)) {
                    continue;
                }
                String* fontFamily =
                    decl->getCSSValuePair(
                            CSSStyleValuePair::KeyKind::FontFamily)
                        .keywordValue();
                FontFaceSrcData* src =
                    decl->getCSSValuePair(CSSStyleValuePair::KeyKind::Src)
                        .fontFaceSrcDataValue();
                bool isFontWeightSpecified = decl->hasCSSValuePair(
                    CSSStyleValuePair::KeyKind::FontWeight);
                bool isFontStyleSpecified = decl->hasCSSValuePair(
                    CSSStyleValuePair::KeyKind::FontStyle);
                FontStyleValue style = FontStyleValue::NormalFontStyleValue;
                char weight = FontWeightValue::NormalFontWeightValue;
                if (isFontWeightSpecified) {
                    auto w = decl->getCSSValuePair(
                        CSSStyleValuePair::KeyKind::FontWeight);
                    if (w.valueKind() !=
                        CSSStyleValuePair::ValueKind::FontWeightValueKind) {
                        isFontWeightSpecified = false;
                    } else {
                        weight = w.fontWeightValue();
                    }
                }

                switch (weight) {
                case OneHundredFontWeightValue:
                    weight = 1;
                    break;
                case TwoHundredsFontWeightValue:
                    weight = 2;
                    break;
                case ThreeHundredsFontWeightValue:
                    weight = 3;
                    break;
                case FourHundredsFontWeightValue:
                case NormalFontWeightValue:
                    weight = 4;
                    break;
                case FiveHundredsFontWeightValue:
                    weight = 5;
                    break;
                case SixHundredsFontWeightValue:
                    weight = 6;
                    break;
                case SevenHundredsFontWeightValue:
                case BoldFontWeightValue:
                    weight = 7;
                    break;
                case EightHundredsFontWeightValue:
                    weight = 8;
                    break;
                case NineHundredsFontWeightValue:
                    weight = 9;
                    break;
                default:
                    break;
                }

                if (isFontStyleSpecified) {
                    auto w = decl->getCSSValuePair(
                        CSSStyleValuePair::KeyKind::FontStyle);
                    if (w.valueKind() !=
                        CSSStyleValuePair::ValueKind::FontStyleValueKind) {
                        isFontStyleSpecified = false;
                    } else {
                        style = w.fontStyleValue();
                    }
                }

                // finding suitable font
                std::vector<size_t> indexes;
                for (size_t k = 0; k < src->data().size(); k++) {
                    indexes.push_back(k);
                }

                std::sort(indexes.begin(), indexes.end(),
                          [&](const size_t& a, const size_t& b) -> bool {
                              auto sa = src->data()[a];
                              auto sb = src->data()[b];

                              auto loadFromA = std::get<1>(sa);
                              auto loadFromB = std::get<1>(sb);

                              // a < b -> true;
                              if (loadFromA == FontFaceSrcData::Local &&
                                  loadFromB == FontFaceSrcData::Local) {
                                  return a < b;
                              } else if (loadFromA == FontFaceSrcData::Local &&
                                         loadFromB == FontFaceSrcData::URL) {
                                  return false;
                              } else if (loadFromA == FontFaceSrcData::URL &&
                                         loadFromB == FontFaceSrcData::Local) {
                                  return true;
                              } else {
                                  auto formatA = std::get<2>(sa);
                                  auto formatB = std::get<2>(sb);
                                  return formatA > formatB;
                              }
                          });

                auto fontFaceData = src->data()[indexes[0]];

                if (std::get<1>(fontFaceData) != FontFaceSrcData::Local) {
                    ResourceURL* fontURL =
                        new ResourceURL(std::get<0>(fontFaceData),
                                        document()->documentURI()->urlString());

                    FontResource* res = nullptr;
                    for (size_t i = 0;
                         i < document()->m_loadedWebFontList.size(); i++) {
                        if (fontURL->urlString()->equals(
                                document()
                                    ->m_loadedWebFontList[i]
                                    ->url()
                                    ->urlString())) {
                            res = document()->m_loadedWebFontList[i];
                            break;
                        }
                    }

                    if (res == nullptr) {
                        res = document()->resourceLoader().fetchFont(fontURL);
                        res->addResourceClient(
                            new WebFontLoadChecker(res, fontFamily));

                        document()->m_loadedWebFontList.push_back(res);
                    }

                    WebFont webFont(isFontStyleSpecified, isFontWeightSpecified,
                                    fontFamily, style, weight, res);
                    document()->m_webFontList.push_back(webFont);
                } else {
                    WebFont webFont(isFontStyleSpecified, isFontWeightSpecified,
                                    fontFamily, style, weight,
                                    std::get<0>(fontFaceData));
                    document()->m_webFontList.push_back(webFont);
                }
            }
#endif
        }

        // resolve style
        INSTALL_PROFILE_TIMER("resolve style");

        m_styleResolveStartTick = tickCount();
        document()->styleResolver().resolveDOMStyle(
            document(), m_needsStyleRecalcForWholeDocument);
        m_needsStyleRecalc = false;
        m_needsStyleRecalcForWholeDocument = false;

        if (document()->animationExecutor()->activeTransitions().size() > 0) {
            auto& l = document()->animationExecutor()->activeTransitions();
            uint64_t currentTick = tickCount();
            bool canceled = false;
            for (size_t i = 0; i < l.size(); i++) {
                if (webView()->inRendering() != true) {
                    // when in rendering, start time is updated by
                    // WebView::rendering()
                    // because other steps(painting, layout) can take too
                    // long(ex. longer than duration)
                    l[i]->initializeStartTimeIfNeeded(currentTick);
                }
                if ((l[i]->targetElement()->isInDocumentScope() == false) ||
                    (l[i]->targetElement()->style() == nullptr) ||
                    l[i]->targetElement()->style()->display() ==
                        DisplayValue::NoneDisplayValue) {
                    canceled = true;
                    l[i]->fireTransitionCancelEvent();
                    l[i]->detachFromElement(nullptr);
                    l.erase(i);
                    i--;
                }
            }

            if (canceled == true) {
                document()->animationExecutor()->checkActiveExecutorInWebView();
            }

            if (webView()->inRendering() == false) {
                webView()->setNeedsRendering();
            }
        }

        if (document()->animationExecutor()->activeAnimations().size() > 0) {
            auto& animations =
                document()->animationExecutor()->activeAnimations();
            for (auto& animation : animations) {
                uint64_t currentTick = tickCount();
                uint64_t cancelTick = 0;
                bool canceled = false;
                for (auto task = animation.second.begin();
                     task != animation.second.end();) {
                    if (webView()->inRendering() != true) {
                        // when in rendering, start time is updated by
                        // WebView::rendering()
                        // because other steps(painting, layout) can take too
                        // long(ex. longer than duration)
                        (*task)->initializeStartTimeIfNeeded(currentTick);
                    }
                    if (((*task)->targetElement()->isInDocumentScope() ==
                         false) ||
                        ((*task)->targetElement()->style() == nullptr) ||
                        (*task)->targetElement()->style()->display() ==
                            DisplayValue::NoneDisplayValue) {
                        canceled = true;
                        (*task)->detachFromElement(nullptr);
                        float progress = (*task)->fraction(currentTick);
                        cancelTick = (*task)->duration() * progress / 1000;
                        task = animation.second.erase(task);
                    } else {
                        task++;
                    }
                }
                if (canceled == true) {
                    Element* e = animation.first->m_element;
                    String* n = animation.first->m_name;
                    document()->animationExecutor()->fireAnimationCancelEvent(
                        e, n, cancelTick);
                    document()
                        ->animationExecutor()
                        ->checkActiveExecutorInWebView();
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
        // Update shadow tree for SVGUseElement
        document()->updateShadowTreeForUseElement();

        if (document()->frame()) {
            // create frame tree
            INSTALL_PROFILE_TIMER("create frame tree");

            FrameTreeBuilder::buildFrameTree(document());
            m_needsLayout = true;
            m_needsFrameTreeBuild = false;
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
        INSTALL_PROFILE_TIMER("layout frame tree");

        LayoutContext ctx(starfish(), document()
                                          ->frame()
                                          ->asFrameBox()
                                          ->asFrameBlockBox()
                                          ->asFrameDocument());

        document()->frame()->layout(ctx,
                                    Frame::LayoutWantToResolve::ResolveAll);

        registerDidLayoutInWebView();

        webView()->setNeedsComputeStackingContextProperties();
        m_needsLayout = false;
        ret = true;
    }

    if (document()->animationExecutor()->activeAnimations().size() != 0) {
        auto& activeAnimations =
            document()->animationExecutor()->activeAnimations();
        auto iter = activeAnimations.begin();
        while (iter != activeAnimations.end()) {
            auto direction = iter->first->m_direction;
            auto iterationCount = iter->first->m_iterationCount;
            bool isOddIteration;
            auto& l = iter->second;
            for (size_t i = 0; i < l.size(); i++) {
                if (std::isinf(iterationCount) == false) {
                    isOddIteration =
                        std::fmod(iterationCount - l[i]->iterationStart() + 1,
                                  2) >= 1;
                } else {
                    isOddIteration = std::fmod(l[i]->iterationStart(), 2) >= 1;
                }
                bool isForwardDirection =
                    (direction ==
                     AnimationDirectionValue::AnimationDirectionNormalValue) ||
                    (direction == AnimationDirectionValue::
                                      AnimationDirectionAlternateValue &&
                     isOddIteration) ||
                    (direction == AnimationDirectionValue::
                                      AnimationDirectionAlternateReverseValue &&
                     !isOddIteration);

                l[i]->setIsForward(isForwardDirection);
                l[i]->resolveUnresolvedAnimatedValues();
            }
            iter++;
        }
    }

    document()->resourceLoader().cachePruning();

    return ret;
}

template <typename T>
void BrowsingContext::clearingBeforePaint(T canvas)
{
#ifdef STARFISH_TIZEN
    if (!document()->tizenWidgetTransparentBackground()) {
        if (document()->browsingContext()->isTopLevelBrowsingContext()) {
            canvas->clearColor(webView()->baseBackgroundColor());
        }
    } else {
        canvas->clearColor(Unit::Color(0, 0, 0, 0));
    }
#else
    if (document()->browsingContext()->isTopLevelBrowsingContext()) {
        canvas->clearColor(webView()->baseBackgroundColor());
    }
#endif
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

std::pair<bool, Unit::Color> BrowsingContext::hasWindowBackgroundColor()
{
    if (hasRootElementBackground() || hasBodyElementBackground()) {
        if (hasRootElementBackground() &&
            !document()
                 ->rootElement()
                 ->style()
                 ->backgroundColor()
                 .isTransparent()) {
            HTMLHtmlElement* root = document()->rootElement();
            return std::make_pair(true, root->style()->backgroundColor());
        } else {
            HTMLBodyElement* body = document()->rootElement()->body();
            if (body && !body->style()->backgroundColor().isTransparent()) {
                return std::make_pair(true, body->style()->backgroundColor());
            }
        }
    }
    return std::make_pair(false, Unit::Color());
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
    // STARFISH_LOG_INFO("Window::markHasPendingStyleSheet\n");
    m_pendingStyleSheetCount++;
}

void BrowsingContext::unmarkHasPendingStyleSheet()
{
    // STARFISH_LOG_INFO("Window::unmarkHasPendingStyleSheet\n");
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
        const auto& v = document()->loadedWebFontList();
        for (size_t i = 0; i < v.size(); i++) {
            if (v[i]->fontFace()) {
                v[i]->fontFace()->clearCache();
            }
        }
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
        webView()->platformWindow()->clearResources();
        webView()->messageLoop()->clearPendingIdlers(m_window);

        auto& prevDrawnInfo = webView()->prevDrawnStackingContextInfo();
        auto iter = prevDrawnInfo.begin();
        while (iter != prevDrawnInfo.end()) {
            if (iter->second.graphicsBufferHolder) {
                iter->second.graphicsBufferHolder->detachNativeBuffers();
                iter->second.graphicsBufferHolder = nullptr;
            }
            iter++;
        }
        prevDrawnInfo.clear();

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
#ifdef STARFISH_ENABLE_TEST
        if (webView()->startUpFlag() & StarfishStartUpFlag::enableHitTestDump) {
            printf("hitTest Result-> ");
            frame->node()->dump();
            puts("");
        }
#endif
        return frame->node();
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

// https://www.w3.org/TR/html5/editing.html#focusing-steps
void BrowsingContext::setFocusedNode(Node* n, bool byMouseEvent)
{
    didFocusEvent();

    if (!n->isInDocumentScope() || !n->document()->browsingContext()) {
        return;
    }

    Element* e = n->isElement() ? n->asElement() : n->parentElement();
    if (!e) {
        // If document area is selected.
        releaseFocusedNode(nullptr);
        return;
    } else if (e == m_focusedNode) {
        // If the element is already focused.
        return;
    } else if (e->isHTMLIFrameElement()) {
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

    if (byMouseEvent && !e->isHTMLFormControl()) {
        return;
    }

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
            childBrowsingContext->releaseFocusedNode(nullptr, false);
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
                               GCUnorderedSet<Node*>& set, Node** target,
                               size_t* version, Node::NodeState state)
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

bool BrowsingContext::setHoveredNode(Node* n)
{
    return updateEventNodeSet(
        document(), n, m_hoveredNodeSet, &m_hoveredNodeTarget,
        &m_documentVersionWhenComputingHoveredNodeSet, Node::NodeStateHovered);
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
        auto absPoint = fb->absolutePoint(document()->frame()->asFrameBox());
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

    Node* oldTarget = m_hoveredNodeTarget;
    if (setHoveredNode(targetNode)) {
        Node* newTarget = m_hoveredNodeTarget;
        if (newTarget != oldTarget) {
            Node* newElement = newTarget->nearestParentElement();
            Node* oldElement =
                oldTarget ? oldTarget->nearestParentElement() : nullptr;

            if (newElement && newElement->isElement()) {
                MouseData data(button, buttons, posX, posY, 0, oldElement);
                Element* enterTarget = newElement->asElement();
                while (enterTarget) {
                    Event* e = createMouseEvent(
                        document(),
                        starfish()->staticStrings()->m_mouseenter.localName(),
                        data);
                    e->setCancelable(false);
                    e->setBubbles(false);
                    enterTarget->dispatchEventByUA(newElement, e, true);
                    enterTarget = enterTarget->parentElement();
                }
            }

            {
                String* name =
                    starfish()->staticStrings()->m_mouseover.localName();
                MouseData data(button, buttons, posX, posY, 0, oldElement);
                Event* e = createMouseEvent(document(), name, data);
                document()->window()->dispatchEventByUA(
                    newElement ? newElement : document(), e);
            }

            {
                String* name =
                    starfish()->staticStrings()->m_mouseout.localName();
                MouseData data(button, buttons, posX, posY, 0, newElement);
                Event* e = createMouseEvent(document(), name, data);
                document()->window()->dispatchEventByUA(
                    oldElement ? oldElement : document(), e);
            }

            if (oldElement && oldElement->isElement()) {
                MouseData data(button, buttons, posX, posY, 0, newElement);
                Element* leaveTarget = oldElement->asElement();
                while (leaveTarget) {
                    Event* e = createMouseEvent(
                        document(),
                        starfish()->staticStrings()->m_mouseleave.localName(),
                        data);
                    e->setCancelable(false);
                    e->setBubbles(false);
                    leaveTarget->dispatchEventByUA(oldElement, e, true);
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
        return false;
    }
    if (count < 1) {
        return false;
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
    double newX = targetX;
    double newY = targetY;
    if (isInnerIFrameEvent(targetNode, newX, newY)) {
        handleActiveAndFocus((MouseEventKind)kind, targetNode, targetX,
                             targetY);

        TouchData newData(newX, newY);
        if (targetNode->asHTMLIFrameElement()
                ->browsingContext()
                ->dispatchTouchEvent(kind, &newData, 1)) {
            return true;
        }
    }

    bool returnValue = false;
    // Dispatch events
    String* name = String::emptyString;
    switch (kind) {
    case TouchEventKind::TouchEventStart: {
        // Dispatch touchstart event
        name = starfish()->staticStrings()->m_touchstart.localName();
        Event* e = createTouchEvent(document(), name, touches, count);
        Node* t = targetNode->nearestParentElement();
        t = t ? t : document();
        returnValue = !document()->window()->dispatchEventByUA(t, e);
        break;
    }
    case TouchEventKind::TouchEventMove: {
        // Dispatch touchmove event
        name = starfish()->staticStrings()->m_touchmove.localName();
        Event* e = createTouchEvent(document(), name, touches, count);
        Node* t = targetNode->nearestParentElement();
        t = t ? t : document();
        returnValue = !document()->window()->dispatchEventByUA(t, e);
        break;
    }
    case TouchEventKind::TouchEventEnd: {
        Node* t = targetNode->nearestParentElement();
        if (m_activeNodeTarget == targetNode) {
            // Dispatch click event
            Node* t = targetNode->nearestParentElement();
            t = t ? t : document();
            name = starfish()->staticStrings()->m_click.localName();
            MouseData clickData(MouseButtonValue::LeftButton,
                                MouseButtonsValue::LeftButtonDown, targetX,
                                targetY, 1);
            Event* click = createMouseEvent(document(), name, clickData);
            document()->window()->dispatchEventByUA(t, click);
        }
        // Dispatch touchend event
        name = starfish()->staticStrings()->m_touchend.localName();
        Event* e = createTouchEvent(document(), name, touches, count);
        returnValue = !document()->window()->dispatchEventByUA(t, e);
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
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
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

    // STARFISH_LOG_INFO("BrowsingContext::dispatchMouseEvent %d %f %f %d\n",
    // (int)kind, data.clientX(), data.clientY(), (int)data.buttons());

    data.setClientX(data.clientX() + window()->scrollX(false));
    data.setClientY(data.clientY() + window()->scrollY(false));

    // Hit test to validate event position
    Node* targetNode = hitTest((float)data.clientX(), (float)data.clientY());
    if (!targetNode) {
        return false;
    }
    double targetX = data.clientX();
    double targetY = data.clientY();
    double newX = targetX;
    double newY = targetY;

    // Handle event inside iframe
    if (isInnerIFrameEvent(targetNode, newX, newY)) {
        handleActiveAndFocus(kind, targetNode, targetX, targetY);
        handleHover(kind, targetNode, data.button(), data.buttons(), targetX,
                    targetY);

        MouseData newData(data.button(), data.buttons(), newX, newY, 0);
        if (targetNode->asHTMLIFrameElement()
                ->browsingContext()
                ->dispatchMouseEvent(kind, newData)) {
            return true;
        }
    }

    bool returnValue = false;
    // Dispatch events
    String* name = String::emptyString;
    Node* t = targetNode->nearestParentElement();
    t = t ? t : document();
    switch (kind) {
    case MouseEventKind::MouseEventDown: {
        // Dispatch mousedown event
        name = starfish()->staticStrings()->m_mousedown.localName();
        MouseData downData(data);
        downData.setRelatedTarget(nullptr);
        Event* e = createMouseEvent(document(), name, downData);
        returnValue = !document()->window()->dispatchEventByUA(t, e);
        break;
    }
    case MouseEventKind::MouseEventMove: {
        // Dispatch mousemove event
        name = starfish()->staticStrings()->m_mousemove.localName();
        MouseData mvData(data);
        mvData.setRelatedTarget(nullptr);
        Event* e = createMouseEvent(document(), name, mvData);
        returnValue = !document()->window()->dispatchEventByUA(t, e);
        break;
    }
    case MouseEventKind::MouseEventUp: {
        // Dispatch mouseup event
        name = starfish()->staticStrings()->m_mouseup.localName();
        MouseData upData(data);
        upData.setRelatedTarget(nullptr);
        Event* mouseup = createMouseEvent(document(), name, upData);
        returnValue = !document()->window()->dispatchEventByUA(t, mouseup);

        if (m_activeNodeTarget == t) {
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
                            t += z * 15;
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
                            t += z * 15;
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
    OverflowValue ox = document()->appliedOverflowX();
    OverflowValue oy = document()->appliedOverflowY();

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
        if (!String::isASCIIPrintableKey(pkdata.keyValue())) {
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

    pkdata.setEventModifierData(
        webView()->platformWindow()->eventModifierData());

    KeyboardEventInit kinitData(pkdata);
    KeyboardEvent* e =
        new KeyboardEvent(document()->executionContext(), eventType, kinitData);
    e->setBubbles(true);
    e->setCancelable(true);
    e->setView(document()->window());
    document()->window()->dispatchEventByUA(target, e);

    if (!e->defaultPrevented()) {
        if (kind == KeyEventKind::KeyEventDown) {
            if (e->keyValue() == KeyValue::TabKey) {
                if (e->shiftKey()) {
                    focusNavigation(false);
                } else {
                    focusNavigation();
                }
                e->defaultPrevented();
            } else if (e->keyValue() == KeyValue::EnterKey ||
                       e->keyValue() == KeyValue::SpaceKey) {
                String* eventType =
                    starfish()->staticStrings()->m_click.localName();
                Node* t = webView()->focusedNode();
                if (t) {
                    t = t->nearestParentElement();
                    t->dispatchEventByUA(
                        new Event(t->document()->executionContext(), eventType,
                                  EventInit(true, true)));
                    e->defaultPrevented();
                }
            } else if (e->keyValue() >= KeyValue::ArrowDownKey &&
                       e->keyValue() <= KeyValue::ArrowRightKey) {
#if defined(STARFISH_ANDROID)
                focusNavigationWithArrow(e);
                e->defaultPrevented();
#else
                double sx = window()->scrollX(false);
                double sy = window()->scrollY(false);
                OverflowValue ox = document()->appliedOverflowX();
                OverflowValue oy = document()->appliedOverflowY();

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

    // After editing, this 'oninput' event is called.
    if (target->isHTMLInputElement()) {
        InputEvent* event = new InputEvent(document()->executionContext(),
                                           String::createASCIIString("input"));
        event->setCancelable(false);
        event->setBubbles(true);
        event->setComposed(true);
        event->setData(target->asHTMLInputElement()->value());
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
                                               String* data, Node* node)
{
    // Set target
    // 1) currently focused element if possible
    // or 2) body element if possible
    // or 3) root element
    Node* target;
    if (node) {
        target = node;
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
                    ->dispatchCompositionEvent(kind, data);
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

void BrowsingContext::setNeedsFullPainting()
{
    if (document()->frame() != nullptr) {
        FrameBox* fb = document()->frame()->asFrameBox();
        fb->markNeedsPainting();
        fb->iterateChildFrameBox([](FrameBox* fb) {
            STARFISH_ASSERT(fb);
            fb->markNeedsPainting();
        });

        setNeedsPainting();
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
}
