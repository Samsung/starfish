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
#include "core/dom/HTMLAnchorElement.h"
#include "core/dom/HTMLDocument.h"
#include "core/dom/HTMLCollection.h"
#include "core/dom/Traverse.h"
#include "core/page/BrowsingContext.h"
#include "core/page/History.h"
#include "core/page/Navigator.h"
#include "core/page/Location.h"
#include "core/page/Screen.h"
#include "core/page/WebView.h"

#include "core/page/SecurityOriginData.h"
#include "core/storage/Storage.h"
#include "core/storage/StorageNamespace.h"
#include "core/style/CSSParser.h"
#include "core/style/MediaQueryList.h"

#if defined(STARFISH_TIZEN_TV) && defined(STARFISH_ENABLE_AVPLAY)
#include "core/extra/WebApis.h"
#endif
#include "platform/window/PlatformWindow.h"

#include "core/modules/message_loop/Timer.h"

#ifdef STARFISH_ENABLE_TEST
#include <sys/ioctl.h>
#include <net/if.h>
#endif

namespace StarFish {

Window* Window::create(StarFish* starFish, BrowsingContext* browsingContext,
                       ResourceURL* url)
{
    return new Window(starFish, browsingContext, url);
}

Window::Window(StarFish* starFish, BrowsingContext* browsingContext,
               ResourceURL* url)
    : EventTarget(nullptr)
    , m_starFish(starFish)
    , m_browsingContext(browsingContext)
    , m_history(nullptr)
    , m_navigator(nullptr)
    , m_location(nullptr)
    , m_screen(nullptr)
#if defined(STARFISH_TIZEN_TV) && defined(STARFISH_ENABLE_AVPLAY)
    , m_webapis(nullptr)
#endif
{
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
    m_scriptBindingInstance = nullptr;
}

void Window::initFlags()
{
}

void Window::close()
{
    clearEventListeners();
    m_location->close();
    m_navigator->close();
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
    return m_starFish->platformWindow()->width();
}

int32_t Window::height()
{
    return m_starFish->platformWindow()->height();
}

float Window::devicePixelRatio()
{
    return screen()->devicePixelRatio();
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
    m_starFish->platformWindow()->dispatchTouchEvent(
        x, y, PlatformWindow::TouchEventStart, true);
    m_starFish->platformWindow()->dispatchTouchEvent(
        x, y, PlatformWindow::TouchEventEnd, true);
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
        m_cssTarget->setState(Node::NodeStateTarget, Node::NotAffected, true);
    }
}

void Window::releaseCSSTarget()
{
    if (m_cssTarget) {
        m_cssTarget->setState(Node::NodeStateTarget, Node::NotAffected, false);
    }
}

DEFINE_EVENT_LISTENER(Window, abort);
DEFINE_EVENT_LISTENER(Window, canplay);
DEFINE_EVENT_LISTENER(Window, canplaythrough);
DEFINE_EVENT_LISTENER(Window, click);
DEFINE_EVENT_LISTENER(Window, durationchange);
DEFINE_EVENT_LISTENER(Window, emptied);
DEFINE_EVENT_LISTENER(Window, ended);
DEFINE_EVENT_LISTENER(Window, error);
DEFINE_EVENT_LISTENER(Window, focus);
DEFINE_EVENT_LISTENER(Window, keydown);
DEFINE_EVENT_LISTENER(Window, keyup);
DEFINE_EVENT_LISTENER(Window, load);
DEFINE_EVENT_LISTENER(Window, loadeddata);
DEFINE_EVENT_LISTENER(Window, loadedmetadata);
DEFINE_EVENT_LISTENER(Window, loadstart);
DEFINE_EVENT_LISTENER(Window, mouseover);
DEFINE_EVENT_LISTENER(Window, pause);
DEFINE_EVENT_LISTENER(Window, play);
DEFINE_EVENT_LISTENER(Window, playing);
DEFINE_EVENT_LISTENER(Window, progress);
DEFINE_EVENT_LISTENER(Window, ratechange);
DEFINE_EVENT_LISTENER(Window, seeked);
DEFINE_EVENT_LISTENER(Window, seeking);
DEFINE_EVENT_LISTENER(Window, stalled);
DEFINE_EVENT_LISTENER(Window, suspend);
DEFINE_EVENT_LISTENER(Window, timeupdate);
DEFINE_EVENT_LISTENER(Window, volumechange);
DEFINE_EVENT_LISTENER(Window, waiting);

DEFINE_EVENT_LISTENER(Window, unload);

CSSStyleDeclaration* Window::getComputedStyle(Element* element)
{
    return element->getComputedStyle();
}

CSSStyleDeclaration* Window::getComputedStyle(Element* element,
                                              String* pseudoElt)
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
HTMLCollection* Window::namedAccess(String* name)
{
    // TODO
    // when child browser context(ex- iframe) implemented, we should
    // re-implement this block
    if (document()) {
        return document()->namedAccess(name);
    } else {
        return nullptr;
    }
}

void Window::screenShot(std::string filePath)
{
    browsingContext()->starFish()->platformWindow()->screenShot(filePath);
}

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
}
