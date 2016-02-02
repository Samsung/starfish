#ifndef __StarFishWindow__
#define __StarFishWindow__

#include "style/Style.h"
#include "dom/EventTarget.h"

namespace StarFish {

class StarFish;
class Document;
class Window;

typedef void (*WindowSetTimeoutHandler)(Window* window, void* data);

class Window : public EventTarget<ScriptWrappableGlobalObject> {
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

    void setNeedsRendering();
    void renderingIfNeeds();

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

    void pause();
    void resume();

    StyleResolver* styleResolver()
    {
        return &m_styleResolver;
    }

    virtual int width() = 0;
    virtual int height() = 0;

protected:
    Window(StarFish* starFish);
    void rendering();

    StyleResolver m_styleResolver;

    bool m_needsRendering;
    bool m_isRunning;
    Document* m_document;
    Node* m_activeNodeWithTouchDown;

    StarFish* m_starFish;

    uint32_t m_timeoutCounter;
    std::unordered_map<uint32_t, std::pair<WindowSetTimeoutHandler, void*>, std::hash<uint32_t>, std::equal_to<uint32_t>,
        gc_allocator<std::pair<uint32_t, std::pair<WindowSetTimeoutHandler, void*> > > > m_timeoutHandler;
};

}

#endif
