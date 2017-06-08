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

#include "core/dom/EventTarget.h"

namespace StarFish {

class AnimationExecutor;
class Canvas;
class CanvasSurface;
class Document;
class History;
class HTMLCollection;
class Location;
class Navigator;
class ScriptBindingInstance;
class StarFish;
class StackingContext;
class StorageNamespace;
class URL;
class WebApis;
class Window;
class Screen;

typedef void (*WindowSetTimeoutHandler)(Window* window, void* data);

class Window : public EventTarget {
    friend class MessageLoop;
    friend class Timer;
    friend class HTMLHtmlElement;
    friend class HTMLBodyElement;
    friend class HTMLLinkElement;
    friend class Node;
    friend class CSSImportRule;

public:
    static Window* create(StarFish* sf, void* win, int width, int height);
    ~Window();
    void navigate(URL* url);
    void navigateAsync(URL* url);
    void navigateAsyncWithoutSetHistory(URL* url);
    void setHistory(URL* url);
    void initStorage(URL* url);

    virtual void init(ScriptBindingInstance* instance) override;
    virtual void postInit(ScriptBindingInstance* instance) override;
    virtual bool isWindow() const;

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
#ifdef STARFISH_TC_COVERAGE
        STARFISH_LOG_INFO("&&&document\n");
#endif
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

    Screen* screen();

    AnimationExecutor* animationExecutor()
    {
        return m_animationExecutor;
    }

    Storage* localStorage();
    Storage* sessionStorage();

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

    uint32_t setTimeout(WindowSetTimeoutHandler handler, int32_t delay,
                        void* data);
    void clearTimeout(int32_t id);
    uint32_t setInterval(WindowSetTimeoutHandler handler, int32_t delay,
                         void* data);
    void clearInterval(int32_t id);

    uint32_t requestAnimationFrame(WindowSetTimeoutHandler handler, void* data);
    void cancelAnimationFrame(int32_t reqID);

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

    CSSStyleDeclaration* getComputedStyle(Element* element);
    CSSStyleDeclaration* getComputedStyle(Element* element, String* pseudoElt);

    virtual int32_t width() = 0;
    virtual int32_t height() = 0;
    virtual void resizeTo(int w, int h) = 0;
    virtual void* unwrap() = 0;
    virtual void clearResources() = 0;
    virtual Canvas* preparePainting(bool forPainting) = 0;

    // The viewport width and height are same as the window size for wearable
    // widget.
    int32_t innerWidth()
    {
        return width();
    }

    int32_t innerHeight()
    {
        return height();
    }

