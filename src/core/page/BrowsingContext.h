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

#ifndef __StarFishBrowsingContext__
#define __StarFishBrowsingContext__

#include "StarFishConfig.h"

#include "binding/StarFishHoldable.h"
#include "browser/history/HistoryManager.h"
#include "platform/window/PlatformWindow.h"

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
class MouseData;
class TouchData;
class KeyboardData;

class BrowsingContext : public gc, public StarFishHoldable {
    friend class PlatformWindow;
    friend class Window;
    friend class HTMLHtmlElement;
    friend class HTMLBodyElement;
    friend class HTMLLinkElement;
    friend class WebView;
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
        STARFISH_ASSERT(!isMainBrowsingContext());
        return m_sourceElement;
    }

    void navigate(ResourceURL* url, HistoryManager::Action type);

    void pause();
    void resume();
    void close();

    void setNeedsStyleRecalc()
    {
        if (!m_needsStyleRecalc) {
            m_needsStyleRecalc = true;
            setNeedsRendering();
            registerNeedsLayoutInWebView();
        }
    }

    void setWholeDocumentNeedsStyleRecalc();

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

    Node* hitTest(float x, float y);

    bool hasPendingStyleSheet()
    {
        return m_pendingStyleSheetCount;
    }
    void markHasPendingStyleSheet();
    void unmarkHasPendingStyleSheet();

    bool isInnerIFrameEvent(Node* targetNode, double& posX, double& posY);
    void handleActiveAndFocus(PlatformWindow::MouseEventKind kind,
                              Node* targetNode, double posX, double posY);
    void handleHover(PlatformWindow::MouseEventKind kind, Node* targetNode,
                     unsigned char button, unsigned char buttons, double posX,
                     double posY);

    bool dispatchTouchEvent(PlatformWindow::TouchEventKind kind,
                            TouchData* touches, size_t touchCount);
    bool dispatchMouseEvent(PlatformWindow::MouseEventKind kind,
                            MouseData data);
    bool dispatchMouseWheelEvent(
        float screenX, float screenY, int z,
        bool isVerticalWheelEvent); // z : -1(up, left) or 1(down, right)
    void dispatchKeyEvent(PlatformWindow::KeyEventKind kind,
                          KeyboardData& data);

    bool setActiveNode(Node* n);
    void releaseActiveNode();

    bool setHoveredNode(Node* n);
    void releaseHoveredNode();

    // TODO: Nested browsing contexts should be considered.
    Node* focusedNode();
    void setFocusedNode(Node* n);
    void releaseFocusedNode(Node* n);

    void paintWindowBackground(Canvas* canvas);

    bool isMainBrowsingContext()
    {
        return m_parentBrowsingContext == nullptr;
    }

    BrowsingContext* parentBrowsingContext()
    {
        return m_parentBrowsingContext;
    }

    void addGlobalPointingEventInterceptListener(EventTarget* node);
    void removeGlobalPointingEventInterceptListener(EventTarget* node);

    void addPointerInRootSet(void* ptr);
    void removePointerFromRootSet(void* ptr);
#ifndef NDEBUG
    size_t countPointersInRootSet(void* ptr);
#endif
private:
    // return did layout
    bool layoutIfNeeds();

    void iterateChildContext(const std::function<void(BrowsingContext*)>& fn);

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
    bool m_needsFrameTreeBuild;
    bool m_needsLayout;

    size_t m_pendingStyleSheetCount;
    size_t m_pendingRenderingCount;

    bool m_isRunning;
    bool m_isActive;

    Unit::Location m_touchDownPoint;

    GCUnorderedSet<Node*> m_activeNodeSet;
    Node* m_activeNodeTarget;
    size_t m_documentVersionWhenComputingActiveNodeSet;

    GCUnorderedSet<Node*> m_hoveredNodeSet;
    Node* m_hoveredNodeTarget;
    size_t m_documentVersionWhenComputingHoveredNodeSet;

    Node* m_focusedNode;

    bool m_hasRootElementBackground;
    bool m_hasBodyElementBackground;

    GCVector<EventTarget*> m_globalPointingEventListener;
    GCUnorderedMap<void*, size_t> m_rootMap;
};
}

#endif
