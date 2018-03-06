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
#include "core/modules/message_loop/Timer.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/threading/Thread.h"
#include "core/util/URL.h"
#include "platform/window/PlatformWindow.h"
#include "core/animation/Animation.h"
#include "core/dom/Traverse.h"
#include "core/dom/HTMLIFrameElement.h"
#include "platform/loader/ResourceLoader.h"

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
    , m_touchDownPoint(0, 0)
    , m_activeNodeTarget(nullptr)
    , m_documentVersionWhenComputingActiveNodeSet(0)
    , m_hoveredNodeTarget(nullptr)
    , m_documentVersionWhenComputingHoveredNodeSet(0)
    , m_focusedNode(nullptr)
    , m_activeElement(nullptr)
    , m_name(String::emptyString)
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
    m_isRunning = true;
    m_pendingStyleSheetCount = 0;
}

void BrowsingContext::open(ResourceURL* url, HistoryManager::Action type,
                           ResourceURL* referrerURL)
{
    dispose();
    initFlags();

    m_isActive = true;

    StarFishEnterer enter(m_starFish);

    if (isTopLevelBrowsingContext()) {
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

    m_window->document()->init(referrerURL);

    // STARFISH_LOG_INFO("BrowsingContext::open %s\n",
    // url->urlString()->toUTF8String().data());

    switch (type) {
    case HistoryManager::Action::Add:
        historyManager()->push(document(), url);
        break;
    case HistoryManager::Action::Replace:
        historyManager()->replace(document(), url);
        break;
    case HistoryManager::Action::Intact:
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

ScriptBindingInstance* BrowsingContext::scriptBindingInstance()
{
    return window()->scriptBindingInstance();
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
        resource()->loader()->document()->setNeedsPainting();
    }

    virtual void didLoadFinished()
    {
        ResourceClient::didLoadFinished();
        resource()
            ->loader()
            ->document()
            ->browsingContext()
            ->setWholeDocumentNeedsStyleRecalc();
        resource()->loader()->document()->setNeedsLayout();
        resource()->loader()->document()->fontSelector()->clearCache(
            m_familyName);
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
            INSTALL_PROFILE_TIMER(starFish(), "parse sheet & collect rules");

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
            }

#if !defined(PORT_CANVAS_BACKEND_EFL) && !defined(PORT_CANVAS_BACKEND_MOCK)
            for (size_t i = 0; i < webFonts.size(); i++) {
                CSSStyleDeclaration* decl = webFonts[i];
                if (decl->getCSSValuePair(
                            CSSStyleValuePair::KeyKind::FontFamily)
                        .valueKind() !=
                    CSSStyleValuePair::ValueKind::StringValueKind) {
                    continue;
                }
                String* fontFamily =
                    decl->getCSSValuePair(
                            CSSStyleValuePair::KeyKind::FontFamily)
                        .stringValue();
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
                        res->request(Resource::SyncIfAlreadyLoaded,
                                     document()->documentURI(), true);
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
        INSTALL_PROFILE_TIMER(starFish(), "resolve style");

        document()->styleResolver().resolveDOMStyle(
            document(), m_needsStyleRecalcForWholeDocument);
        m_needsStyleRecalc = false;
        m_needsStyleRecalcForWholeDocument = false;
    }
}

void BrowsingContext::buildFrameTreeIfNeeds(bool fromWebView)
{
    resolveStyleIfNeeds();
    if (m_needsFrameTreeBuild) {
        if (document()->frame()) {
            if (fromWebView) {
                webView()->clearStackingContext(true);
            }

            // create frame tree
            INSTALL_PROFILE_TIMER(starFish(), "create frame tree");

            FrameTreeBuilder::buildFrameTree(document());
            m_needsLayout = true;
            m_needsFrameTreeBuild = false;
        }
    }
}

bool BrowsingContext::layoutIfNeeds(bool fromWebView)
{
    resolveStyleIfNeeds();
    buildFrameTreeIfNeeds();

    bool ret = false;
    if (m_needsLayout) {
        if (fromWebView) {
            webView()->clearStackingContext(true);
        }

        // lay out frame tree
        INSTALL_PROFILE_TIMER(starFish(), "lay out frame tree");

        LayoutContext ctx(starFish(), document()
                                          ->frame()
                                          ->asFrameBox()
                                          ->asFrameBlockBox()
                                          ->asFrameDocument());
        document()->frame()->layout(ctx,
                                    Frame::LayoutWantToResolve::ResolveAll);
        m_needsLayout = false;
        webView()->setNeedsComputeStackingContextProperties();
        ret = true;
    }

    return ret;
}

void BrowsingContext::clearingBeforePaint(Canvas* canvas)
{
#ifdef STARFISH_TIZEN
    if (!document()->tizenWidgetTransparentBackground()) {
        if (document()->browsingContext()->isTopLevelBrowsingContext()) {
#ifdef STARFISH_TIZEN_WEARABLE
            canvas->clearColor(Unit::Color(0, 0, 0, 255));
#else
            canvas->clearColor(Unit::Color(255, 255, 255, 255));
#endif
        }
    } else {
        canvas->clearColor(Unit::Color(0, 0, 0, 0));
    }
#else
    if (document()->browsingContext()->isTopLevelBrowsingContext())
        canvas->clearColor(Unit::Color(255, 255, 255, 255));
#endif
}

void BrowsingContext::paintWindowBackground(Canvas* canvas)
{
    clearingBeforePaint(canvas);

    if (!document()->rootElement()) {
        return;
    }

    if (m_hasRootElementBackground || m_hasBodyElementBackground) {
        LayoutRect colorRect(0, 0, document()->window()->innerWidth(),
                             document()->window()->innerHeight());
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
    if (m_window) {
        GCVector<Element*> iframeCollection;
        Traverse::collectDescendants(
            iframeCollection, document(),
            [&](Element* element) { return element->isHTMLIFrameElement(); },
            false);

        for (size_t i = 0; i < iframeCollection.size(); i++) {
            iframeCollection[i]->asHTMLIFrameElement()->unloadSrc();
        }
    }

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

    m_globalPointingEventListener.clear();

    if (isTopLevelBrowsingContext())
        m_starFish->timer()->clear(nullptr);
    else
        m_starFish->timer()->clear(this);

    if (m_window) {
        StarFishEnterer enter(m_starFish);
        if (!document()->onLoadFired()) {
            document()
                ->resourceLoader()
                .decreasePendingResourceCountWhileDocumentOpening();
        }
        document()->window()->dispose();
        document()->dispose();

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

    if (isTopLevelBrowsingContext()) {
        m_starFish->platformWindow()->clearResources();
        m_starFish->messageLoop()->clearPendingIdlers(nullptr);
        webView()->clearStackingContext(false);

        m_webView->initRenderingFlags();
    } else {
        m_starFish->messageLoop()->clearPendingIdlers(this);
    }

    m_rootMap.clear();
    unRegisterNeedsLayoutInWebView();
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
void BrowsingContext::setFocusedNode(Node* n)
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

    m_focusedNode = e->asNode();
    m_activeElement = e;

    e->setState(Node::NodeStateFocused, true);

    // focus event
    String* eventType = starFish()->staticStrings()->m_focus.localName();
    Event* event = new FocusEvent(document(), eventType,
                                  FocusEventInit(false, false, relatedTarget));
    document()->dispatchEventByUA(e->asNode(), event);

    // focusin event
    eventType = starFish()->staticStrings()->m_focusin.localName();
    event = new FocusEvent(document(), eventType,
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
        String* eventType = starFish()->staticStrings()->m_blur.localName();
        Event* event = new FocusEvent(
            document(), eventType, FocusEventInit(false, false, relatedTarget));
        document()->dispatchEventByUA(m_focusedNode, event);

        // focusout event
        eventType = starFish()->staticStrings()->m_focusout.localName();
        event = new FocusEvent(document(), eventType,
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
        setFocusedNode(targetNode);
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
                        starFish()->staticStrings()->m_mouseenter.localName(),
                        data);
                    e->setCancelable(false);
                    e->setBubbles(false);
                    enterTarget->dispatchEventByUA(newElement, e, true);
                    enterTarget = enterTarget->parentElement();
                }
            }

            {
                String* name =
                    starFish()->staticStrings()->m_mouseover.localName();
                MouseData data(button, buttons, posX, posY, 0, oldElement);
                Event* e = createMouseEvent(document(), name, data);
                document()->window()->dispatchEventByUA(
                    newElement ? newElement : document(), e);
            }

            {
                String* name =
                    starFish()->staticStrings()->m_mouseout.localName();
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
                        starFish()->staticStrings()->m_mouseleave.localName(),
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
    if (!m_isRunning) {
        return false;
    }

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
            releaseActiveNode();
        }
        return true;
    }

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
        name = starFish()->staticStrings()->m_touchstart.localName();
        Event* e = createTouchEvent(document(), name, touches, count);
        Node* t = targetNode->nearestParentElement();
        t = t ? t : document();
        returnValue = !document()->window()->dispatchEventByUA(t, e);
        break;
    }
    case TouchEventKind::TouchEventMove: {
        // Dispatch touchmove event
        name = starFish()->staticStrings()->m_touchmove.localName();
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
            name = starFish()->staticStrings()->m_click.localName();
            MouseData clickData(MouseData::MouseButtonValue::LeftButton,
                                MouseData::MouseButtonsValue::LeftButtonDown,
                                targetX, targetY, 1);
            Event* click = createMouseEvent(document(), name, clickData);
            document()->window()->dispatchEventByUA(t, click);
        }
        // Dispatch touchend event
        name = starFish()->staticStrings()->m_touchend.localName();
        Event* e = createTouchEvent(document(), name, touches, count);
        returnValue = !document()->window()->dispatchEventByUA(t, e);
        break;
    }
    default:
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    // Handle properties
    handleActiveAndFocus((MouseEventKind)kind, targetNode, targetX, targetY);
    return returnValue;
}

