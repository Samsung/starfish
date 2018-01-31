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

#include "StarFishConfig.h"
#include "StarFish.h"

#include "core/page/Window.h"

#include "binding/ScriptBindingInstance.h"
#include "core/dom/DOMException.h"
#include "core/dom/ErrorEvent.h"
#include "core/dom/HTMLAnchorElement.h"
#include "core/dom/HTMLDocument.h"
#include "core/dom/HTMLIFrameElement.h"
#include "core/dom/HTMLCollection.h"
#include "core/dom/MessageEvent.h"
#include "core/dom/NodeList.h"
#include "core/dom/Traverse.h"
#include "core/dom/TouchEvent.h"
#include "core/dom/WebOrigin.h"
#include "core/extra/Console.h"
#include "core/layout/FrameDocument.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/message_loop/Timer.h"
#include "core/page/BrowsingContext.h"
#include "core/page/History.h"
#include "core/page/Navigator.h"
#include "core/page/Location.h"
#include "core/page/Screen.h"
#include "core/page/WebView.h"
#include "core/page/Serializer.h"
#include "core/page/SecurityOriginData.h"
#include "core/page/Serializer.h"
#include "core/storage/Storage.h"
#include "core/storage/StorageNamespace.h"
#include "core/style/CSSParser.h"
#include "core/style/MediaQueryList.h"
#include "platform/window/PlatformWindow.h"

#ifdef STARFISH_ENABLE_TEST
#include <sys/ioctl.h>
#include <net/if.h>
#endif

namespace StarFish {

Window* Window::create(StarFish* starFish, BrowsingContext* browsingContext,
                       ResourceURL* url, uint32_t initialWidth,
                       uint32_t initialHeight)
{
    return new Window(starFish, browsingContext, url, initialWidth,
                      initialHeight);
}

Window::Window(StarFish* starFish, BrowsingContext* browsingContext,
               ResourceURL* url, uint32_t initialWidth, uint32_t initialHeight)
    : EventTarget(nullptr)
    , m_starFish(starFish)
    , m_browsingContext(browsingContext)
    , m_history(nullptr)
    , m_navigator(nullptr)
    , m_location(nullptr)
    , m_screen(nullptr)
    , m_scrolling(new Scrolling(this))
    , m_width(initialWidth)
    , m_height(initialHeight)
    , m_frames(nullptr)
{
    /*
        GC_REGISTER_FINALIZER_NO_ORDER(
            this,
            [](void* obj, void* cd) { STARFISH_LOG_INFO("Window::~Window\n"); },
            NULL, NULL, NULL);
    */
    m_scriptBindingInstance = new ScriptBindingInstance(
        browsingContext->webView()->scriptEngineInstance(), this);
    initFlags();

    // TODO: use location to open a new document
    m_document = new HTMLDocument(this, m_scriptBindingInstance, url,
                                  String::createASCIIString("UTF-8"), true);

    m_history = new History(m_document);
    m_navigator = new Navigator(m_document);
    m_location = new Location(m_document);
    m_scriptBindingInstance->initBinding(m_document);
}

void Window::deleteScriptBindingInstance()
{
}

void Window::initFlags()
{
}

void Window::dispose()
{
    clearEventListeners();

    ResourceURL* url = m_document->documentURI();
    m_location->dispose();
    m_navigator->dispose();
}

// https://html.spec.whatwg.org/multipage/browsers.html#dom-parent
Window* Window::parent()
{
    if (browsingContext()->isTopLevelBrowsingContext()) {
        return this;
    }

    bool ignoreCrossOrigin = false;
#ifdef STARFISH_IGNORE_CROSS_ORIGIN
    ignoreCrossOrigin = true;
#endif

    if (ignoreCrossOrigin ||
        (browsingContext()->sourceElement()->isInDocumentScope() &&
         browsingContext()->document()->webOrigin()->isSameOriginDomain(
             browsingContext()
                 ->parentBrowsingContext()
                 ->document()
                 ->webOrigin()))) {
        return browsingContext()->parentBrowsingContext()->window();
    }

    return nullptr;
}

Window* Window::top()
{
    if (browsingContext()->isTopLevelBrowsingContext()) {
        return this;
    }

    Window* current = this;
    while (current != nullptr &&
           !current->browsingContext()->isTopLevelBrowsingContext()) {
        current = current->parent();
    }
    return current;
}

// https://w3c.github.io/html/browsers.html#dom-window-frameelement
Element* Window::frameElement()
{
    if (browsingContext()->isTopLevelBrowsingContext()) {
        return nullptr;
    }

    bool ignoreCrossOrigin = false;
#ifdef STARFISH_IGNORE_CROSS_ORIGIN
    ignoreCrossOrigin = true;
#endif

    if (ignoreCrossOrigin ||
        (browsingContext()->document()->webOrigin()->isSameOriginDomain(
            browsingContext()
                ->parentBrowsingContext()
                ->document()
                ->webOrigin()))) {
        HTMLIFrameElement* frameElement = browsingContext()->sourceElement();
        return frameElement;
    }

    return nullptr;
}

Storage* Window::localStorage()
{
    ResourceURL* url = m_document->documentURI();
    SecurityOriginData* origin = new SecurityOriginData(
        url->protocol(), url->host(), String::parseInt(url->port()));
    return browsingContext()->webView()->localStorageNamespace()->storage(
        this, origin);
}

Storage* Window::sessionStorage()
{
    ResourceURL* url = m_document->documentURI();
    SecurityOriginData* origin = new SecurityOriginData(
        url->protocol(), url->host(), String::parseInt(url->port()));
    return browsingContext()->webView()->sessionStorageNamespace()->storage(
        this, origin);
}

void Window::postMessage(ScriptValue message, String* targetOrigin)
{
    GCVector<ScriptValue> emptyList;
    postMessage(message, targetOrigin, emptyList);
}

void Window::postMessage(ScriptValue message, String* targetOrigin,
                         GCVector<ScriptValue>& transfer)
{
    Window* source = parent();
    while (!source->browsingContext()->isTopLevelBrowsingContext()) {
        source = source->parent();
    }
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
        throw new DOMException(document(), DOMException::SYNTAX_ERR, msg);
    } else {
        ResourceURL* url = new ResourceURL(targetOrigin);
        targetOrigin = url->origin();
    }
    SerializeWithTransferResult* serializedRecord =
        new (GC) SerializeWithTransferResult();
    try {
        Serializer::serializeWithTransfer(document(), message, transfer,
                                          *serializedRecord);
    } catch (DOMException* e) {
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "postMessage", "Window",
                        e->message()->toUTF8NonGCString().data());
        e->setMessage(String::fromUTF8(msg));
        throw e;
    }

