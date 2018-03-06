/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
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
#ifndef __StarFishWindow__
#define __StarFishWindow__

#include "core/dom/EventTarget.h"
#include "core/dom/Scrolling.h"

namespace StarFish {

class BrowsingContext;
class Document;
class ErrorEventInit;
class History;
class HTMLCollection;
class Location;
class MediaQueryList;
class Navigator;
class NodeList;
class PlatformWindow;
class ResourceURL;
class Screen;
class ScriptBindingInstance;
class StorageNamespace;
class WebView;

struct ScrollOptions {
public:
    enum ScrollBehavior { Auto, Instant, Smooth };

    STARFISH_MAKE_STACK_ALLOCATED()
    ScrollOptions()
        : m_behavior(Auto)
    {
    }

    void setBehavior(ScrollBehavior scrollBehavior)
    {
        m_behavior = scrollBehavior;
    }

    void setBehavior(String* scrollBehaviorStr)
    {
        m_behavior = stringToBehavior(scrollBehaviorStr);
    }

    ScrollBehavior behaviorValue() const
    {
        return m_behavior;
    }

    String* behavior() const
    {
        return ScrollOptions::behaviorToString(m_behavior);
    }

    static ScrollBehavior stringToBehavior(String* scrollBehaviorStr)
    {
        if (scrollBehaviorStr->length() > 3) {
            if (scrollBehaviorStr->equals("auto")) {
                return ScrollOptions::Auto;
            } else if (scrollBehaviorStr->equals("instant")) {
                return ScrollOptions::Instant;
            } else if (scrollBehaviorStr->equals("smooth")) {
                return ScrollOptions::Smooth;
            }
        }
        return ScrollOptions::Auto;
    }

    static String* behaviorToString(ScrollBehavior scrollBehavior)
    {
        switch (scrollBehavior) {
        case ScrollBehavior::Auto:
            return String::fromUTF8("auto");
        case ScrollBehavior::Instant:
            return String::fromUTF8("instant");
        case ScrollBehavior::Smooth:
            return String::fromUTF8("smooth");
        default:
            return String::emptyString;
        }
        return String::emptyString;
    }

private:
    ScrollBehavior m_behavior;
};

struct ScrollToOptions : public ScrollOptions {
public:
    STARFISH_MAKE_STACK_ALLOCATED()
    ScrollToOptions()
        : ScrollOptions()
        , m_left(0)
        , m_top(0)
        , m_hasLeft(false)
        , m_hasTop(false)
    {
    }

    ScrollToOptions(double left, double top)
        : ScrollOptions()
        , m_left(left)
        , m_top(top)
        , m_hasLeft(true)
        , m_hasTop(true)
    {
    }

    void setLeft(double left)
    {
        m_left = left;
        m_hasLeft = true;
    }

    double left() const
    {
        return m_left;
    }

    void setTop(double top)
    {
        m_top = top;
        m_hasTop = true;
    }

    double top() const
    {
        return m_top;
    }

    bool hasLeft() const
    {
        return m_hasLeft;
    }

    bool hasTop() const
    {
        return m_hasTop;
    }

private:
    double m_left;
    double m_top;
    bool m_hasLeft;
    bool m_hasTop;
};

typedef void (*WindowSetTimeoutHandler)(Window* window, void* data);

class Window : public EventTarget {
    friend class MessageLoop;
    friend class Timer;
    friend class Node;
    friend class StyleRuleImport;

public:
    static Window* create(StarFish* starfish, BrowsingContext* browsingContext,
                          ResourceURL* url, uint32_t initialWidth,
                          uint32_t initialHeight);
    virtual ~Window()
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual void postInit(ScriptBindingInstance* instance) override;
    virtual bool isWindow() const;
    virtual void onGlobalPointingEvent(float x, float y,
                                       GlobalPointingEventKind kind) override;
    virtual bool handleDefaultEvent(Event* event);
    void deleteScriptBindingInstance();
    void dispose();

    // IDL methods
    Document* document()
    {
#ifdef STARFISH_TC_COVERAGE
        STARFISH_LOG_INFO("&&&document\n");
#endif
        return DocumentHoldable::document();
    }

    // https://html.spec.whatwg.org/multipage/browsers.html#dom-parent
    Window* parent();
    Window* top();

    Element* frameElement();

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

    Storage* localStorage();
    Storage* sessionStorage();

    uint32_t requestAnimationFrame(WindowSetTimeoutHandler handler, void* data);
    void cancelAnimationFrame(int32_t reqID);

    uint32_t setTimeout(WindowSetTimeoutHandler handler, int32_t delay,
                        void* data);
    void clearTimeout(int32_t id);
    uint32_t setInterval(WindowSetTimeoutHandler handler, int32_t delay,
                         void* data);
    void clearInterval(int32_t id);

    // Other methods
    BrowsingContext* browsingContext()
    {
        return m_browsingContext;
    }

    StarFish* starFish() const
    {
        return m_starFish;
    }

    ScriptBindingInstance* scriptBindingInstance() const
    {
        return m_scriptBindingInstance;
    }

    void processUrlFragment(String* name);
    void setCSSTarget(Node* n);
    void releaseCSSTarget();

    CSSStyleDeclaration* getComputedStyle(Element* element);
    CSSStyleDeclaration* getComputedStyle(Element* element,
                                          Nullable<String*> pseudoElt);

    MediaQueryList* matchMedia(String* query);

    int32_t innerWidth();
    int32_t innerHeight();

    float devicePixelRatio();

    double scrollX();
    double scrollY();
    double pageXOffset();
    double pageYOffset();