bool BrowsingContext::dispatchMouseEvent(MouseEventKind kind, MouseData data)
{
    if (!m_isRunning) {
        return false;
    }
    // MouseEventEnter/MouseEventOut are not supported yet
    if (kind >= MouseEventKind::MouseEventEnter) {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        return false;
    }

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
            releaseActiveNode();
        }
        return true;
    }

    // STARFISH_LOG_INFO("BrowsingContext::dispatchMouseEvent %d %f %f\n",
    // (int)kind, data.clientX(), data.clientY());

    data.setClientX(data.clientX() + window()->scrollX());
    data.setClientY(data.clientY() + window()->scrollY());

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
        name = starFish()->staticStrings()->m_mousedown.localName();
        Event* e = createMouseEvent(document(), name, data);
        returnValue = !document()->window()->dispatchEventByUA(t, e);
        break;
    }
    case MouseEventKind::MouseEventMove: {
        // Dispatch mousemove event
        name = starFish()->staticStrings()->m_mousemove.localName();
        Event* e = createMouseEvent(document(), name, data);
        returnValue = !document()->window()->dispatchEventByUA(t, e);
        break;
    }
    case MouseEventKind::MouseEventUp: {
        // Dispatch mouseup event
        name = starFish()->staticStrings()->m_mouseup.localName();
        Event* mouseup = createMouseEvent(document(), name, data);
        returnValue = !document()->window()->dispatchEventByUA(t, mouseup);

        if (m_activeNodeTarget == t) {
            // Dispatch click event
            name = starFish()->staticStrings()->m_click.localName();
            Event* click = createMouseEvent(document(), name, data);
            document()->window()->dispatchEventByUA(t, click);
        }
        break;
    }
    default:
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    // Handle properties
    handleActiveAndFocus(kind, t, targetX, targetY);
    handleHover(kind, t, data.button(), data.buttons(), targetX, targetY);
    return returnValue;
}

