#ifndef __StarFishWindow__
#define __StarFishWindow__

#include "style/Style.h"
#include "dom/EventTarget.h"

namespace StarFish {

class StarFish;
class Document;
class Window;
class URL;

typedef void (*WindowSetTimeoutHandler)(Window* window, void* data);

class Window : public EventTarget {
    friend class HTMLHtmlElement;
    friend class HTMLBodyElement;
    friend class Node;
public:
#ifndef STARFISH_TIZEN_WEARABLE
    static Window* create(StarFish* sf, size_t w = SIZE_MAX, size_t h = SIZE_MAX);
#else
    static Window* create(StarFish* sf, size_t w = SIZE_MAX, size_t h = SIZE_MAX, void* win = nullptr);
#endif

    virtual bool isWindow()
    {
        return true;
    }

    void setNeedsStyleRecalc()
    {
        if (!m_needsStyleRecalc) {
            m_needsStyleRecalc = true;
            setNeedsRendering();
        }
    }

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

    void loadXMLDocument(String* filePath);
    uint32_t setTimeout(WindowSetTimeoutHandler handler, uint32_t delay, void* data);
    void clearTimeout(uint32_t id);

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

    StyleResolver* styleResolver()
    {
        return &m_styleResolver;
    }

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

protected:
    void setNeedsRendering()
    {
        if (m_needsRendering) {
            return;
        }
        setNeedsRenderingSlowCase();
    }

    void setNeedsRenderingSlowCase();
    Window(StarFish* starFish);
    void rendering();

    StyleResolver m_styleResolver;

    bool m_needsRendering;
    bool m_needsStyleRecalc;
    bool m_needsFrameTreeBuild;
    bool m_needsLayout;
    bool m_needsPainting;
    // bool m_needsComposite; // TODO
    bool m_hasRootElementBackground;
    bool m_hasBodyElementBackground;
    bool m_isRunning;

    Document* m_document;
    Node* m_activeNodeWithTouchDown;
    Location m_touchDownPoint;

    StarFish* m_starFish;

    uint32_t m_timeoutCounter;
    std::unordered_map<uint32_t, std::pair<WindowSetTimeoutHandler, void*>, std::hash<uint32_t>, std::equal_to<uint32_t>,
        gc_allocator<std::pair<uint32_t, std::pair<WindowSetTimeoutHandler, void*> > > > m_timeoutHandler;

    uint32_t m_requestAnimationFrameCounter;
    std::unordered_map<uint32_t, std::pair<WindowSetTimeoutHandler, void*>, std::hash<uint32_t>, std::equal_to<uint32_t>,
        gc_allocator<std::pair<uint32_t, std::pair<WindowSetTimeoutHandler, void*> > > > m_requestAnimationFrameHandler;

    std::vector<Node*, gc_allocator<Node*> > m_activeNodes;
};

}

#endif
