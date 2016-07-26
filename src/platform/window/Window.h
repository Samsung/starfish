/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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

#include "style/Style.h"
#include "dom/EventTarget.h"

namespace StarFish {

class StarFish;
class Document;
class Window;
class URL;
class Canvas;
class HTMLCollection;

typedef void (*WindowSetTimeoutHandler)(Window* window, void* data);

class Window : public EventTarget {
    friend class HTMLHtmlElement;
    friend class HTMLBodyElement;
    friend class Node;
public:
    static Window* create(StarFish* sf, void* win, int width, int height, const URL& url);
    virtual bool isWindow()
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

    StarFish* starFish()
    {
        return m_starFish;
    }

    uint32_t setTimeout(WindowSetTimeoutHandler handler, uint32_t delay, void* data);
    void clearTimeout(uint32_t id);
    uint32_t setInterval(WindowSetTimeoutHandler handler, uint32_t delay, void* data);
    void clearInterval(uint32_t id);

    uint32_t requestAnimationFrame(WindowSetTimeoutHandler handler, void* data);
    void cancelAnimationFrame(uint32_t reqID);

    enum TouchEventKind {
        TouchEventDown,
        TouchEventMove,
        TouchEventUp
    };
    void dispatchTouchEvent(float x, float y, TouchEventKind kind);

    enum KeyEventKind {
        KeyEventDown,
        KeyEventUp
    };
    void dispatchKeyEvent(String* key, KeyEventKind kind);

    Node* hitTest(float x, float y);
    Node* activeNodeWithTouchDown()
    {
        return m_activeNodeWithTouchDown;
    }

    void setActiveNode(Node* n);
    void releaseActiveNode();

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

    // The viewport width and height are same as the window size for wearable widget.
    double innerWidth()
    {
        return width();
    }
    double innerHeight()
    {
        return height();
    }

    // https://html.spec.whatwg.org/multipage/browsers.html#named-access-on-the-window-object
    HTMLCollection* namedAccess(String* name);

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

    void setNeedsRenderingSlowCase();
    Window(StarFish* starFish, const URL& url);
    void rendering();
    void paintWindowBackground(Canvas* canvas);

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

    StarFish* m_starFish;
    Document* m_document;
    Node* m_activeNodeWithTouchDown;
    Location m_touchDownPoint;

    uint32_t m_timeoutCounter;
    std::unordered_map<uint32_t, void*, std::hash<uint32_t>, std::equal_to<uint32_t>,
        gc_allocator<std::pair<uint32_t, void* > > > m_timeoutHandler;

    uint32_t m_requestAnimationFrameCounter;
    std::unordered_map<uint32_t, void*, std::hash<uint32_t>, std::equal_to<uint32_t>,
        gc_allocator<std::pair<uint32_t, void*> > > m_requestAnimationFrameHandler;

    std::vector<Node*, gc_allocator<Node*> > m_activeNodes;
};

}

#endif