bool BrowsingContext::dispatchMouseWheelEvent(float screenX, float screenY,
                                              int z, bool isVerticalWheelEvent)
{
    if (!m_isRunning) {
        return false;
    }

    double wx = window()->scrollX() + screenX;
    double wy = window()->scrollY() + screenY;
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
        if (node->isElement()) {
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
        eventType = starFish()->staticStrings()->m_keyup.localName();
        setKeydownEventDefaultPrevented(false);
    } else if (kind == KeyEventKind::KeyEventPress) {
        if (!String::isASCIIPrintableKey(pkdata.keyValue())) {
            return;
        } else if (keydownEventDefaultPrevented()) {
            return;
        }

        eventType = starFish()->staticStrings()->m_keypress.localName();
    } else {
        // kind == KeyEventKind::KeyEventDown
        eventType = starFish()->staticStrings()->m_keydown.localName();
    }

    if (kind != KeyEventKind::KeyEventPress) {
        // For keydown or keyup events, the value of charCode is 0.
        pkdata.setCharCode(0);
    }

    KeyboardEventInit kinitData(pkdata);
    KeyboardEvent* e = new KeyboardEvent(document(), eventType, kinitData);
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
                    starFish()->staticStrings()->m_click.localName();
                Node* t = webView()->focusedNode();
                if (t) {
                    t = t->nearestParentElement();
                    t->dispatchEventByUA(new Event(t->document(), eventType,
                                                   EventInit(true, true)));
                    e->defaultPrevented();
                }
            } else if (e->keyValue() >= KeyValue::ArrowDownKey &&
                       e->keyValue() <= KeyValue::ArrowRightKey) {
                double sx = window()->scrollX();
                double sy = window()->scrollY();
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

                window()->scrollTo(sx, sy);
            }
        }
    }
}

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
        setFocusedNode(focusRing[current]);
    } else {
        releaseFocusedNode(nullptr);
    }
}

