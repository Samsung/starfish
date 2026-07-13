/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
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
#if !defined(__StarfishWindow__)
#define __StarfishWindow__

#include "binding/WindowProxy.h"
#include "core/dom/EventTarget.h"
#include "core/fetch/Fetch.h"
#include "core/modules/tts/SpeechSynthesis.h"
#include "core/page/GlobalScope.h"
#include "core/page/WindowOrWorkerGlobalScope.h"
#include "core/page/ScrollOptions.h"
#include "core/extra/Performance.h"
#include "core/modules/crypto/Crypto.h"

namespace Starfish {

class BrowsingContext;
class Document;
class ErrorEventInit;
class History;
class HTMLCollection;
class Location;
class MediaQueryList;
class Navigator;
class NodeList;
class Renderer;
class ResourceURL;
class Screen;
class ScriptBindingInstance;
class StorageNamespace;
class WebView;
class Scrolling;
class LayoutUnit;
class CustomElementRegistry;
class IDBFactory;

typedef void (*TimerHandler)(void* data);
using Disposer = std::function<void()>;

class Window : public EventTarget, public GlobalScope {
    friend class MessageLoop;
    friend class Timer;
    friend class Node;
    friend class StyleRuleImport;

public:
    static Window* create(BrowsingContext* browsingContext, ResourceURL* url,
                          uint32_t initialWidth, uint32_t initialHeight);
    virtual ~Window()
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual void postInit(ScriptBindingInstance* instance) override;
    virtual bool isWindow() const override;
    virtual void onGlobalPointingEvent(float x, float y, DOMTimeStamp timeStamp,
                                       GlobalPointingEventKind kind) override;
    virtual bool handleDefaultEvent(Event* event) override;

    bool isGlobalScope() const override
    {
        return true;
    }

    ExecutionContext* executionContext() const override;

    void dispose();

    // IDL methods
    Document* document() const
    {
#ifdef STARFISH_TC_COVERAGE
        STARFISH_LOG_INFO("&&&document");
#endif
        return m_document;
    }

    Starfish* starfish() const;
    StaticStrings* staticStrings() const;
    WebView* webView() const;

    // https://html.spec.whatwg.org/multipage/browsers.html#dom-parent
    WindowProxy* parent();
    WindowProxy* top();

    Element* frameElement();

    History* history()
    {
        return m_history;
    }

