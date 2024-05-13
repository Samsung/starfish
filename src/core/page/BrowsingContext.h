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

#ifndef __StarfishBrowsingContext__
#define __StarfishBrowsingContext__

#include "binding/WebViewHoldable.h"
#include "core/layout/LayoutRepaintTracker.h"

namespace Starfish {

class Document;
class Node;
class WebView;
class ResourceURL;
class Starfish;
class StackingContext;
class CanvasSurface;
class Renderer;
class Window;
class HTMLIFrameElement;
#ifdef STARFISH_ENABLE_MULTIMEDIA
class HTMLMediaElement;
#endif
class MouseData;
class TouchData;
class PlatformKeyEventData;
class Canvas;
class EventTarget;
class HistoryManager;
class KeyboardEvent;

enum class TouchEventKind;
enum class KeyEventKind;
enum class MouseEventKind;
enum class CompositionEventKind;
enum class HistoryManagerAction;

class BrowsingContext : public gc, public WebViewHoldable {
    friend class Renderer;
    friend class Window;
    friend class HTMLHtmlElement;
    friend class HTMLBodyElement;
    friend class HTMLLinkElement;
    friend class WebView;
    friend class HTMLIFrameElement;
    friend class FrameReplacedIFrame;

public:
    static BrowsingContext* create(WebView* webView);
    static BrowsingContext* create(HTMLIFrameElement* sourceElement,
                                   bool isScriptingEnabled = true);
    virtual ~BrowsingContext()
    {
    }

    virtual bool isDocument() const final
    {
        return true;
    }

    void initFlags();

    ScriptBindingInstance* scriptBindingInstance();

    Window* window()
    {
        return m_window;
    }

    Document* document();

    WebView* webView()
    {
        return m_webView;
    }

    HTMLIFrameElement* sourceElement()
    {
        STARFISH_ASSERT(!isTopLevelBrowsingContext());
        return m_sourceElement;
    }

    void pause();
    void resume();
    void dispose();

    void setNeedsStyleRecalc()
    {
        if (!m_needsStyleRecalc) {
            m_needsStyleRecalc = true;
            setNeedsRendering();
            registerNeedsLayoutInWebView();
        }
    }

    void setWholeDocumentNeedsStyleRecalc();
    void setNeedsStyleSheetsRecalc();
    void setNeedsStyleSheetsRecalcAndWholeDocumentNeedsStyleRecalc();

    void updateDefaultFontSize();

    void setNeedsFrameTreeBuild()
    {
        if (!m_needsFrameTreeBuild) {
            m_needsFrameTreeBuild = true;
            setNeedsRendering();
            registerNeedsLayoutInWebView();
        }
        setNeedsLayout();
    }

    void setNeedsLayout()
    {
        if (!m_needsLayout) {
            m_needsLayout = true;
            setNeedsRendering();
            registerNeedsLayoutInWebView();
        }
    }

    void setNeedsFullLayout();
    void setNeedsPainting();
    void setNeedsFullPainting();
    void setNeedsComposite();

    bool needsFrameTreeBuild()
    {
        return m_needsFrameTreeBuild;
    }

    bool needsLayout()
    {
        return m_needsLayout;
    }

    bool needsStyleRecalc()
    {
        return m_needsStyleRecalc;
    }

    bool needsStyleRecalcForWholeDocument()
    {
        return m_needsStyleRecalcForWholeDocument;
    }

    bool hasRootElementBackground()
    {
        return m_hasRootElementBackground;
    }

    bool hasBodyElementBackground()
    {
        return m_hasBodyElementBackground;
    }

    Node* hitTest(float x, float y);

    bool hasPendingStyleSheet()
    {
        return m_pendingStyleSheetCount;
    }
    void markHasPendingStyleSheet();
    void unmarkHasPendingStyleSheet();

    bool isInnerIFrameEvent(Node* targetNode, double& posX, double& posY);
    void handleActiveAndFocus(MouseEventKind kind, Node* targetNode,
                              double posX, double posY);
    void handleHover(MouseEventKind kind, Node* targetNode,
                     unsigned char button, unsigned char buttons, double posX,
                     double posY);

    bool dispatchTouchEvent(TouchEventKind kind, TouchData* touches,
                            size_t touchCount);
    bool dispatchMouseEvent(MouseEventKind kind, MouseData data);
    bool dispatchMouseWheelEvent(
        float screenX, float screenY, int z,
        bool isVerticalWheelEvent); // z : -1(up, left) or 1(down, right)
    void dispatchKeyEvent(KeyEventKind kind, PlatformKeyEventData& data);
    void dispatchCompositionEvent(CompositionEventKind kind, String* data,
                                  Nullable<Node*> node);

    bool setActiveNode(Node* n);
    void releaseActiveNode();

