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

#include "StarfishConfig.h"
#include "Starfish.h"

#include "core/page/Window.h"
#include "binding/ScriptBindingInstance.h"
#include "binding/ScriptBindingWindowInstance.h"
#include "core/csp/ContentSecurityPolicy.h"
#include "core/dom/CustomElementRegistry.h"
#include "core/dom/DOMException.h"
#include "core/dom/ErrorEvent.h"
#include "core/dom/HTMLAnchorElement.h"
#include "core/dom/HTMLDocument.h"
#include "core/dom/HTMLIFrameElement.h"
#include "core/dom/HTMLCollection.h"
#include "core/dom/MessageEvent.h"
#include "core/dom/MouseEvent.h"
#include "core/dom/NodeList.h"
#include "core/dom/Traverse.h"
#include "core/dom/TouchEvent.h"
#include "core/dom/WebOrigin.h"
#include "core/dom/Scrolling.h"
#include "core/extra/Console.h"
#include "core/extra/Performance.h"
#include "core/layout/FrameDocument.h"
#include "core/layout/StackingContext.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/message_loop/Timer.h"
#include "core/page/BrowsingContext.h"
#include "core/page/History.h"
#include "core/page/Navigator.h"
#include "core/page/Location.h"
#include "core/page/Screen.h"
#include "core/page/WebView.h"
#include "core/page/GlobalScope.h"
#include "core/page/Serializer.h"
#include "core/storage/Storage.h"
#include "core/storage/StorageNamespace.h"
#include "core/style/CSSParser.h"
#include "core/style/MediaQueryList.h"
#include "core/style/MediaQueryListMatcher.h"
#include "core/modules/renderer/Renderer.h"
#include "core/modules/crypto/Crypto.h"
#include "core/modules/indexeddb/IDBStorageManager.h"
#include "core/modules/indexeddb/IDBFactory.h"
#include "binding/ScriptBindingSecurity.h"

#ifdef STARFISH_ENABLE_SERVICE_WORKER
#include "platform/process/base/ProcessType.h"
#include "core/modules/serviceworker/client/ServiceWorkerProcessManager.h"
#endif

#ifdef STARFISH_ENABLE_TEST
#include <sys/ioctl.h>
#include <net/if.h>
#endif