    CustomElementRegistry* customElements();
    bool hasCustomElements()
    {
        return m_customElementRegistry.hasValue();
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

    uint32_t requestAnimationFrame(TimerHandler handler, void* data);
    void cancelAnimationFrame(int32_t reqID);

    uint32_t setTimeout(TimerHandler handler, int32_t delay, void* data);
    void clearTimeout(int32_t id);
    uint32_t setInterval(TimerHandler handler, int32_t delay, void* data);
    void clearInterval(int32_t id);

    String* btoa(ExecutionContext* executionContext, String* data);
    String* atob(ExecutionContext* executionContext, String* data);

    void queueMicrotask(ExecutionContext* executionContext,
                        ScriptObject callback);
    ScriptValue structuredClone(ExecutionContext* executionContext,
                                ScriptValue value);
    ScriptValue structuredClone(ExecutionContext* executionContext,
                                ScriptValue value,
                                StructuredSerializeOptions options);

#ifdef STARFISH_ENABLE_CANVAS
    Promise* createImageBitmap(
        ExecutionContext* executionContext, ImageBitmapSource image,
        ImageBitmapOptions options = ImageBitmapOptions());
    Promise* createImageBitmap(
        ExecutionContext* executionContext, ImageBitmapSource image, int32_t sx,
        int32_t sy, int32_t sw, int32_t sh,
        ImageBitmapOptions options = ImageBitmapOptions());
#endif
    void alert();
    void alert(String* message);
    bool confirm(String* message);
    Optional<String*> prompt(String* message, String* defaultValue);

#if defined(STARFISH_ENABLE_TTS)
    SpeechSynthesis* speechSynthesis()
    {
        return m_speechSynthesis;
    }
#endif

    // Other methods
    BrowsingContext* browsingContext() const
    {
        return m_browsingContext;
    }

    ScriptBindingInstance* scriptBindingInstance() override
    {
        return m_scriptBindingInstance;
    }

    void processUrlFragment(String* name);
    void setCSSTarget(Node* n);
    void releaseCSSTarget();

    CSSStyleDeclaration* getComputedStyle(Element* element);
    CSSStyleDeclaration* getComputedStyle(Element* element,
                                          Optional<String*> pseudoElt);

    MediaQueryList* matchMedia(String* query);

    int32_t innerWidth();
    int32_t innerHeight();
    bool isInnerSizeEmpty();

    float devicePixelRatio();

    double scrollX(bool canLeadLayoutThrashing = true);
    double scrollY(bool canLeadLayoutThrashing = true);
    double pageXOffset();
    double pageYOffset();
    LayoutUnit scrollWidth(bool canLeadLayoutThrashing = true);
    LayoutUnit scrollHeight(bool canLeadLayoutThrashing = true);

    bool scrollTo() // returns scrolling is actually happened
    {
        ScrollToOptions options;
        return scrollTo(options);
    }

    bool scrollToWithoutLayout(double x, double y);
    bool scrollTo(ScrollToOptions options);
    bool scrollTo(double x, double y)
    {
        ScrollToOptions options(x, y);
        return scrollTo(options);
    }

    bool scrollBy() // returns scrolling is actually happened
    {
        return scrollTo();
    }
    bool scrollBy(ScrollToOptions options);
    bool scrollBy(double x, double y);

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

    void postMessage(Window* source, ScriptValue message, String* targetOrigin);
    void postMessage(Window* source, ScriptValue message, String* targetOrigin,
                     GCVector<ScriptObject>& transfer);

    // https://html.spec.whatwg.org/multipage/
    // browsers.html#named-access-on-the-window-object
    ScriptValue namedAccess(String* name);
    Optional<ScriptObject> defaultNamedGetter(String* name);
    Window* defaultIndexedGetter(uint32_t idx);
    uint32_t length();
    void invalidateFramesIfNeeded();
    WindowProxy* frames()
    {
        return window();
    }
    WindowProxy* window()
    {
        return m_proxy;
    }
    String* name();
    void setName(String* name);

    void dispatchErrorEvent(ErrorEventInit& errorInfo);

    bool checkSecurityPolicy();

    Promise* fetch(RequestInfo& input);
    Promise* fetch(RequestInfo& input, RequestInit& init);

    Performance* performance();
    Crypto* crypto();

    void registerDisposer(void* holder, Disposer function);

#ifdef STARFISH_ENABLE_OBSOLETE_SPEC
    Event* event();
    void setEvent(Event* e);
#endif

#if defined(STARFISH_ENABLE_IDB)
    IDBFactory* indexedDB();
#endif

#ifdef STARFISH_ENABLE_TEST
    void setNetworkState(bool state);
    void screenShot(std::string filePath, void (*callback)(void*), void* data);
    void forceDisableOnloadCapture();
    void simulateClick(float x, float y);
    void simulateMouseDown(float x, float y);
    void simulateMouseUp(float x, float y);
    void simulateMouseMove(float x, float y);
    void simulateTouchStart(float x, float y);
    void simulateTouchMove(float x, float y);
    void simulateTouchEnd(float x, float y);
    void simulateTouchCancel(float x, float y);
    void simulateVisibilitychange(bool show);
    void testStart();
    // Touch-exploration accessibility test helpers.
    String* getLastTTSText();
    void setTTSAccessibilityMode(bool value);
    String* getA11yFocusedElementId();
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
    DECLARE_EVENT_LISTENER(mouseenter);
    DECLARE_EVENT_LISTENER(mouseleave);
    DECLARE_EVENT_LISTENER(mousemove);
    DECLARE_EVENT_LISTENER(mouseout);
    DECLARE_EVENT_LISTENER(mouseover);
    DECLARE_EVENT_LISTENER(mouseup);
    // DECLARE_EVENT_LISTENER(wheel);
    DECLARE_EVENT_LISTENER(progress);
    // DECLARE_EVENT_LISTENER(reset);
    DECLARE_EVENT_LISTENER(resize);
    // DECLARE_EVENT_LISTENER(select);
    // DECLARE_EVENT_LISTENER(show);
    // DECLARE_EVENT_LISTENER(toggle);
    DECLARE_EVENT_LISTENER(submit);
    DECLARE_EVENT_LISTENER(securitypolicyviolation);
    DECLARE_EVENT_LISTENER(pointerdown);
    DECLARE_EVENT_LISTENER(pointerup);
    DECLARE_EVENT_LISTENER(pointermove);
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
#if defined(STARFISH_WEBWORKER_NOT_HOST)
    DECLARE_EVENT_LISTENER(hashchange);
#endif
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
    DECLARE_EVENT_LISTENER(scroll);
    DECLARE_EVENT_LISTENER(ttsstart);
    DECLARE_EVENT_LISTENER(ttsend);
#undef VIRTUAL
#undef OVERRIDE

private:
    Window(BrowsingContext* browsingContext, ResourceURL* url,
           uint32_t initialWidth, uint32_t initialHeight);
    Window();
    NodeList* ensureFrames();
#if defined(STARFISH_ENABLE_CDP)
    // Emit Page.javascriptDialogOpening for alert/confirm/prompt via the CDP
    // dispatcher of this window's WebView (no-op without an attached client).
    void emitCDPDialog(const char* type, String* message,
                       String* defaultPrompt);
#endif

    BrowsingContext* m_browsingContext;
    ScriptBindingInstance* m_scriptBindingInstance;
    WindowProxy* m_proxy;
    Document* m_document;
    History* m_history;
    Navigator* m_navigator;
    Location* m_location;
    Optional<Screen*> m_screen;
    Scrolling* m_scrolling;
    Optional<Performance*> m_performance;
    Optional<CustomElementRegistry*> m_customElementRegistry;
    Optional<Crypto*> m_crypto;

#if defined(STARFISH_ENABLE_TTS)
    SpeechSynthesis* m_speechSynthesis;
#endif

#if defined(STARFISH_ENABLE_IDB)
    IDBFactory* m_idbFactory{ nullptr };
#endif

    uint32_t m_width;
    uint32_t m_height;

    Optional<Node*> m_cssTarget;
    Optional<NodeList*> m_frames;
#ifdef STARFISH_ENABLE_OBSOLETE_SPEC
    Event* m_currentDispatchingEvent;
#endif
    GCUnorderedMap<void**, Disposer> m_disposers;
};
} // namespace Starfish

#endif
