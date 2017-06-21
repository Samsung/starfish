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

class BrowsingContext : public gc, public StarFishHoldable {
    friend class PlatformWindow;
    friend class Window;
    friend class HTMLHtmlElement;
    friend class HTMLBodyElement;
    friend class HTMLLinkElement;

public:
    static BrowsingContext* create(StarFish* starFish, WebView* webView);
    virtual ~BrowsingContext()
    {
    }

    void initFlags();
    ScriptBindingInstance* scriptBindingInstance();

    Document* document()
    {
        return m_document;
    }

    WebView* webView()
    {
        return m_webView;
    }

    void navigate(ResourceURL* url);
    void navigateAsync(ResourceURL* url);

    void clearStackingContext(bool backupBuffer);

    void pause();
    void resume();
    void close();

    void rendering();

    void layoutIfNeeds();

    bool inRendering()
    {
        return m_inRendering;
    }

    void setNeedsStyleRecalc()
    {
        if (!m_needsStyleRecalc) {
            m_needsStyleRecalc = true;
            setNeedsRendering();
        }
    }

    void setWholeDocumentNeedsStyleRecalc();

    void setNeedsFrameTreeBuild()
    {
        if (!m_needsFrameTreeBuild) {
            m_needsFrameTreeBuild = true;
            setNeedsRendering();
        }
        setNeedsLayout();
    }

    void setNeedsLayout()
    {
        if (!m_needsLayout) {
            m_needsLayout = true;
            setNeedsRendering();
        }
        setNeedsPainting();
    }

    void setNeedsPainting()
    {
        if (!m_needsPainting) {
            m_needsPainting = true;
            setNeedsRendering();
        }
    }

    void setNeedsComposite()
    {
        if (!m_needsComposite) {
            m_needsComposite = true;
            setNeedsRendering();
        }
    }

    void renderingIfNeeds()
    {
        if (m_needsRendering) {
            rendering();
            m_needsRendering = false;
        }
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

    void markHasPendingStyleSheet();
    void unmarkHasPendingStyleSheet();

    void dispatchTouchEvent(float x, float y,
                            PlatformWindow::TouchEventKind kind, bool isMobile);
    void dispatchMouseEvent(float x, float y,
                            PlatformWindow::MouseEventKind kind);
    void dispatchKeyEvent(String* key, PlatformWindow::KeyEventKind kind);

    void setActiveNode(Node* n);
    void releaseActiveNode();

    void setFocusedNode(Node* n);
    void releaseFocusedNode();

    void setHoveredNode(Node* n);
    void releaseHoveredNode();

    void paintWindowBackground(Canvas* canvas);

private:
    BrowsingContext(StarFish* starFish, WebView* webView);

    void setNeedsRenderingSlowCase();

    void setNeedsRendering()
    {
        if (m_needsRendering) {
            return;
        }
        setNeedsRenderingSlowCase();
    }

    WebView* m_webView;
    Document* m_document;
    BrowsingContext* m_mainBrowsingContext;

    bool m_inRendering;
    bool m_needsRendering;
    bool m_needsStyleRecalc;
    bool m_needsStyleRecalcForWholeDocument;
    bool m_needsFrameTreeBuild;
    bool m_needsLayout;
    bool m_needsPainting;
    bool m_needsComposite;

    size_t m_pendingStyleSheetCount;
    size_t m_pendingRenderingCount;

    StackingContext* m_rootStackingContext;
    GCVector<CanvasSurface*> m_backStackingContextBufferUpWhileReCompsite;

    bool m_isRunning;
    bool m_isActive;

    Unit::Location m_touchDownPoint;

    GCVector<Node*> m_activeNodes;
    GCVector<Node*> m_hoveredNodes;

    int m_ctrlKeyDown;
    int m_shiftKeyDown;
    int m_altKeyDown;
    int m_metaKeyDown;

    uint64_t m_lastRenderingTime;

    Node* m_focusedNode;
    Node* m_relatedTarget;

    bool m_hasRootElementBackground;
    bool m_hasBodyElementBackground;
};
}

#endif