namespace Starfish {

Window* Window::create(BrowsingContext* browsingContext, ResourceURL* url,
                       uint32_t initialWidth, uint32_t initialHeight)
{
    return new Window(browsingContext, url, initialWidth, initialHeight);
}

Window::Window(BrowsingContext* browsingContext, ResourceURL* url,
               uint32_t initialWidth, uint32_t initialHeight)
    : EventTarget()
    , GlobalScope(browsingContext->webView())
    , m_browsingContext(browsingContext)
    , m_proxy(nullptr)
    , m_document(nullptr)
    , m_history(nullptr)
    , m_navigator(nullptr)
    , m_location(nullptr)
    , m_scrolling(new Scrolling(this))
    , m_width(initialWidth)
    , m_height(initialHeight)
#ifdef STARFISH_ENABLE_OBSOLETE_SPEC
    , m_currentDispatchingEvent(nullptr)
#endif
{
    /*
        GC_REGISTER_FINALIZER_NO_ORDER(
            this,
            [](void* obj, void* cd) { STARFISH_LOG_INFO("Window::~Window"); },
            NULL, NULL, NULL);
    */
    m_scriptBindingInstance = new ScriptBindingWindowInstance(
        browsingContext->webView()->scriptEngineInstance(), this);

    if (!browsingContext->isTopLevelBrowsingContext()) {
        // if there is already created window proxy..
        if (browsingContext->sourceElement()->contentWindow()) {
            m_proxy = browsingContext->sourceElement()->contentWindow();
            m_proxy->updateSource(this);
        } else {
            m_proxy = new WindowProxy(this);
        }
    } else {
        m_proxy = new WindowProxy(this);
    }

    // TODO: use location to open a new document
    m_document = new HTMLDocument(this, m_scriptBindingInstance, url,
                                  String::createASCIIString("UTF-8"), true);
    m_history = new History(m_document);
    m_navigator = new Navigator(m_document);
    m_location = new Location(m_document);
    m_scriptBindingInstance->initBinding();

#if defined(STARFISH_ENABLE_TTS)
    m_speechSynthesis = new SpeechSynthesis(m_document);
#endif

#ifdef STARFISH_ENABLE_SERVICE_WORKER
    ServiceWorkerProcessManager::instance()->registerActiveGlobalScope(uid(),
                                                                       this);
#endif

#if defined(STARFISH_ENABLE_IDB)
    IDBStorageManager::instance();
#endif
}

Starfish* Window::starfish() const
{
    return browsingContext()->webView()->starfish();
}

StaticStrings* Window::staticStrings() const
{
    return starfish()->staticStrings();
}

WebView* Window::webView() const
{
    return browsingContext()->webView();
}

void Window::registerDisposer(void* object, Disposer function)
{
    STARFISH_ASSERT(GC_base(object) != 0);
    STARFISH_ASSERT(function != nullptr);

    // link is a weak pointer to the given object. *link will be nullptr when
    // the object is inaccessible.
    void** link = reinterpret_cast<void**>(GC_MALLOC_ATOMIC(sizeof(void*)));
    *link = reinterpret_cast<void*>(GC_HIDE_POINTER(object));
    int result = GC_GENERAL_REGISTER_DISAPPEARING_LINK(link, object);

    STARFISH_ASSERT(result == 0);

    m_disposers[link] = function;
}

void Window::dispose()
{
    GCVector<Element*> iframeCollection;
    Traverse::collectDescendants(
        iframeCollection, document(),
        [&](Element* element) { return element->isHTMLIFrameElement(); },
        false);

    for (size_t i = 0; i < iframeCollection.size(); i++) {
        iframeCollection[i]->asHTMLIFrameElement()->unloadSrc();
    }

    for (const auto& pair : m_disposers) {
        void** link = pair.first;
        Disposer disposer = pair.second;
        if (*link) {
            disposer();
        }
    }
    m_disposers.clear();

    clearEventListeners();

    ResourceURL* url = m_document->documentURI();
    m_location->dispose();
    m_navigator->dispose();
    m_document->dispose();
#if defined(STARFISH_ENABLE_TTS)
    m_speechSynthesis->dispose();
#endif

    if (m_scriptBindingInstance) {
        m_scriptBindingInstance->destroy();
    }
#ifdef STARFISH_ENABLE_SERVICE_WORKER
    ServiceWorkerProcessManager::instance()->deregisterActiveGlobalScope(uid());
#endif

#if defined(STARFISH_ENABLE_IDB)
    IDBStorageManager::instance().dispose();
#endif
}

ExecutionContext* Window::executionContext() const
{
    return document()->executionContext();
}

// https://html.spec.whatwg.org/multipage/browsers.html#dom-parent
WindowProxy* Window::parent()
{
    if (browsingContext()->isTopLevelBrowsingContext()) {
        return window();
    }

    if (browsingContext()->parentBrowsingContext()) {
        return browsingContext()->parentBrowsingContext()->window()->window();
    }

    return nullptr;
}

WindowProxy* Window::top()
{
    if (browsingContext()->isTopLevelBrowsingContext()) {
        return window();
    }

    Window* current = this;
    while (current != nullptr &&
           !current->browsingContext()->isTopLevelBrowsingContext()) {
        current = current->parent()->window();
    }
    return current->window();
}

// https://w3c.github.io/html/browsers.html#dom-window-frameelement
Element* Window::frameElement()
{
    if (browsingContext()->isTopLevelBrowsingContext()) {
        return nullptr;
    }

    Document* source = document();
    Document* target = browsingContext()->sourceElement()->document();
    if (source && target && ScriptBindingSecurity::canAccess(source, target)) {
        return browsingContext()->sourceElement();
    }
    return nullptr;
}

Storage* Window::localStorage()
{
    auto storageInternal =
        browsingContext()->webView()->localStorageNamespace()->storageInternal(
            m_document->webOrigin());
    return new Storage(this, storageInternal);
}

Storage* Window::sessionStorage()
{
    auto storageInternal = browsingContext()
                               ->webView()
                               ->sessionStorageNamespace()
                               ->storageInternal(m_document->webOrigin());
    return new Storage(this, storageInternal);
}

void Window::postMessage(Window* source, ScriptValue message,
                         String* targetOrigin)
{
    GCAtomicVector<ScriptObject> emptyList;
    postMessage(source, message, targetOrigin, emptyList);
}

void Window::postMessage(Window* source, ScriptValue message,
                         String* targetOrigin,
                         GCAtomicVector<ScriptObject>& transfer)
{
    String* origin = source->location()->origin();
    if (targetOrigin->equals("/")) {
        targetOrigin = origin;
    } else if (targetOrigin->equals("*")) {
    } else if (!ResourceURL::isValidURL(targetOrigin)) {
        COMPOSE_MESSAGE(reason, INVALID_TARGET_ORIGIN,
                        targetOrigin->toUTF8NonGCString().data(),
                        "postMessage");
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "postMessage", "Window",
                        reason);
        throw new DOMException(document()->executionContext(),
                               DOMException::SYNTAX_ERR, msg);
    } else {
        ResourceURL* url = new ResourceURL(targetOrigin);
        targetOrigin = url->origin();
    }
    SerializeWithTransferResult* serializedRecord =
        new (GC) SerializeWithTransferResult();
    try {
        Serializer::serializeWithTransfer(document()->executionContext(),
                                          message, transfer, *serializedRecord);
    } catch (DOMException* e) {
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "postMessage", "Window",
                        e->message()->toUTF8NonGCString().data());
        STARFISH_ASSERT(msg != nullptr);
        e->setMessage(String::fromUTF8(msg, strlen(msg)));
        throw e;
    }

    // NOTE addIder would hold serializedRecord
    if (browsingContext()) {
        webView()->messageLoop()->addIdler(
            browsingContext()->window(),
            [](size_t handle, void* data, void* data1, void* data2) {
                Window* window = (Window*)data;
                SerializeWithTransferResult* serializedRecord =
                    (SerializeWithTransferResult*)data1;

                MessageEvent* event = new MessageEvent(
                    window->document()->executionContext(), serializedRecord);

                Window* source = (Window*)data2;

                event->setSource(MessageEventSource::createWindow(source));
                event->setOrigin(source->location()->origin());
                window->dispatchEventByUA(event);
            },
            this, serializedRecord, source);
    }
}