#ifndef STARFISH_IGNORE_CROSS_ORIGIN
    if (!targetOrigin->equals("*") && !targetOrigin->equals("about:blank") &&
        !targetOrigin->equals(origin)) {
        COMPOSE_MESSAGE(reason, ORIGINS_ARE_NOT_MATCHED,
                        targetOrigin->toUTF8NonGCString().data(),
                        origin->toUTF8NonGCString().data());
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "postMessage", "Window",
                        reason);
        starFish()->console()->error(String::fromUTF8(msg));
        return;
    }
#endif
    // NOTE addIder would hold serializedRecord
    if (browsingContext()) {
        starFish()->messageLoop()->addIdler(
            browsingContext(),
            [](size_t handle, void* data, void* data1) {
                Window* window = (Window*)data;
                SerializeWithTransferResult* serializedRecord =
                    (SerializeWithTransferResult*)data1;
                DeserializeWithTransferResult deserializedRecord;
                bool fail = false;
                try {
                    Serializer::deserializeWithTransfer(window->document(),
                                                        *serializedRecord,
                                                        deserializedRecord);
                } catch (DOMException* e) {
                    fail = true;
                }
                MessageEvent* e;
                String* eventType;
                if (fail == false) {
                    eventType = window->starFish()
                                    ->staticStrings()
                                    ->m_message.localName();
                    e = new MessageEvent(window->document(), eventType);
                    e->setData(deserializedRecord.m_deserialized);

                    GCVector<MessagePort*> newPorts;
                    for (size_t i = 0;
                         i < deserializedRecord.m_deserializedTransfer.size();
                         i++) {
                        ScriptValue item =
                            deserializedRecord.m_deserializedTransfer[i];
                        STARFISH_ASSERT(isObjectScriptValue(item));
                        ScriptWrappable* sw = toScriptWrappable(item);
                        if (sw && sw->isMessagePort()) {
                            newPorts.push_back(sw->asMessagePort());
                        }
                    }
                    e->setPorts(newPorts);

                } else {
                    eventType = window->starFish()
                                    ->staticStrings()
                                    ->m_messageerror.localName();
                    e = new MessageEvent(window->document(), eventType);
                }
                Window* source = window->parent();
                while (
                    !source->browsingContext()->isTopLevelBrowsingContext()) {
                    source = source->parent();
                }
                e->setSource(source);
                e->setOrigin(source->location()->origin());
                window->dispatchEventByUA(e);
            },
            this, serializedRecord);
    }
}

Screen* Window::screen()
{
    if (!m_screen) {
        m_screen = new Screen(m_document);
    }
    return m_screen;
}

int32_t Window::innerWidth()
{
    return width();
}

int32_t Window::innerHeight()
{
    return height();
}

int32_t Window::width()
{
    return m_width;
}

int32_t Window::height()
{
    return m_height;
}

void Window::resize(uint32_t w, uint32_t h)
{
    if (document()->styleResolver().mediaQueryAffectedByViewportChange()) {
        browsingContext()->setNeedsStyleSheetsRecalc();
    } else {
        if (document()->frame()) {
            document()->frame()->asFrameBox()->iterateChildFrameBox([](
                FrameBox* box) {
                if (box->style() && box->style()->seenViewPortUnitInStyle()) {
                    box->nearstNotAnonymousNode()->setNeedsStyleRecalc();
                }
            });
        }
    }

    if (m_width != w || m_height != h) {
        m_width = w;
        m_height = h;
        String* eventType = starFish()->staticStrings()->m_resize.localName();
        UIEvent* e = new UIEvent(document(), eventType);
        e->setView(this);
        dispatchEventByUA(this, e);
        browsingContext()->setNeedsLayout();
    }
}