    bool scrollTo() // returns scrolling is actually happened
    {
        ScrollToOptions options;
        return scrollTo(options);
    }

    bool scrollTo(ScrollToOptions options);
    bool scrollTo(double x, double y)
    {
        ScrollToOptions options(x, y);
        return scrollTo(options);
    }

    bool scroll() // returns scrolling is actually happened
    {
        return scrollTo();
    }

    bool scroll(ScrollToOptions options)
    {
        return scrollTo(options);
    }

    bool scroll(double x, double y)
    {
        return scrollTo(x, y);
    }

    Scrolling* scrolling()
    {
        return m_scrolling;
    }

    void resize(uint32_t w, uint32_t h);

    void focus();
    void blur();

    void postMessage(ScriptValue message, String* targetOrigin);
    void postMessage(ScriptValue message, String* targetOrigin,
                     GCVector<ScriptValue>& transfer);

    // https://html.spec.whatwg.org/multipage/
    // browsers.html#named-access-on-the-window-object
    ScriptValue namedAccess(String* name);
    Nullable<ScriptObject> defaultNamedGetter(String* name);
    Window* defaultIndexedGetter(uint32_t idx);
    uint32_t length();
    void invalidateFramesIfNeeded();
    Window* frames()
    {
        return this;
    }
    Window* window()
    {
        return this;
    }
    String* name();
    void setName(String* name);

    void dispatchErrorEvent(ErrorEventInit& errorInfo);

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
    // https://html.spec.whatwg.org/multipage/webappapis.html#globaleventhandlers
    DECLARE_EVENT_LISTENER(abort);
    // DECLARE_EVENT_LISTENER(auxclick);
    DECLARE_EVENT_LISTENER(blur);
    // DECLARE_EVENT_LISTENER(cancel);
    DECLARE_EVENT_LISTENER(change);
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
    DECLARE_EVENT_LISTENER(error);
    DECLARE_EVENT_LISTENER(focus);
    DECLARE_EVENT_LISTENER(input);
    DECLARE_EVENT_LISTENER(invalid);
    DECLARE_EVENT_LISTENER(keydown);
    DECLARE_EVENT_LISTENER(keypress);
    DECLARE_EVENT_LISTENER(keyup);
    DECLARE_EVENT_LISTENER(load);
    // DECLARE_EVENT_LISTENER(loadend);
    DECLARE_EVENT_LISTENER(loadstart);
    DECLARE_EVENT_LISTENER(mousedown);
    // DECLARE_EVENT_LISTENER(mouseenter);
    // DECLARE_EVENT_LISTENER(mouseleave);
    DECLARE_EVENT_LISTENER(mousemove);
    DECLARE_EVENT_LISTENER(mouseout);
    DECLARE_EVENT_LISTENER(mouseover);
    DECLARE_EVENT_LISTENER(mouseup);
    // DECLARE_EVENT_LISTENER(wheel);
    DECLARE_EVENT_LISTENER(progress);
    // DECLARE_EVENT_LISTENER(reset);
    DECLARE_EVENT_LISTENER(resize);
    // DECLARE_EVENT_LISTENER(scroll);
    // DECLARE_EVENT_LISTENER(select);
    // DECLARE_EVENT_LISTENER(show);
    // DECLARE_EVENT_LISTENER(toggle);
    DECLARE_EVENT_LISTENER(submit);
#ifdef STARFISH_ENABLE_MULTIMEDIA
    DECLARE_EVENT_LISTENER(suspend);
    DECLARE_EVENT_LISTENER(emptied);
    DECLARE_EVENT_LISTENER(stalled);
    DECLARE_EVENT_LISTENER(loadedmetadata);
    DECLARE_EVENT_LISTENER(loadeddata);
    DECLARE_EVENT_LISTENER(canplay);
    DECLARE_EVENT_LISTENER(canplaythrough);
    DECLARE_EVENT_LISTENER(playing);
    DECLARE_EVENT_LISTENER(waiting);
    DECLARE_EVENT_LISTENER(seeking);
    DECLARE_EVENT_LISTENER(seeked);
    DECLARE_EVENT_LISTENER(ended);
    DECLARE_EVENT_LISTENER(durationchange);
    DECLARE_EVENT_LISTENER(timeupdate);
    DECLARE_EVENT_LISTENER(play);
    DECLARE_EVENT_LISTENER(pause);
    DECLARE_EVENT_LISTENER(ratechange);
    DECLARE_EVENT_LISTENER(volumechange);
#endif

    // https://html.spec.whatwg.org/multipage/webappapis.html#windoweventhandlers
    // DECLARE_EVENT_LISTENER(afterprint);
    // DECLARE_EVENT_LISTENER(beforeprint);
    // DECLARE_EVENT_LISTENER(beforeunload);
    // DECLARE_EVENT_LISTENER(hashchange);
    // DECLARE_EVENT_LISTENER(languagechange);
    DECLARE_EVENT_LISTENER(message);
    DECLARE_EVENT_LISTENER(messageerror);
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

private:
    void initFlags();
    Window(StarFish* starFish, BrowsingContext* browsingContext,
           ResourceURL* url, uint32_t initialWidth, uint32_t initialHeight);
    Window();
    NodeList* ensureFrames();

    StarFish* m_starFish;
    BrowsingContext* m_browsingContext;
    ScriptBindingInstance* m_scriptBindingInstance;
    History* m_history;
    Navigator* m_navigator;
    Location* m_location;
    Screen* m_screen;
    Scrolling* m_scrolling;

    uint32_t m_width;
    uint32_t m_height;

    Node* m_cssTarget;
    NodeList* m_frames;
};
}

#endif