Screen* Window::screen()
{
    if (!m_screen) {
        m_screen = new Screen(m_document);
    }
    return m_screen.value();
}

int32_t Window::innerWidth()
{
    return m_width;
}

int32_t Window::innerHeight()
{
    return m_height;
}

bool Window::isInnerSizeEmpty()
{
    return m_width == 0 || m_height == 0;
}

static void checkVwVh(Node* nd)
{
    if (nd->isElement()) {
        if (nd->style() && nd->style()->seenViewPortUnitInStyle()) {
            nd->setNeedsStyleRecalc();
        }
    }

    Node* child = nd->firstChild();
    while (child) {
        if (child->isElement()) {
            checkVwVh(child);
        }
        child = child->nextSibling();
    }
}

void Window::resize(uint32_t w, uint32_t h)
{
    if (document()->styleResolver().mediaQueryAffectedByViewportChange()) {
        browsingContext()
            ->setNeedsStyleSheetsRecalcAndWholeDocumentNeedsStyleRecalc();
    } else {
        checkVwVh(document());
    }

    if (m_width != w || m_height != h) {
        m_width = w;
        m_height = h;
        document()->setNeedsLayout();

        String* eventType = staticStrings()->m_resize.localName();
        UIEvent* e = new UIEvent(document()->executionContext(), eventType);
        e->setView(this);
        if (browsingContext()->isTopLevelBrowsingContext()) {
            dispatchEventByUA(e);
        } else {
            dispatchEventIdleTimeByUA(e);
        }
    }

    // Change event will be fired at the MediaQueryList when the matches state
    // changes.
    document()->evalMediaQueryLists();
}

void Window::focus()
{
    STARFISH_UNSUPPORTED("Window function: focus");
}

void Window::blur()
{
    STARFISH_UNSUPPORTED("Window function: blur");
}

float Window::devicePixelRatio()
{
    return screen()->devicePixelRatio();
}

double Window::scrollX(bool canLeadLayoutThrashing)
{
    if (canLeadLayoutThrashing &&
        !browsingContext()->webView()->inRendering()) {
        browsingContext()->webView()->layoutIfNeeded(false);
    }
    if (document()->frame()) {
        return document()->frame()->asFrameDocument()->scrollLeft();
    }
    return 0;
}