    bool setHoveredNode(Node* n,
                        Nullable<GCUnorderedSet<Node*>*> oldHoveredNodeSet);
    void releaseHoveredNode();

    Node* focusedNode();
    void setFocusedNode(Node* n, bool byMouseEvent);
    void releaseFocusedNode(Node* n, bool resetActiveElement = true);
    Element* activeElement();
    Node* imageAreaForImage(Frame* cb, float x, float y);

    void setKeydownEventDefaultPrevented(bool b)
    {
        m_keydownEventDefaultPrevented = b;
    }

    bool keydownEventDefaultPrevented() const
    {
        return m_keydownEventDefaultPrevented;
    }

    void setCompositionStartEventDefeaultPrevented(bool b)
    {
        m_compositionStartEventDefeaultPrevented = b;
    }

    bool compositionStartEventDefaultPrevented() const
    {
        return m_compositionStartEventDefeaultPrevented;
    }

    template <typename T>
    void clearingBeforePaint(T canvas);
    void paintWindowBackground(Canvas* canvas);

    std::pair<Nullable<Element*>, Unit::Color> hasWindowBackgroundColor();
    bool rootStackingContextNeedsGraphicsBuffer();

    bool isTopLevelBrowsingContext()
    {
        return m_parentBrowsingContext == nullptr;
    }

    BrowsingContext* parentBrowsingContext()
    {
        return m_parentBrowsingContext;
    }

    bool isDescendantOf(BrowsingContext* other);

    HistoryManager* historyManager();

    void resolveStyleIfNeeds();
    void buildFrameTreeIfNeeds();
    // return did layout
    bool layoutIfNeeded();
    bool isScriptingEnabled()
    {
        return m_isScriptingEnabled;
    }
#ifdef STARFISH_ENABLE_MULTIMEDIA
    void registerMediaElement(HTMLMediaElement* element);
#endif
    void onIdle();

    String* name()
    {
        return m_name;
    }
    void setName(String* name)
    {
        m_name = name;
    }

    uint64_t styleResolveStartTick()
    {
        return m_styleResolveStartTick;
    }

    LayoutRepaintTracker& layoutRepaintTracker()
    {
        return m_layoutRepaintTracker;
    }

    void iterateChildContext(const std::function<void(BrowsingContext*)>& fn);

private:
    // Don't call function directly
    // you can use this function from WebView::navigate or
    // HTMLIFrameElement::navigate
    void open(ResourceURL* url, HistoryManagerAction type,
              ReferrerURL* referrerURL);

    void didFocusEvent();
#if defined(STARFISH_ANDROID)
    void focusNavigationWithArrow(KeyboardEvent*);
#endif
    void focusNavigation(bool forward = true);

    BrowsingContext(WebView* webView, HTMLIFrameElement* source = nullptr,
                    bool isScriptingEnabled = true);

    void setNeedsRendering();
    void registerNeedsLayoutInWebView();
    void unregisterNeedsLayoutInWebView();

    void registerDidLayoutInWebView();
    void unregisterDidLayoutInWebView();

    void computeLayoutPaintingDirty();

    WebView* m_webView;
    Window* m_window;

    BrowsingContext* m_parentBrowsingContext;
    HTMLIFrameElement* m_sourceElement;

    bool m_needsStyleRecalc : 1;
    bool m_needsStyleRecalcForWholeDocument : 1;
    bool m_needsStyleSheetsRecalc : 1;
    bool m_needsFrameTreeBuild : 1;
    bool m_needsLayout : 1;

    bool m_keydownEventDefaultPrevented : 1;
    bool m_compositionStartEventDefeaultPrevented : 1;

    bool m_hasRootElementBackground : 1;
    bool m_hasBodyElementBackground : 1;

    bool m_isScriptingEnabled : 1;

    size_t m_pendingStyleSheetCount;
    size_t m_pendingRenderingCount;

    Unit::Location m_touchDownPoint;
    Unit::Location m_lastMouseMovePoint;

    GCUnorderedSet<Node*> m_activeNodeSet;
    Node* m_activeNodeTarget;
    size_t m_documentVersionWhenComputingActiveNodeSet;

    GCUnorderedSet<Node*> m_hoveredNodeSet;
    Node* m_hoveredNodeTarget;
    size_t m_documentVersionWhenComputingHoveredNodeSet;

    Node* m_focusedNode;
    Element* m_activeElement;

#ifdef STARFISH_ENABLE_MULTIMEDIA
    GCVector<HTMLMediaElement*> m_existingMediaElements;
#endif
    String* m_name;

    uint64_t m_styleResolveStartTick;

    LayoutRepaintTracker m_layoutRepaintTracker;
};
} // namespace Starfish

#endif