    float devicePixelRatio();

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

#define VIRTUAL
#define OVERRIDE
    // https://html.spec.whatwg.org/multipage/webappapis.html#globaleventhandlers
    DECLARE_EVENT_LISTENER(abort);
    // DECLARE_EVENT_LISTENER(auxclick);
    // DECLARE_EVENT_LISTENER(blur);
    // DECLARE_EVENT_LISTENER(cancel);
    DECLARE_EVENT_LISTENER(canplay);
    DECLARE_EVENT_LISTENER(canplaythrough);
    // DECLARE_EVENT_LISTENER(change);
    DECLARE_EVENT_LISTENER(click);
    // DECLARE_EVENT_LISTENER(close);
    // DECLARE_EVENT_LISTENER(contextmenu);
    // DECLARE_EVENT_LISTENER(cuechange);
    // DECLARE_EVENT_LISTENER(dblclick);
    // DECLARE_EVENT_LISTENER(drag);
    // DECLARE_EVENT_LISTENER(dragend);
    // DECLARE_EVENT_LISTENER(dragenter);
    // DECLARE_EVENT_LISTENER(dragexit);
    // DECLARE_EVENT_LISTENER(dragleave);
    // DECLARE_EVENT_LISTENER(dragover);
    // DECLARE_EVENT_LISTENER(dragstart);
    // DECLARE_EVENT_LISTENER(drop);
    DECLARE_EVENT_LISTENER(durationchange);
    DECLARE_EVENT_LISTENER(emptied);
    DECLARE_EVENT_LISTENER(ended);
    DECLARE_EVENT_LISTENER(error);
    DECLARE_EVENT_LISTENER(focus);
    // DECLARE_EVENT_LISTENER(input);
    // DECLARE_EVENT_LISTENER(invalid);
    DECLARE_EVENT_LISTENER(keydown);
    // DECLARE_EVENT_LISTENER(keypress);
    DECLARE_EVENT_LISTENER(keyup);
    DECLARE_EVENT_LISTENER(load);
    DECLARE_EVENT_LISTENER(loadeddata);
    DECLARE_EVENT_LISTENER(loadedmetadata);
    // DECLARE_EVENT_LISTENER(loadend);
    DECLARE_EVENT_LISTENER(loadstart);
    // DECLARE_EVENT_LISTENER(mousedown);
    // DECLARE_EVENT_LISTENER(mouseenter);
    // DECLARE_EVENT_LISTENER(mouseleave);
    // DECLARE_EVENT_LISTENER(mousemove);
    // DECLARE_EVENT_LISTENER(mouseout);
    DECLARE_EVENT_LISTENER(mouseover);
    // DECLARE_EVENT_LISTENER(mouseup);
    // DECLARE_EVENT_LISTENER(wheel);
    DECLARE_EVENT_LISTENER(pause);
    DECLARE_EVENT_LISTENER(play);
    DECLARE_EVENT_LISTENER(playing);
    DECLARE_EVENT_LISTENER(progress);
    DECLARE_EVENT_LISTENER(ratechange);
    // DECLARE_EVENT_LISTENER(reset);
    // DECLARE_EVENT_LISTENER(resize);
    // DECLARE_EVENT_LISTENER(scroll);
    DECLARE_EVENT_LISTENER(seeked);
    DECLARE_EVENT_LISTENER(seeking);
    // DECLARE_EVENT_LISTENER(select);
    // DECLARE_EVENT_LISTENER(show);
    DECLARE_EVENT_LISTENER(stalled);
    // DECLARE_EVENT_LISTENER(submit);
    DECLARE_EVENT_LISTENER(suspend);
    DECLARE_EVENT_LISTENER(timeupdate);
    // DECLARE_EVENT_LISTENER(toggle);
    DECLARE_EVENT_LISTENER(volumechange);
    DECLARE_EVENT_LISTENER(waiting);

    // https://html.spec.whatwg.org/multipage/webappapis.html#windoweventhandlers
    // DECLARE_EVENT_LISTENER(afterprint);
    // DECLARE_EVENT_LISTENER(beforeprint);
    // DECLARE_EVENT_LISTENER(beforeunload);
    // DECLARE_EVENT_LISTENER(hashchange);
    // DECLARE_EVENT_LISTENER(languagechange);
    // DECLARE_EVENT_LISTENER(message);
    // DECLARE_EVENT_LISTENER(offline);
    // DECLARE_EVENT_LISTENER(online);
    // DECLARE_EVENT_LISTENER(pagehide);
    // DECLARE_EVENT_LISTENER(pageshow);
    // DECLARE_EVENT_LISTENER(popstate);
    // DECLARE_EVENT_LISTENER(rejectionhandled);
    // DECLARE_EVENT_LISTENER(storage);
    // DECLARE_EVENT_LISTENER(unhandledrejection);
    DECLARE_EVENT_LISTENER(unload);
#undef VIRTUAL
#undef OVERRIDE

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
    bool m_isActive;

    size_t m_pendingStyleSheetCount;
    size_t m_pendingRenderingCount;
    uint64_t m_lastRenderingTime;

    StarFish* m_starFish;
    ScriptBindingInstance* m_scriptBindingInstance;
    History* m_history;
    Navigator* m_navigator;
    Location* m_location;
    Screen* m_screen;
    AnimationExecutor* m_animationExecutor;
    StorageNamespace* m_localStorageNamespace;
    StorageNamespace* m_sessionStorageNamespace;
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

    GCVector<Node*> m_activeNodes;
    GCVector<Node*> m_hoveredNodes;
};
}

#endif