double Window::scrollY(bool canLeadLayoutThrashing)
{
    if (canLeadLayoutThrashing &&
        !browsingContext()->webView()->inRendering()) {
        browsingContext()->webView()->layoutIfNeeded(false);
    }
    if (document()->frame()) {
        return document()->frame()->asFrameDocument()->scrollTop();
    }
    return 0;
}

double Window::pageXOffset()
{
    return scrollX();
}

double Window::pageYOffset()
{
    return scrollY();
}

LayoutUnit Window::scrollWidth(bool canLeadLayoutThrashing)
{
    if (canLeadLayoutThrashing &&
        !browsingContext()->webView()->inRendering()) {
        browsingContext()->webView()->layoutIfNeeded(false);
    }

    if (document()->frame()) {
        return document()->frame()->asFrameDocument()->scrollWidth();
    }
    return 0;
}

LayoutUnit Window::scrollHeight(bool canLeadLayoutThrashing)
{
    if (canLeadLayoutThrashing &&
        !browsingContext()->webView()->inRendering()) {
        browsingContext()->webView()->layoutIfNeeded(false);
    }

    if (document()->frame()) {
        return document()->frame()->asFrameDocument()->scrollHeight();
    }
    return 0;
}

bool Window::scrollToWithoutLayout(double x, double y)
{
    if (document()->frame()) {
        if (document()->frame()->asFrameBlockBox()->asFrameDocument()->scrollTo(
                x, y)) {
            m_scrolling->markAsActive();
            m_scrolling->giveDamageToTarget();

            String* eventType = staticStrings()->m_scroll.localName();
            UIEvent* e = new UIEvent(document()->executionContext(), eventType);
            e->setView(this);
            e->setTarget(document());
            if (document()->browsingContext()->isTopLevelBrowsingContext()) {
                dispatchEventByUA(e);
            } else {
                dispatchEventIdleTimeByUA(e);
            }

            return true;
        }
    }
    return false;
}

bool Window::scrollTo(ScrollToOptions options)
{
    LayoutUnit x;
    LayoutUnit y;

    if (options.hasLeft()) {
        x = options.left();
    } else {
        x = scrollX();
    }

    if (options.hasTop()) {
        y = options.top();
    } else {
        y = scrollY();
    }

    browsingContext()->webView()->layoutIfNeeded(false);
    return scrollToWithoutLayout(x.toDouble(), y.toDouble());
}

bool Window::scrollBy(ScrollToOptions options)
{
    return scrollTo(options.left() + scrollX(), options.top() + scrollY());
}

bool Window::scrollBy(double x, double y)
{
    return scrollTo(x + scrollX(), y + scrollY());
}

bool Window::handleDefaultEvent(Event* event)
{
    if (EventTarget::handleDefaultEvent(event)) {
        return true;
    }

    if (document() && document()->frame()) {
        if (m_scrolling->handleDefaultEvent(
                event, this, document()->frame()->asFrameBlockBox(),
                OverflowValue::AutoOverflow, OverflowValue::AutoOverflow)) {
            return true;
        }
    }
    return false;
}

void Window::onGlobalPointingEvent(float x, float y, DOMTimeStamp timeStamp,
                                   GlobalPointingEventKind kind)
{
    m_scrolling->onGlobalPointingEvent(x, y, timeStamp, kind);
}

#ifdef STARFISH_ENABLE_TEST
void Window::setNetworkState(bool state)
{
    int sockfd;
    struct ifreq ifr;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0) {
        return;
    }

    memset(&ifr, 0, sizeof ifr);
    strncpy(ifr.ifr_name, "eth0", IFNAMSIZ);

    if (state) {
        ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
    } else {
        ifr.ifr_flags |= ~IFF_RUNNING;
        // ifr.ifr_flags |= ~IFF_UP;
    }

    ioctl(sockfd, SIOCSIFFLAGS, &ifr);
}

void Window::forceDisableOnloadCapture()
{
    setenv("SCREEN_SHOT", "", 1);
}

void Window::simulateClick(float x, float y)
{
    TouchData data(x * webView()->screenInfo().devicePixelRatio,
                   y * webView()->screenInfo().devicePixelRatio);
    webView()->renderer()->dispatchTouchEvent(TouchEventKind::TouchEventStart,
                                              &data, 1);
    webView()->renderer()->dispatchTouchEvent(TouchEventKind::TouchEventEnd,
                                              &data, 1);
}

