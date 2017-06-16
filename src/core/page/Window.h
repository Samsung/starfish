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

class BrowsingContext;
class Document;
class History;
class HTMLCollection;
class Location;
class Navigator;
class WebView;
class ScriptBindingInstance;
class StorageNamespace;
class ResourceURL;
class WebApis;
class PlatformWindow;
class Screen;

typedef void (*WindowSetTimeoutHandler)(Window* window, void* data);

class Window : public EventTarget {
    friend class MessageLoop;
    friend class Timer;
    friend class Node;
    friend class StyleRuleImport;

public:
    static Window* create(StarFish* starfish, BrowsingContext* browsingContext,
                          ResourceURL* url);
    virtual ~Window()
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual void postInit(ScriptBindingInstance* instance) override;
    virtual bool isWindow() const;
    void deleteScriptBindingInstance();
    void close();

    // IDL methods
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

    void initStorage(ResourceURL* url);

    BrowsingContext* browsingContext()
    {
        return m_browsingContext;
    }

#if defined(STARFISH_TIZEN_TV) && defined(STARFISH_ENABLE_AVPLAY)
    WebApis* Webapis()
    {
        return m_webapis;
    }
#endif

    StarFish* starFish() const
    {
        return m_starFish;
    }

    ScriptBindingInstance* scriptBindingInstance()
    {
        return m_scriptBindingInstance;
    }

    void processUrlFragment(String* name);
    void setCSSTarget(Node* n);
    void releaseCSSTarget();

    CSSStyleDeclaration* getComputedStyle(Element* element);
    CSSStyleDeclaration* getComputedStyle(Element* element, String* pseudoElt);

    // The viewport width and height are same as the window size for wearable
    // widget.
    int32_t innerWidth();
    int32_t innerHeight();

    int32_t width();
    int32_t height();

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

private:
    void initFlags();
    Window(StarFish* starFish, BrowsingContext* browsingContext,
           ResourceURL* url);
    Window();

    StarFish* m_starFish;
    BrowsingContext* m_browsingContext;
    ScriptBindingInstance* m_scriptBindingInstance;
    History* m_history;
    Navigator* m_navigator;
    Location* m_location;
    Screen* m_screen;

    StorageNamespace* m_localStorageNamespace;
    StorageNamespace* m_sessionStorageNamespace;
#if defined(STARFISH_TIZEN_TV) && defined(STARFISH_ENABLE_AVPLAY)
    WebApis* m_webapis;
#endif

    Node* m_cssTarget;
};
}

#endif
