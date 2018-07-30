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

#ifndef __StarFishBrowsingContext__
#define __StarFishBrowsingContext__

#include "binding/StarFishHoldable.h"
#include "browser/history/HistoryManager.h"

namespace StarFish {

class Document;
class Node;
class WebView;
class ResourceURL;
class ScriptBindingInstance;
class StarFish;
class StackingContext;
class CanvasSurface;
class PlatformWindow;
class Window;
class HTMLIFrameElement;
#ifdef STARFISH_ENABLE_MULTIMEDIA
class HTMLMediaElement;
#endif
class MouseData;
class TouchData;
class PlatformKeyEventData;
class Canvas;

enum class TouchEventKind;
enum class KeyEventKind;
enum class MouseEventKind;
enum class CompositionEventKind;

class BrowsingContext : public gc, public StarFishHoldable {
    friend class PlatformWindow;
    friend class Window;
    friend class HTMLHtmlElement;
    friend class HTMLBodyElement;
    friend class HTMLLinkElement;
    friend class WebView;
    friend class HTMLIFrameElement;
    friend class FrameReplacedIFrame;

public:
    static BrowsingContext* create(StarFish* starFish, WebView* webView);
    static BrowsingContext* create(HTMLIFrameElement* sourceElement);
    virtual ~BrowsingContext()
    {
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
        setNeedsPainting();
    }

    void setNeedsPainting();
    void setNeedsComposite();

    bool needsFrameTreeBuild()
    {
        return m_needsFrameTreeBuild;
    }

    bool needsLayout()
    {
        return m_needsLayout;
    }

    bool hasRootElementBackground()
    {
        return m_hasRootElementBackground;
    }

    bool hasBodyElementBackground()
    {
        return m_hasBodyElementBackground;
    }

    bool isActive()
    {
        return m_isActive;
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
                                  Node* node = nullptr);

    bool setActiveNode(Node* n);
    void releaseActiveNode();

    bool setHoveredNode(Node* n);
    void releaseHoveredNode();

    Node* focusedNode();
    void setFocusedNode(Node* n, bool byMouseEvent);
    void releaseFocusedNode(Node* n, bool resetActiveElement = true);
    Element* activeElement();

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

    void clearingBeforePaint(Canvas* canvas);
    void paintWindowBackground(Canvas* canvas);

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

    // starting global pointing Intercept must use default event.
    void addGlobalPointingEventInterceptListener(EventTarget* node);
    void removeGlobalPointingEventInterceptListener(EventTarget* node);

    void addPointerInRootSet(void* ptr);
    void removePointerFromRootSet(void* ptr);
#ifndef NDEBUG
    size_t countPointersInRootSet(void* ptr);
#endif
    void resolveStyleIfNeeds();
    void buildFrameTreeIfNeeds();
    // return did layout
    bool layoutIfNeeds();
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

    void notifyHasPendingAnimation();

private:
    // Don't call function directly
    // you can use this function from WebView::navigate or
    // HTMLIFrameElement::navigate
    void open(ResourceURL* url, HistoryManager::Action type,
              ResourceURL* referrerURL);

    void didFocusEvent();
    void iterateChildContext(const std::function<void(BrowsingContext*)>& fn);
    void focusNavigation(bool forward = true);

    BrowsingContext(StarFish* starFish, WebView* webView,
                    HTMLIFrameElement* source = nullptr);

    void setNeedsRendering();
    void registerNeedsLayoutInWebView();
    void unRegisterNeedsLayoutInWebView();

    WebView* m_webView;
    Window* m_window;

    BrowsingContext* m_parentBrowsingContext;
    HTMLIFrameElement* m_sourceElement;

    bool m_needsStyleRecalc;
    bool m_needsStyleRecalcForWholeDocument;
    bool m_needsStyleSheetsRecalc;
    bool m_needsFrameTreeBuild;
    bool m_needsLayout;

    size_t m_pendingStyleSheetCount;
    size_t m_pendingRenderingCount;

    bool m_isRunning;
    bool m_isActive; // true means this context is alive

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

    bool m_keydownEventDefaultPrevented;
    bool m_compositionStartEventDefeaultPrevented;

    bool m_hasRootElementBackground;
    bool m_hasBodyElementBackground;

    GCVector<EventTarget*> m_globalPointingEventListener;
    GCUnorderedMap<void*, size_t> m_rootMap;
#ifdef STARFISH_ENABLE_MULTIMEDIA
    GCVector<HTMLMediaElement*> m_existingMediaElements;
#endif
    String* m_name;
};
}

#endif