void Window::simulateMouseDown(float x, float y)
{
    MouseData data(MouseButtonValue::LeftButton,
                   MouseButtonsValue::LeftButtonDown,
                   x * webView()->screenInfo().devicePixelRatio,
                   y * webView()->screenInfo().devicePixelRatio, 0);
    webView()->renderer()->dispatchMouseEvent(MouseEventKind::MouseEventDown,
                                              data);
}

void Window::simulateMouseUp(float x, float y)
{
    MouseData data(MouseButtonValue::NoButton, MouseButtonsValue::NoButtonDown,
                   x * webView()->screenInfo().devicePixelRatio,
                   y * webView()->screenInfo().devicePixelRatio, 0);
    webView()->renderer()->dispatchMouseEvent(MouseEventKind::MouseEventUp,
                                              data);
}

void Window::simulateMouseMove(float x, float y)
{
    MouseData data(MouseButtonValue::NoButton, MouseButtonsValue::NoButtonDown,
                   x * webView()->screenInfo().devicePixelRatio,
                   y * webView()->screenInfo().devicePixelRatio, 0);
    webView()->renderer()->dispatchMouseEvent(MouseEventKind::MouseEventMove,
                                              data);
}

void Window::simulateVisibilitychange(bool show)
{
    if (show) {
        webView()->renderer()->resume();
    } else {
        webView()->renderer()->pause();
    }
}

void Window::testStart()
{
    invokeTestStartFunction(scriptBindingInstance());
}
#endif

uint32_t Window::setTimeout(TimerHandler handler, int32_t delay, void* data)
{
    return webView()->timer()->addTimer(delay, this, handler, data, false);
}

void Window::clearTimeout(int32_t id)
{
    webView()->timer()->removeTimer(id);
}

uint32_t Window::setInterval(TimerHandler handler, int32_t delay, void* data)
{
    return webView()->timer()->addTimer(delay, this, handler, data, true);
}

void Window::clearInterval(int32_t id)
{
    webView()->timer()->removeTimer(id);
}

String* Window::btoa(ExecutionContext* executionContext, String* data)
{
    return WindowOrWorkerGlobalScope::btoa(executionContext, data);
}

String* Window::atob(ExecutionContext* executionContext, String* data)
{
    return WindowOrWorkerGlobalScope::atob(executionContext, data);
}

void Window::queueMicrotask(ExecutionContext* executionContext,
                            ScriptObject callback)
{
    WindowOrWorkerGlobalScope::queueMicrotask(executionContext, callback);
}

#ifdef STARFISH_ENABLE_CANVAS
Promise* Window::createImageBitmap(ExecutionContext* executionContext,
                                   ImageBitmapSource image,
                                   ImageBitmapOptions options)
{
    return WindowOrWorkerGlobalScope::createImageBitmap(executionContext, image,
                                                        options);
}

Promise* Window::createImageBitmap(ExecutionContext* executionContext,
                                   ImageBitmapSource image, int32_t sx,
                                   int32_t sy, int32_t sw, int32_t sh,
                                   ImageBitmapOptions options)
{
    return WindowOrWorkerGlobalScope::createImageBitmap(
        executionContext, image, sx, sy, sw, sh, options);
}
#endif
void Window::alert()
{
    alert(String::emptyString);
}

void Window::alert(String* message)
{
    // calls the platform's alert UI
    struct Param {
        std::string title;
        std::string message;
    };

    Param* p = new Param();
    p->title =
        document()->location()->url()->origin()->toUTF8NonGCString().data();
    p->message = message->toUTF8NonGCString().data();
    webView()->renderer()->callHandler(WindowHandlerShowAlert, (void*)p);
}

void Window::processUrlFragment(String* name)
{
    Node* n = document()->getElementById(name);
    if (n) {
        setCSSTarget(n);
        return;
    }

    Node* anchor = Traverse::findDescendant(document(), [&](Node* child) {
        if (child->isHTMLAnchorElement() &&
            child->asHTMLAnchorElement()->name().localName()->equals(name)) {
            return true;
        } else {
            return false;
        }
    });

    if (anchor) {
        setCSSTarget(anchor);
    }
}

