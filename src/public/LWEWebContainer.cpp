/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#include "StarFishConfig.h"
#include "StarFish.h"

#include "LWEWebView.h"
#include "platform/window/PlatformWindow.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "core/page/WebView.h"
#include "core/page/History.h"
#include "core/page/Location.h"
#include "core/dom/Document.h"
#include "binding/ScriptWrappable.h"
#include "JavaScriptNativeHandler.h"
#include "core/modules/threading/Thread.h"
#include "core/dom/MouseEvent.h"
#include "core/dom/KeyboardEvent.h"
#include "platform/network/HTTPCache.h"
#include "core/dom/MouseEvent.h"
#include "core/event/KeyBoardEventData.h"
#include "platform/event/PlatformKeyEventData.h"

#include <EscargotPublic.h>

#define TO_STARFISH(ptr) ((StarFish::StarFish*)ptr)
#define TO_HISTORY(ptr)         \
    ((StarFish::StarFish*)ptr)  \
        ->platformWindow()      \
        ->webView()             \
        ->mainBrowsingContext() \
        ->window()              \
        ->history()

#define TO_LOCATION(ptr)        \
    ((StarFish::StarFish*)ptr)  \
        ->platformWindow()      \
        ->webView()             \
        ->mainBrowsingContext() \
        ->window()              \
        ->location()

#define TO_RESOURCE_LOADER(ptr) \
    ((StarFish::StarFish*)ptr)  \
        ->platformWindow()      \
        ->webView()             \
        ->mainBrowsingContext() \
        ->document()            \
        ->resourceLoader()

#define TO_SCRIPT_BINDING_INSTANCE(ptr) \
    ((StarFish::StarFish*)ptr)          \
        ->platformWindow()              \
        ->webView()                     \
        ->mainBrowsingContext()         \
        ->window()                      \
        ->scriptBindingInstance()