void Window::focus()
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}

void Window::blur()
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}

float Window::devicePixelRatio()
{
    return screen()->devicePixelRatio();
}

double Window::scrollX()
{
    browsingContext()->webView()->layoutIfNeeds();
    if (document()->frame()) {
        return document()->frame()->asFrameBlockBox()->scrollLeft();
    }
    return 0;
}

double Window::scrollY()
{
    browsingContext()->webView()->layoutIfNeeds();
    if (document()->frame()) {
        return document()->frame()->asFrameBlockBox()->scrollTop();
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

    browsingContext()->webView()->layoutIfNeeds();
    if (document()->frame()) {
        if (document()->frame()->asFrameBlockBox()->asFrameDocument()->scrollTo(
                x, y)) {
            if (webView()->didCompositeBefore()) {
                browsingContext()->setNeedsComposite();
            } else {
                browsingContext()->setNeedsPainting();
            }
            return true;
        }
    }
    return false;
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

void Window::onGlobalPointingEvent(float x, float y,
                                   GlobalPointingEventKind kind)
{
    m_scrolling->onGlobalPointingEvent(x, y, kind);
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
    TouchData data(x, y);
    m_starFish->platformWindow()->dispatchTouchEvent(
        PlatformWindow::TouchEventStart, &data, 1);
    m_starFish->platformWindow()->dispatchTouchEvent(
        PlatformWindow::TouchEventEnd, &data, 1);
}

void Window::simulateVisibilitychange(bool show)
{
    if (show) {
        m_starFish->resume();
    } else {
        m_starFish->pause();
    }
}

void Window::testStart()
{
    invokeTestStartFunction(scriptBindingInstance());
}
#endif

uint32_t Window::setTimeout(WindowSetTimeoutHandler handler, int32_t delay,
                            void* data)
{
    STARFISH_RELEASE_ASSERT(browsingContext()->m_isActive);
    return m_starFish->timer()->addTimer(delay, this, handler, data, false);
}

void Window::clearTimeout(int32_t id)
{
    STARFISH_RELEASE_ASSERT(browsingContext()->m_isActive);
    m_starFish->timer()->removeTimer(id);
}

uint32_t Window::setInterval(WindowSetTimeoutHandler handler, int32_t delay,
                             void* data)
{
    STARFISH_RELEASE_ASSERT(browsingContext()->m_isActive);
    return m_starFish->timer()->addTimer(delay, this, handler, data, true);
}

void Window::clearInterval(int32_t id)
{
    STARFISH_RELEASE_ASSERT(browsingContext()->m_isActive);
    m_starFish->timer()->removeTimer(id);
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
    // TODO Invoking attribute listener(onerror) should differ
    // ErrorEventInit errorInfo(message, filename, lineno, colno, errorScript);
    Event* errorEvent = new ErrorEvent(
        document(), starFish()->staticStrings()->m_error.localName(),
        errorInfo);
    dispatchEventByUA(errorEvent);
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
DEFINE_EVENT_LISTENER(Window, mouseup);
DEFINE_EVENT_LISTENER(Window, progress);
DEFINE_EVENT_LISTENER(Window, resize);
DEFINE_EVENT_LISTENER(Window, submit);
DEFINE_EVENT_LISTENER(Window, message);
DEFINE_EVENT_LISTENER(Window, messageerror);
DEFINE_EVENT_LISTENER(Window, unload);
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

    CSSParser parser(document());
    RefPtr<CSSToken> token = parser.makeToken(query);
    MediaQuerySet* mediaQuerySet = parser.parseMediaQuery();
    MediaQueryEvaluator* mediaQueryEvaluator = const_cast<MediaQueryEvaluator*>(
        &document()->styleResolver().mediaQueryEvaluator());
    return new MediaQueryList(scriptBindingInstance(), mediaQuerySet,
                              mediaQueryEvaluator);
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
            return item->asHTMLIFrameElement()->contentWindow();
        }
        // TODO Handle HTMLFrameElement
        // else if (item->isHTMLFrameElement()) {}
        else {
            STARFISH_ASSERT_NOT_REACHED();
        }
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
        m_frames = new NodeList(document(), gatherFrames,
                                starFish()->staticStrings(), true);
    }
    return m_frames;
}

#ifdef STARFISH_ENABLE_TEST
void Window::screenShot(std::string filePath)
{
    browsingContext()->starFish()->platformWindow()->screenShot(filePath);
}
#endif

uint32_t Window::requestAnimationFrame(WindowSetTimeoutHandler handler,
                                       void* data)
{
    STARFISH_RELEASE_ASSERT(browsingContext()->m_isActive);
    return m_starFish->timer()->addAnimator(this, handler, data);
}

void Window::cancelAnimationFrame(int32_t reqID)
{
    m_starFish->timer()->removeWindowAnimator(reqID);
}

String* Window::name()
{
    return m_browsingContext->name();
}

void Window::setName(String* name)
{
    m_browsingContext->setName(name);
}
}