void Window::setCSSTarget(Node* n)
{
    releaseCSSTarget();

    m_cssTarget = n;
    if (m_cssTarget) {
        m_cssTarget->setState(Node::NodeStateTarget, true);
    }
}

void Window::releaseCSSTarget()
{
    if (m_cssTarget) {
        m_cssTarget->setState(Node::NodeStateTarget, false);
    }
}

void Window::dispatchErrorEvent(ErrorEventInit& errorInfo)
{
    Event* errorEvent =
        new ErrorEvent(document()->executionContext(),
                       staticStrings()->m_error.localName(), errorInfo);
    dispatchEventByUA(errorEvent);
}

bool Window::checkSecurityPolicy()
{
    return document()->contentSecurityPolicy()->allowEval(
        CSPDirectives::ScriptSrc);
}

Promise* Window::fetch(RequestInfo& input)
{
    return Fetch::fetch(executionContext(), input);
}

Promise* Window::fetch(RequestInfo& input, RequestInit& init)
{
    return Fetch::fetch(executionContext(), input, init);
}

Performance* Window::performance()
{
    if (!m_performance) {
        m_performance = Performance::create(executionContext());
    }
    return m_performance.value();
}

Crypto* Window::crypto()
{
    if (!m_crypto) {
        m_crypto = Crypto::create(executionContext());
    }
    return m_crypto.value();
}

CustomElementRegistry* Window::customElements()
{
    if (!m_customElementRegistry) {
        m_customElementRegistry = new CustomElementRegistry(executionContext());
    }
    return m_customElementRegistry.value();
}

DEFINE_EVENT_LISTENER(Window, abort);
DEFINE_EVENT_LISTENER(Window, blur);
DEFINE_EVENT_LISTENER(Window, click);
DEFINE_EVENT_LISTENER(Window, change);
DEFINE_EVENT_LISTENER(Window, error);
DEFINE_EVENT_LISTENER(Window, focus);
DEFINE_EVENT_LISTENER(Window, input);
DEFINE_EVENT_LISTENER(Window, invalid);
DEFINE_EVENT_LISTENER(Window, keydown);
DEFINE_EVENT_LISTENER(Window, keypress);
DEFINE_EVENT_LISTENER(Window, keyup);
DEFINE_EVENT_LISTENER(Window, load);
DEFINE_EVENT_LISTENER(Window, loadstart);
DEFINE_EVENT_LISTENER(Window, mousedown);
DEFINE_EVENT_LISTENER(Window, mousemove);
DEFINE_EVENT_LISTENER(Window, mouseover);
DEFINE_EVENT_LISTENER(Window, mouseout);
DEFINE_EVENT_LISTENER(Window, mouseenter);
DEFINE_EVENT_LISTENER(Window, mouseleave);
DEFINE_EVENT_LISTENER(Window, mouseup);
DEFINE_EVENT_LISTENER(Window, progress);
DEFINE_EVENT_LISTENER(Window, resize);
DEFINE_EVENT_LISTENER(Window, submit);
DEFINE_EVENT_LISTENER(Window, securitypolicyviolation);
DEFINE_EVENT_LISTENER(Window, message);
DEFINE_EVENT_LISTENER(Window, messageerror);
DEFINE_EVENT_LISTENER(Window, unload);
DEFINE_EVENT_LISTENER(Window, scroll);
DEFINE_EVENT_LISTENER(Window, ttsstart);
DEFINE_EVENT_LISTENER(Window, ttsend);
DEFINE_EVENT_LISTENER(Window, pointerdown);
DEFINE_EVENT_LISTENER(Window, pointerup);
DEFINE_EVENT_LISTENER(Window, pointermove);
#ifdef STARFISH_ENABLE_MULTIMEDIA
DEFINE_EVENT_LISTENER(Window, suspend);
DEFINE_EVENT_LISTENER(Window, emptied);
DEFINE_EVENT_LISTENER(Window, stalled);
DEFINE_EVENT_LISTENER(Window, loadedmetadata);
DEFINE_EVENT_LISTENER(Window, loadeddata);
DEFINE_EVENT_LISTENER(Window, canplay);
DEFINE_EVENT_LISTENER(Window, canplaythrough);
DEFINE_EVENT_LISTENER(Window, playing);
DEFINE_EVENT_LISTENER(Window, waiting);
DEFINE_EVENT_LISTENER(Window, seeking);
DEFINE_EVENT_LISTENER(Window, seeked);
DEFINE_EVENT_LISTENER(Window, ended);
DEFINE_EVENT_LISTENER(Window, durationchange);
DEFINE_EVENT_LISTENER(Window, timeupdate);
DEFINE_EVENT_LISTENER(Window, play);
DEFINE_EVENT_LISTENER(Window, pause);
DEFINE_EVENT_LISTENER(Window, ratechange);
DEFINE_EVENT_LISTENER(Window, volumechange);
#endif