void BrowsingContext::dispatchCompositionEvent(CompositionEventKind kind,
                                               String* data)
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
        eventType = starFish()->staticStrings()->m_compositionstart.localName();
    } else if (kind == CompositionEventKind::CompositionEventUpdate) {
        if (compositionStartEventDefaultPrevented()) {
            return;
        }
        eventType =
            starFish()->staticStrings()->m_compositionupdate.localName();
    } else {
        STARFISH_ASSERT(kind == CompositionEventKind::CompositionEventEnd);
        eventType = starFish()->staticStrings()->m_compositionend.localName();
        setCompositionStartEventDefeaultPrevented(false);
    }
    CompositionEvent* e = new CompositionEvent(document(), eventType, data);
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
        return;
    }

    m_isRunning = true;

    document()->setVisibilityState(VisibilityState::VisibilityStateVisible);

    iterateChildContext([](BrowsingContext* ctx) { ctx->resume(); });
}

void BrowsingContext::setNeedsPainting()
{
    m_webView->setNeedsPainting();

    if (m_webView->didCompositeBefore() && document() && document()->frame() &&
        document()->frame()->firstChild()) {
        if (document()
                ->frame()
                ->firstChild()
                ->asFrameBox()
                ->stackingContext()) {
            document()
                ->frame()
                ->firstChild()
                ->asFrameBox()
                ->stackingContext()
                ->setNeedsRepainting();
        }
    }
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

void BrowsingContext::unRegisterNeedsLayoutInWebView()
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

void BrowsingContext::addGlobalPointingEventInterceptListener(EventTarget* node)
{
    m_globalPointingEventListener.insert(m_globalPointingEventListener.end(),
                                         node);
}

void BrowsingContext::removeGlobalPointingEventInterceptListener(
    EventTarget* node)
{
    auto iter = std::find(m_globalPointingEventListener.begin(),
                          m_globalPointingEventListener.end(), node);
    if (iter != m_globalPointingEventListener.end()) {
        m_globalPointingEventListener.erase(iter);
    }
}

void BrowsingContext::addPointerInRootSet(void* ptr)
{
    STARFISH_ASSERT(isMainThread());
    if (!isActive()) {
        return;
    }
    auto iter = m_rootMap.find(ptr);
    if (iter == m_rootMap.end()) {
        m_rootMap.insert(std::make_pair(ptr, 1));
    } else {
        iter->second++;
    }
}

void BrowsingContext::removePointerFromRootSet(void* ptr)
{
    STARFISH_ASSERT(isMainThread());
    if (!isActive()) {
        return;
    }
    auto iter = m_rootMap.find(ptr);
    if (iter != m_rootMap.end()) {
        if (iter->second == 1) {
            m_rootMap.erase(iter);
        } else {
            iter->second--;
        }
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

#ifndef NDEBUG
size_t BrowsingContext::countPointersInRootSet(void* ptr)
{
    auto iter = m_rootMap.find(ptr);
    if (iter != m_rootMap.end()) {
        return iter->second;
    } else {
        return 0;
    }
}
#endif
}
