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

typedef void (*WindowSetTimeoutHandler)(Window* window, void* data);

class Window : public EventTarget {
    friend class HTMLHtmlElement;
    friend class HTMLBodyElement;
    friend class Node;
public:
#ifndef STARFISH_TIZEN_WEARABLE_APP
    static Window* create(StarFish* sf, size_t w = SIZE_MAX, size_t h = SIZE_MAX);
#else
    static Window* create(StarFish* sf, size_t w = SIZE_MAX, size_t h = SIZE_MAX, void* win = nullptr);
#endif

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

    void loadXMLDocument(String* filePath);
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

    void dispatchLoadEvent();
    void dispatchUnloadEvent();

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

    // The viewport width and height are same as the window size for wearable widget.
    double innerWidth()
    {
        return width();
    }
    double innerHeight()
    {
        return height();
    }

    ScriptValue onclick()
    {
        auto eventType = starFish()->staticStrings()->m_click.string();
        EventListener* l = getAttributeEventListener(eventType);
        if (!l)
            return ScriptValueNull;
        return l->scriptFunction(this, eventType);
    }

    void setOnclick(ScriptValue f)
    {
        auto eventType = starFish()->staticStrings()->m_click.string();
        EventListener* l = new EventListener(f, true);
        setAttributeEventListener(eventType, l);
    }

    void clearOnClick()
    {
        auto eventType = starFish()->staticStrings()->m_click.string();
        clearAttributeEventListener(eventType);
    }

    ScriptValue onload()
    {
        auto eventType = starFish()->staticStrings()->m_load.string();
        EventListener* l = getAttributeEventListener(eventType);
        if (!l)
            return ScriptValueNull;
        return l->scriptFunction(this, eventType);
    }

    void setOnload(ScriptValue f)
    {
        auto eventType = starFish()->staticStrings()->m_load.string();
        EventListener* l = new EventListener(f, true);
        setAttributeEventListener(eventType, l);
    }

    void clearOnload()
    {
        auto eventType = starFish()->staticStrings()->m_load.string();
        clearAttributeEventListener(eventType);
    }

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