namespace LWE {

static StarFish::ScriptValue nativeCallbackFunction(
    StarFish::ScriptExecutionState state, StarFish::ScriptValue thisValue,
    size_t argc, StarFish::ScriptValue* argv, bool isNewExpression)
{
    auto callee = StarFish::toCalleeObject(state);
    if (callee) {
        void* data = callee->extraData();
        if (data) {
            StarFish::ScriptWrappable* w = (StarFish::ScriptWrappable*)data;
            if (w->isJavaScriptNativeHandler()) {
                StarFish::JavaScriptNativeHandler* jsNhandler =
                    (StarFish::JavaScriptNativeHandler*)w;
                StarFish::String* result = StarFish::String::emptyString;
                StarFish::String* param = StarFish::String::emptyString;
                if (argc > 0) {
                    StarFish::ScriptValue arg0 = argv[0];
                    param = StarFish::toBrowserString(state, arg0);
                }
                result = jsNhandler->callNativeHandler(param);
                return StarFish::createScriptValue(
                    StarFish::createScriptString(result));
            }
        }
    }
    return StarFish::scriptUndefined();
}

WebContainer* WebContainer::Create(void* buffer, uint width, uint height,
                                   uint stride, float scaleFactor)
{
#if !defined(PORT_GRAPHIC_BACKEND_GENERAL_BUFFER)
    STARFISH_LOG_ERROR("Cannot use WebContainer this port!");
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
    return nullptr;
#endif
    std::string screenShot;
    std::string customUserAgentString;
    std::string builtinPolyfillPathString;
    int flag = 0;

    const char* defaultFontName = "serif";

#if defined(STARFISH_DALI)
    defaultFontName = "samsungOne";
#endif

#if defined(STARFISH_ANDROID)
    std::string tempPath = "/sdcard/tmp/";
#else
    std::string tempPath = "/tmp/";
#endif

#if defined(OS_WINDOWS)
    tempPath = ::StarFish::getWindowsTempDir();
#endif

    std::string localStoragePath = tempPath;
    localStoragePath += "StarFish_localStorage.txt";

    std::string cookiePath = tempPath;
    localStoragePath += "StarFish_Cookies.txt";

    StarFish::ScreenInfo info;
    info.rect.setWidth(width);
    info.rect.setHeight(height);
    info.availableRect.setWidth(width);
    info.availableRect.setHeight(height);
    info.deviceScaleFactor = scaleFactor;

    std::string cacheDir = tempPath;
    cacheDir += "Starfish-cache";
    StarFish::StarFish* starfish = new StarFish::StarFish(
        (StarFish::StarFishStartUpFlag)flag, "ko-KR", "Asia/Seoul", nullptr,
        width, height, 0, 0, 1,
        StarFish::String::createASCIIString(defaultFontName), info,
        localStoragePath.data(), cookiePath.data(), cacheDir.data(),
        StarFish::String::fromUTF8(customUserAgentString.data()),
        StarFish::String::fromUTF8(builtinPolyfillPathString.data()));
    starfish->platformWindow()->updateDrawingBufferAddress(buffer, width,
                                                           height, stride);

    WebContainer* newWebContainer =
        new (GC_MALLOC_UNCOLLECTABLE(sizeof(WebView))) WebContainer(starfish);

#if defined(STARFISH_ANDROID)
    starfish->setLWEWebView(newWebContainer);
#endif
    return newWebContainer;
}

WebContainer::WebContainer(void* starFish)
    : m_starfish(starFish)
{
}

Settings WebContainer::GetSettings()
{
    STARFISH_ASSERT(m_starfish);
    Settings result(USER_AGENT(STARFISH_NAME, VERSION),
                    TO_STARFISH(m_starfish)->userAgent()->toUTF8NonGCString());
#ifdef STARFISH_ENABLE_HTTPCACHE
    result.SetCacheMode(TO_STARFISH(m_starfish)->httpCache()->cacheMode());
#endif
    result.SetProxyURL(TO_STARFISH(m_starfish)->proxyURL());
    return result;
}

void WebContainer::LoadURL(const std::string& url)
{
    STARFISH_ASSERT(m_starfish);
    TO_STARFISH(m_starfish)
        ->loadHTMLDocument(StarFish::String::fromUTF8(url.data()));
}

std::string WebContainer::GetURL()
{
    STARFISH_ASSERT(m_starfish);
    return TO_LOCATION(m_starfish)->url()->urlString()->toUTF8NonGCString();
}

void WebContainer::LoadData(const std::string& data)
{
    STARFISH_ASSERT(m_starfish);
    if (data.size() > 0) {
        auto dataURI = StarFish::StringUtils::toBase64HTMLDataURI(data);
        TO_STARFISH(m_starfish)
            ->loadHTMLDocument(StarFish::String::fromUTF8(dataURI.data()));
    } else {
        TO_STARFISH(m_starfish)
            ->loadHTMLDocument(StarFish::String::fromUTF8("about:blank"));
    }
}

void WebContainer::Reload()
{
    STARFISH_ASSERT(m_starfish);
    TO_LOCATION(m_starfish)->reload(true);
}

void WebContainer::StopLoading()
{
    STARFISH_ASSERT(m_starfish);
    TO_RESOURCE_LOADER(m_starfish).clear();
}

void WebContainer::GoBack()
{
    STARFISH_ASSERT(m_starfish);
    TO_HISTORY(m_starfish)->back();
}

void WebContainer::GoForward()
{
    STARFISH_ASSERT(m_starfish);
    TO_HISTORY(m_starfish)->forward();
}

bool WebContainer::CanGoBack()
{
    STARFISH_ASSERT(m_starfish);
    return TO_HISTORY(m_starfish)->canGoBack();
}

bool WebContainer::CanGoForward()
{
    STARFISH_ASSERT(m_starfish);
    return TO_HISTORY(m_starfish)->canGoForward();
}

void WebContainer::AddJavaScriptInterface(
    const std::string& exposedObjectName, const std::string& jsFunctionName,
    std::function<std::string(const std::string&)> cb)
{
    STARFISH_ASSERT(m_starfish);

    StarFish::String* objectName =
        StarFish::String::fromUTF8(exposedObjectName.c_str());
    StarFish::String* functionName =
        StarFish::String::fromUTF8(jsFunctionName.c_str());

    StarFish::registerJavaScriptNativeInterface(
        TO_SCRIPT_BINDING_INSTANCE(m_starfish), objectName, functionName,
        new StarFish::JavaScriptNativeHandler(TO_STARFISH(m_starfish),
                                              functionName, cb),
        nativeCallbackFunction);
}

std::string WebContainer::EvaluateJavaScript(const std::string& script)
{
    STARFISH_ASSERT(m_starfish);
    return TO_STARFISH(m_starfish)
        ->evaluate(StarFish::String::fromUTF8(script.c_str()))
        ->toUTF8NonGCString();
}

void WebContainer::ClearHistory()
{
    STARFISH_ASSERT(m_starfish);
    TO_STARFISH(m_starfish)
        ->platformWindow()
        ->webView()
        ->historyManager()
        ->clear();
}

void WebContainer::Destroy()
{
    STARFISH_ASSERT(m_starfish);
    TO_STARFISH(m_starfish)->close();
    m_starfish = nullptr;

    GC_FREE(this);

    GC_gcollect_and_unmap();
    GC_gcollect_and_unmap();
    GC_gcollect_and_unmap();
}

void WebContainer::SetSettings(const Settings& settings)
{
    STARFISH_ASSERT(m_starfish);
    TO_STARFISH(m_starfish)
        ->setCustomUserAgentString(
            StarFish::String::fromUTF8(settings.GetUserAgentString().c_str()));
    TO_STARFISH(m_starfish)->setProxyURL(settings.GetProxyURL());
#ifdef STARFISH_ENABLE_HTTPCACHE
    TO_STARFISH(m_starfish)->httpCache()->setCacheMode(settings.GetCacheMode());
#endif
}

void WebContainer::RemoveJavascriptInterface(
    const std::string& exposedObjectName, const std::string& jsFunctionName)
{
    STARFISH_ASSERT(m_starfish);
    StarFish::String* objectName =
        StarFish::String::fromUTF8(exposedObjectName.c_str());
    if (!jsFunctionName.empty()) {
        StarFish::String* functionName =
            StarFish::String::fromUTF8(jsFunctionName.c_str());
        StarFish::unregisterJavaScriptNativeInterface(
            TO_SCRIPT_BINDING_INSTANCE(m_starfish), objectName, functionName);
    } else {
        StarFish::unregisterJavaScriptNativeInterface(
            TO_SCRIPT_BINDING_INSTANCE(m_starfish), objectName);
    }
}
void WebContainer::ClearCache()
{
    STARFISH_ASSERT(m_starfish);
#ifdef STARFISH_ENABLE_HTTPCACHE
    if (TO_STARFISH(m_starfish)->httpCache() != nullptr) {
        TO_STARFISH(m_starfish)->httpCache()->clear();
    }
#endif
}

void WebContainer::RegisterOnReceivedErrorHandler(
    const std::function<void(LWE::WebContainer*, LWE::ResourceError)>& cb)
{
    TO_STARFISH(m_starfish)
        ->registerWebViewHandler(
            std::string("OnReceivedError"),
            [this, cb](StarFish::String* url, int errorCode) -> void {
                // make error description
                cb(this, ResourceError(errorCode, std::string()));
            });
}

void WebContainer::RegisterOnPageFinishedHandler(
    const std::function<void(LWE::WebContainer*, const std::string&)>& cb)
{
    TO_STARFISH(m_starfish)
        ->registerWebViewHandler(
            std::string("OnPageFinished"),
            [this, cb](StarFish::String* url, int errorCode) -> void {
                cb(this, url->toUTF8NonGCString());
            });
}

void WebContainer::RegisterOnPageStartedHandler(
    const std::function<void(LWE::WebContainer*, const std::string&)>& cb)
{
    TO_STARFISH(m_starfish)
        ->registerWebViewHandler(
            std::string("OnPageStarted"),
            [this, cb](StarFish::String* url, int errorCode) -> void {
                cb(this, url->toUTF8NonGCString());
            });
}

void WebContainer::RegisterOnLoadResourceHandler(
    const std::function<void(LWE::WebContainer*, const std::string&)>& cb)
{
    TO_STARFISH(m_starfish)
        ->registerWebViewHandler(
            std::string("OnLoadResource"),
            [this, cb](StarFish::String* url, int errorCode) -> void {
                cb(this, url->toUTF8NonGCString());
            });
}

void WebContainer::UpdateBuffer(void* buffer, uint width, uint height,
                                uint stride)
{
    TO_STARFISH(m_starfish)
        ->platformWindow()
        ->updateDrawingBufferAddress(buffer, width, height, stride);
}

size_t WebContainer::width()
{
    return TO_STARFISH(m_starfish)->platformWindow()->width();
}

size_t WebContainer::height()
{
    return TO_STARFISH(m_starfish)->platformWindow()->height();
}

void WebContainer::RegisterOnRenderedHandler(
    const std::function<void(LWE::WebContainer*, void*)>& cb)
{
    TO_STARFISH(m_starfish)
        ->platformWindow()
        ->registerRenderingFinishedCallback([this, cb]() {
            cb(this, TO_STARFISH(m_starfish)
                         ->platformWindow()
                         ->drawingBufferAddress());
        });
}

void WebContainer::RegisterOnProgressChangedHandler(
    const std::function<void(LWE::WebContainer*, int)>& cb)
{
    TO_STARFISH(m_starfish)
        ->registerWebViewHandler(
            std::string("OnProgressChanged"),
            [this, cb](StarFish::String* url, int newProgress) -> void {
                cb(this, newProgress);
            });
}

void WebContainer::SetUserAgentString(const std::string& userAgent)
{
    TO_STARFISH(m_starfish)
        ->setCustomUserAgentString(
            StarFish::String::fromUTF8(userAgent.c_str()));
}

void WebContainer::SetCacheMode(int mode)
{
#ifdef STARFISH_ENABLE_HTTPCACHE
    if (TO_STARFISH(m_starfish)->httpCache() != nullptr) {
        TO_STARFISH(m_starfish)->httpCache()->setCacheMode(mode);
    }
#endif
}

void WebContainer::DispatchMouseMoveEvent(MouseButtonValue button,
                                          MouseButtonsValue buttons, double x,
                                          double y)
{
    TO_STARFISH(m_starfish)
        ->platformWindow()
        ->dispatchMouseEvent(::StarFish::MouseEventKind::MouseEventMove,
                             ::StarFish::MouseData(button, buttons, x, y, 0));
}

void WebContainer::DispatchMouseDownEvent(MouseButtonValue button,
                                          MouseButtonsValue buttons, double x,
                                          double y)
{
    TO_STARFISH(m_starfish)
        ->platformWindow()
        ->dispatchMouseEvent(::StarFish::MouseEventKind::MouseEventDown,
                             ::StarFish::MouseData(button, buttons, x, y, 0));
}

void WebContainer::DispatchMouseUpEvent(MouseButtonValue button,
                                        MouseButtonsValue buttons, double x,
                                        double y)
{
    TO_STARFISH(m_starfish)
        ->platformWindow()
        ->dispatchMouseEvent(::StarFish::MouseEventKind::MouseEventUp,
                             ::StarFish::MouseData(button, buttons, x, y, 0));
}

void WebContainer::DispatchMouseWheelEvent(double x, double y, int delta)
{
    TO_STARFISH(m_starfish)
        ->platformWindow()
        ->dispatchMouseWheelEvent(x, y, delta, true);
}

void WebContainer::DispatchKeyDownEvent(KeyValue keyCode, int modifier)
{
    TO_STARFISH(m_starfish)
        ->platformWindow()
        ->dispatchKeyEvent(::StarFish::KeyEventKind::KeyEventDown,
                           ::StarFish::PlatformKeyEventData(keyCode));
}

void WebContainer::DispatchKeyPressEvent(KeyValue keyCode, int modifier)
{
    TO_STARFISH(m_starfish)
        ->platformWindow()
        ->dispatchKeyEvent(::StarFish::KeyEventKind::KeyEventPress,
                           ::StarFish::PlatformKeyEventData(keyCode));
}

void WebContainer::DispatchKeyUpEvent(KeyValue keyCode, int modifier)
{
    TO_STARFISH(m_starfish)
        ->platformWindow()
        ->dispatchKeyEvent(::StarFish::KeyEventKind::KeyEventUp,
                           ::StarFish::PlatformKeyEventData(keyCode));
}
}