CSSStyleDeclaration* Window::getComputedStyle(Element* element)
{
    return element->getComputedStyle();
}

CSSStyleDeclaration* Window::getComputedStyle(Element* element,
                                              Nullable<String*> pseudoElt)
{
    return element->getComputedStyle();
}

MediaQueryList* Window::matchMedia(String* query)
{
    if (!document()) {
        return nullptr;
    }

    return document()->mediaQueryListMatcher()->matchMedia(query);
}

// https://html.spec.whatwg.org/multipage/browsers.html#named-access-on-the-window-object
Nullable<ScriptObject> Window::defaultNamedGetter(String* name)
{
    // TODO
    // when child browser context(ex- iframe) implemented, we should
    // re-implement this block
    if (document()) {
        HTMLCollection* coll = document()->namedAccess(name);
        if (coll) {
            if (coll->length() > 1) {
                return coll->scriptObject();
            } else if (coll->length() == 1) {
                return coll->item(0)->scriptObject();
            }
        }
    }

    return Nullable<ScriptObject>();
}

Window* Window::defaultIndexedGetter(uint32_t idx)
{
    Node* item = ensureFrames()->item(idx);
    if (item) {
        if (item->isHTMLIFrameElement()) {
            STARFISH_ASSERT(item->asHTMLIFrameElement()->contentWindow());
            return item->asHTMLIFrameElement()->contentWindow()->window();
        } else {
            STARFISH_ASSERT_NOT_REACHED();
        }
        // TODO Handle HTMLFrameElement
        // else if (item->isHTMLFrameElement()) {}
    }
    return nullptr;
}

uint32_t Window::length()
{
    return ensureFrames()->length();
}

void Window::invalidateFramesIfNeeded()
{
    if (m_frames) {
        m_frames->getNodeListImpl().invalidateCache();
    }
}

static bool gatherFrames(Node* node, void* data, GCVector<Node*>* collection)
{
    StaticStrings* strings = (StaticStrings*)data;
    if (node->isElement()) {
        if (node->asElement()->name().localNameAtomic() ==
                strings->m_frameTagName.localNameAtomic() ||
            node->asElement()->name().localNameAtomic() ==
                strings->m_iframeTagName.localNameAtomic()) {
            return true;
        }
    }
    return false;
};

NodeList* Window::ensureFrames()
{
    if (!m_frames) {
        m_frames =
            new NodeList(document(), gatherFrames, staticStrings(), true);
    }
    return m_frames.value();
}

#ifdef STARFISH_ENABLE_TEST
void Window::screenShot(std::string filePath, void (*callback)(void*),
                        void* data)
{
    browsingContext()->webView()->renderer()->screenShot(filePath, callback,
                                                         data);
}
#endif

uint32_t Window::requestAnimationFrame(TimerHandler handler, void* data)
{
    return webView()->timer()->requestAnimationFrame(this, handler, data);
}

void Window::cancelAnimationFrame(int32_t reqID)
{
    webView()->timer()->cancelAnimationFrame(reqID);
}

String* Window::name()
{
    return m_browsingContext->name();
}

void Window::setName(String* name)
{
    m_browsingContext->setName(name);
}

#ifdef STARFISH_ENABLE_OBSOLETE_SPEC
Event* Window::event()
{
    return m_currentDispatchingEvent;
}

void Window::setEvent(Event* e)
{
    m_currentDispatchingEvent = e;
}
#endif

#if defined(STARFISH_ENABLE_IDB)
IDBFactory* Window::indexedDB()
{
    if (!m_idbFactory) {
        IDBStorageManager::instance().start();
        m_idbFactory = new IDBFactory(executionContext());
    }

    return m_idbFactory;
}
#endif

} // namespace Starfish
