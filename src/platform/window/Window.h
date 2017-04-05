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

#ifndef __StarFishWindow__
#define __StarFishWindow__

#include "util/URL.h"
#include "style/Style.h"
#include "dom/EventTarget.h"

namespace StarFish {

class StarFish;
class Document;
class Window;
class ScriptBindingInstance;
class URL;
class Canvas;
class HTMLCollection;
class StackingContext;
class CanvasSurface;
class History;
class Navigator;
class WebApis;
class AnimationExecutor;

typedef void (*WindowSetTimeoutHandler)(Window* window, void* data);

class Window : public EventTarget {
    friend class MessageLoop;
    friend class HTMLHtmlElement;
    friend class HTMLBodyElement;
    friend class HTMLLinkElement;
    friend class Node;

public:
    static Window* create(StarFish* sf, void* win, int width, int height);
    ~Window();
    void navigate(URL* url);
    void navigateAsync(URL* url);
    void navigateAsyncWithoutSetHistory(URL* url);
    void setHistory(URL* url);

    virtual bool isWindow() const
    {
        return true;
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

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

    Document* document()
    {
        return m_document;
    }

    History* history()
    {
        return m_history;
    }

    Navigator* navigator()
    {
        return m_navigator;
    }

    Location* location()
    {
        return m_location;
    }

    AnimationExecutor* animationExecutor()
    {
        return m_animationExecutor;
    }

#if defined(STARFISH_TIZEN_TV) && defined(STARFISH_ENABLE_AVPLAY)
    WebApis* Webapis()
    {
        return m_webapis;
    }
#endif

    StarFish* starFish()
    {
        return m_starFish;
    }

    ScriptBindingInstance* scriptBindingInstance()
    {
        return m_scriptBindingInstance;
    }

    uint32_t setTimeout(WindowSetTimeoutHandler handler, uint32_t delay,
                        void* data);
    void clearTimeout(uint32_t id);
    uint32_t setInterval(WindowSetTimeoutHandler handler, uint32_t delay,
                         void* data);
    void clearInterval(uint32_t id);

    uint32_t requestAnimationFrame(WindowSetTimeoutHandler handler, void* data);
    void cancelAnimationFrame(uint32_t reqID);

    enum TouchEventKind {
        TouchEventStart,
        TouchEventMove,
        TouchEventEnd,
        TouchEventCancel
    };
    void dispatchTouchEvent(float x, float y, TouchEventKind kind,
                            bool isMobile);

    enum KeyEventKind { KeyEventDown, KeyEventUp };
    void dispatchKeyEvent(String* key, KeyEventKind kind);

    Node* hitTest(float x, float y);

    enum MouseEventKind {
        MouseEventDown,
        MouseEventMove,
        MouseEventUp,
        MouseEventEnter,
        MouseEventOut
    };
    void dispatchMouseEvent(float x, float y, MouseEventKind kind);

    void setActiveNode(Node* n);
    void releaseActiveNode();

    void setFocusedNode(Node* n);
    void releaseFocusedNode();

    void setHoveredNode(Node* n);
    void releaseHoveredNode();

    void processUrlFragment(String* name);
    void setCSSTarget(Node* n);
    void releaseCSSTarget();

    void pause();
    void resume();
    void close();

    bool hasRootElementBackground()
    {
        return m_hasRootElementBackground;
    }

    bool hasBodyElementBackground()
    {
        return m_hasBodyElementBackground;
    }

    virtual int width() = 0;
    virtual int height() = 0;
    virtual void resizeTo(int w, int h) = 0;
    virtual void* unwrap() = 0;

    // The viewport width and height are same as the window size for wearable
    // widget.
    double innerWidth()
    {
        return width();
    }

    double innerHeight()
    {
        return height();
    }

    // These attributes should return the real coordinate when we support
    // scroll.
    double scrollX()
    {
        return 0;
    }

    double scrollY()
    {
        return 0;
    }

    // https://html.spec.whatwg.org/multipage/
    // browsers.html#named-access-on-the-window-object
    HTMLCollection* namedAccess(String* name);

    void layoutIfNeeds();

#ifdef STARFISH_ENABLE_TEST
    void setNetworkState(bool state);
    void screenShot(std::string filePath);
    void forceDisableOnloadCapture();
    void simulateClick(float x, float y);
    void simulateVisibilitychange(bool show);
    void testStart();
#endif

protected:
    void setNeedsRendering()
    {
        if (m_needsRendering) {
            return;
        }
        setNeedsRenderingSlowCase();
    }
    void initFlags();
    void setNeedsRenderingSlowCase();
    Window(StarFish* starFish);

    void rendering();
    void clearStackingContext(bool backupBuffer);
    void paintWindowBackground(Canvas* canvas);

    void markHasPendingStyleSheet();
    void unmarkHasPendingStyleSheet();

    bool m_inRendering;
    bool m_needsRendering;
    bool m_needsStyleRecalc;
    bool m_needsStyleRecalcForWholeDocument;
    bool m_needsFrameTreeBuild;
    bool m_needsLayout;
    bool m_needsPainting;
    bool m_needsComposite;
    bool m_hasRootElementBackground;
    bool m_hasBodyElementBackground;
    bool m_isRunning;

    size_t m_pendingStyleSheetCount;
    size_t m_pendingRenderingCount;
    uint64_t m_lastRenderingTime;

    StarFish* m_starFish;
    ScriptBindingInstance* m_scriptBindingInstance;
    History* m_history;
    Navigator* m_navigator;
    Location* m_location;
    Document* m_document;
    AnimationExecutor* m_animationExecutor;
#if defined(STARFISH_TIZEN_TV) && defined(STARFISH_ENABLE_AVPLAY)
    WebApis* m_webapis;
#endif
    StackingContext* m_rootStackingContext;
    GCVector<CanvasSurface*> m_backStackingContextBufferUpWhileReCompsite;

    Node* m_focusedNode;
    Node* m_relatedTarget;
    Node* m_cssTarget;
    Unit::Location m_touchDownPoint;
    int m_ctrlKeyDown;
    int m_shiftKeyDown;
    int m_altKeyDown;
    int m_metaKeyDown;

    uint32_t m_timeoutCounter;
    GCUnorderedMap<uint32_t, void*> m_timeoutHandler;

    uint32_t m_requestAnimationFrameCounter;
    GCUnorderedMap<uint32_t, void*> m_requestAnimationFrameHandler;

    GCVector<Node*> m_activeNodes;
    GCVector<Node*> m_hoveredNodes;
};
}

#endif
