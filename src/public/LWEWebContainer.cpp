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
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/message_loop/Timer.h"
#include "core/dom/MouseEvent.h"
#include "core/dom/KeyboardEvent.h"
#include "core/page/WebView.h"
#include "platform/network/HTTPCache.h"
#include "core/event/KeyBoardEventData.h"
#include "platform/event/PlatformKeyEventData.h"
#include "platform/loader/ResourceURL.h"

#include <EscargotPublic.h>

#define TO_STARFISH(ptr) (((StarFish::WebView*)ptr)->starFish())
#define TO_WEBVIEW(ptr) (((StarFish::WebView*)ptr))
#define TO_HISTORY(ptr) \
    ((StarFish::WebView*)ptr)->mainBrowsingContext()->window()->history()

#define TO_LOCATION(ptr) \
    ((StarFish::WebView*)ptr)->mainBrowsingContext()->window()->location()

#define TO_RESOURCE_LOADER(ptr) \
    ((StarFish::WebView*)ptr)   \
        ->mainBrowsingContext() \
        ->document()            \
        ->resourceLoader()

#define TO_SCRIPT_BINDING_INSTANCE(ptr) \
    ((StarFish::WebView*)ptr)           \
        ->mainBrowsingContext()         \
        ->window()                      \
        ->scriptBindingInstance()

namespace LWE {

Settings::Settings(const std::string& default_ua, const std::string& ua)
    : m_defaultUserAgent(default_ua)
    , m_userAgent(ua)
#if defined(STARFISH_ENABLE_HTTPCACHE)
    , m_cacheMode(StarFish::HTTPCache::LOAD_DEFAULT)
#else
    , m_cacheMode(0)
#endif
    , m_defaultFontSize(DEFAULT_FONT_SIZE)
{
}

std::string Settings::GetDefaultUserAgent() const
{
    return m_defaultUserAgent;
}

std::string Settings::GetUserAgentString() const
{
    return m_userAgent;
}

std::string Settings::GetProxyURL() const
{
    return m_proxyURL;
}

void Settings::SetUserAgentString(const std::string& ua)
{
    m_userAgent = ua;
}

int Settings::GetCacheMode() const
{
    return m_cacheMode;
}

void Settings::SetCacheMode(int mode)
{
    m_cacheMode = mode;
}

void Settings::SetProxyURL(const std::string& s)
{
    m_proxyURL = s;
}

ResourceError::ResourceError(int code, const std::string& description)
    : m_errorCode(code)
    , m_description(description)
{
}

int ResourceError::GetErrorCode()
{
    return m_errorCode;
}

std::string ResourceError::GetDescription()
{
    return m_description;
}

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

StarFish::StarFish* g_starFishInstance;

static StarFish::WebView* createStarfishInstance(
    uint width, uint height, float devicePixelRatio,
    const char* defaultFontName, const char* locale, const char* timezoneID,
    const char* localStorageFilePath, const char* cookieStoreFilePath,
    const char* httpCacheDirectorypath)
{
    std::string screenShot;
    std::string customUserAgentString;
    std::string builtinPolyfillPathString;

    StarFish::ScreenInfo info;
    info.rect.setWidth(width);
    info.rect.setHeight(height);
    info.availableRect.setWidth(width);
    info.availableRect.setHeight(height);
    info.devicePixelRatio = devicePixelRatio;

    if (g_starFishInstance == nullptr) {
        g_starFishInstance = new StarFish::StarFish(
            localStorageFilePath, cookieStoreFilePath, httpCacheDirectorypath);
    }

    ::StarFish::WebView* webView = ::StarFish::WebView::create(
        g_starFishInstance, locale, timezoneID, width, height,
        DEFAULT_FONT_SIZE, StarFish::String::createASCIIString(defaultFontName),
        info, StarFish::String::fromUTF8(customUserAgentString.data()),
        StarFish::String::fromUTF8(builtinPolyfillPathString.data()));
    return webView;
}

WebContainer* WebContainer::Create(void* buffer, uint width, uint height,
                                   uint stride, float scaleFactor,
                                   const char* defaultFontName,
                                   const char* locale, const char* timezoneID,
                                   const char* localStorageFilePath,
                                   const char* cookieStoreFilePath,
                                   const char* httpCacheDirectorypath)
{
#if !defined(PORT_WINDOW_BACKEND_GB)
    STARFISH_LOG_ERROR("Cannot use this set of function within this port!");
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
    return nullptr;
#endif
    auto webView = createStarfishInstance(
        width, height, scaleFactor, defaultFontName, locale, timezoneID,
        localStorageFilePath, cookieStoreFilePath, httpCacheDirectorypath);

    webView->platformWindow()->updateDrawingBufferAddress(buffer, width, height,
                                                          stride);

    WebContainer* newWebContainer =
        new (GC_MALLOC_UNCOLLECTABLE(sizeof(WebView))) WebContainer(webView);

    return newWebContainer;
}

void WebContainer::UpdateBuffer(void* buffer, uint width, uint height,
                                uint stride)
{
#if !defined(PORT_WINDOW_BACKEND_GB)
    STARFISH_LOG_ERROR("Cannot use this set of function within this port!");
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
#endif
    TO_WEBVIEW(m_impl)
        ->platformWindow()
        ->updateDrawingBufferAddress(buffer, width, height, stride);
}

void WebContainer::RegisterOnRenderedHandler(
    const std::function<void(LWE::WebContainer*,
                             const LWE::WebContainer::RenderResult&)>& cb)
{
#if !defined(PORT_WINDOW_BACKEND_GB)
    STARFISH_LOG_ERROR("Cannot use this set of function within this port!");
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
#endif
    TO_WEBVIEW(m_impl)
        ->platformWindow()
        ->registerRenderingFinishedCallback([this, cb](
            const StarFish::RenderResult& renderResult) {
            LWE::WebContainer::RenderResult result;
            result.updatedX = (int)renderResult.updateRect.x();
            result.updatedY = (int)renderResult.updateRect.y();
            result.updatedWidth = (int)renderResult.updateRect.width();
            result.updatedHeight = (int)renderResult.updateRect.height();
            result.updatedBufferAddress =
                TO_WEBVIEW(m_impl)->platformWindow()->drawingBufferAddress();
            result.bufferImageWidth =
                TO_WEBVIEW(m_impl)->platformWindow()->width();
            result.bufferImageHeight =
                TO_WEBVIEW(m_impl)->platformWindow()->height();
            cb(this, result);
        });
}

WebContainer* WebContainer::CreateGL(
    uint width, uint height,
    const std::function<void(LWE::WebContainer*)>& onGLMakeCurrent,
    const std::function<void(LWE::WebContainer*)>& onGLSwapBuffers,
    float devicePixelRatio, const char* defaultFontName, const char* locale,
    const char* timezoneID, const char* localStorageFilePath,
    const char* cookieStoreFilePath, const char* httpCacheDirectorypath)
{
#if !defined(PORT_WINDOW_BACKEND_GL)
    STARFISH_LOG_ERROR("Cannot use this set of function within this port!");
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
#endif

    auto webView = createStarfishInstance(
        width, height, devicePixelRatio, defaultFontName, locale, timezoneID,
        localStorageFilePath, cookieStoreFilePath, httpCacheDirectorypath);

    WebContainer* newWebContainer =
        new (GC_MALLOC_UNCOLLECTABLE(sizeof(WebView))) WebContainer(webView);

    webView->platformWindow()->registerGLMakeCurrentCallback(
        [onGLMakeCurrent, newWebContainer](StarFish::PlatformWindow* wnd) {
            onGLMakeCurrent(newWebContainer);
        });

    webView->platformWindow()->registerGLSwapBuffersCallback(
        [onGLSwapBuffers, newWebContainer](StarFish::PlatformWindow* wnd) {
            onGLSwapBuffers(newWebContainer);
        });

    return newWebContainer;
}

void WebContainer::ResizeTo(size_t width, size_t height)
{
#if !defined(PORT_WINDOW_BACKEND_GL)
    STARFISH_LOG_ERROR("Cannot use this set of function within this port!");
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
#endif
    TO_WEBVIEW(m_impl)->platformWindow()->resizeTo((int)width, (int)height);
}

WebContainer::WebContainer(void* impl)
    : m_impl(impl)
{
}

void WebContainer::AddIdleCallback(void (*callback)(void*), void* data)
{
    struct Data : public gc {
        void (*callback)(void*);
        void* data;
    };

    Data* d = new Data();
    d->callback = callback;
    d->data = data;
    TO_WEBVIEW(m_impl)
        ->messageLoop()
        ->addIdler(nullptr,
                   [](size_t, void* data) {
                       Data* d = (Data*)data;
                       d->callback(d->data);
                   },
                   d);
}

size_t WebContainer::AddTimeout(void (*callback)(void*), void* data,
                                size_t timeoutInMS)
{
    struct Data : public gc {
        void (*callback)(void*);
        void* data;
    };

    Data* d = new Data();
    d->callback = callback;
    d->data = data;
    return TO_WEBVIEW(m_impl)->timer()->addTimer(
        timeoutInMS, nullptr,
        [](::StarFish::Window* window, void* data) {
            Data* d = (Data*)data;
            d->callback(d->data);
        },
        d, false);
}

void WebContainer::ClearTimeout(size_t handle)
{
    return TO_WEBVIEW(m_impl)->timer()->removeTimer(handle);
}

Settings WebContainer::GetSettings()
{
    Settings result(USER_AGENT(STARFISH_NAME, VERSION),
                    TO_WEBVIEW(m_impl)->userAgent()->toUTF8NonGCString());

#ifdef STARFISH_ENABLE_HTTPCACHE
    if (TO_STARFISH(m_impl)->httpCache()) {
        result.SetCacheMode(TO_STARFISH(m_impl)->httpCache()->cacheMode());
    } else {
        result.SetCacheMode(::StarFish::HTTPCache::LOAD_NO_CACHE);
    }
#endif
    result.SetProxyURL(TO_WEBVIEW(m_impl)->proxyURL());
    return result;
}

void WebContainer::LoadURL(const std::string& url)
{
    TO_WEBVIEW(m_impl)
        ->loadHTMLDocument(StarFish::String::fromUTF8(url.data()));
}

std::string WebContainer::GetURL()
{
    return TO_LOCATION(m_impl)->url()->urlString()->toUTF8NonGCString();
}

void WebContainer::LoadData(const std::string& data)
{
    if (data.size() > 0) {
        auto dataURI = StarFish::StringUtils::toBase64HTMLDataURI(data);
        TO_WEBVIEW(m_impl)
            ->loadHTMLDocument(StarFish::String::fromUTF8(dataURI.data()));
    } else {
        TO_WEBVIEW(m_impl)
            ->loadHTMLDocument(StarFish::String::fromUTF8("about:blank"));
    }
}

void WebContainer::Reload()
{
    TO_LOCATION(m_impl)->reload();
}

void WebContainer::StopLoading()
{
    STARFISH_ASSERT(m_impl);
    TO_RESOURCE_LOADER(m_impl).clear();
}

void WebContainer::GoBack()
{
    STARFISH_ASSERT(m_impl);
    TO_HISTORY(m_impl)->back();
}

void WebContainer::GoForward()
{
    STARFISH_ASSERT(m_impl);
    TO_HISTORY(m_impl)->forward();
}

bool WebContainer::CanGoBack()
{
    STARFISH_ASSERT(m_impl);
    return TO_HISTORY(m_impl)->canGoBack();
}

bool WebContainer::CanGoForward()
{
    STARFISH_ASSERT(m_impl);
    return TO_HISTORY(m_impl)->canGoForward();
}

void WebContainer::AddJavaScriptInterface(
    const std::string& exposedObjectName, const std::string& jsFunctionName,
    std::function<std::string(const std::string&)> cb)
{
    STARFISH_ASSERT(m_impl);

    StarFish::String* objectName =
        StarFish::String::fromUTF8(exposedObjectName.c_str());
    StarFish::String* functionName =
        StarFish::String::fromUTF8(jsFunctionName.c_str());

    StarFish::registerJavaScriptNativeInterface(
        TO_SCRIPT_BINDING_INSTANCE(m_impl), objectName, functionName,
        new StarFish::JavaScriptNativeHandler(TO_WEBVIEW(m_impl), functionName,
                                              cb),
        nativeCallbackFunction);
}

std::string WebContainer::EvaluateJavaScript(const std::string& script)
{
    STARFISH_ASSERT(m_impl);
    return TO_WEBVIEW(m_impl)
        ->evaluateJavaScript(StarFish::String::fromUTF8(script.c_str()))
        ->toUTF8NonGCString();
}

void WebContainer::ClearHistory()
{
    STARFISH_ASSERT(m_impl);
    TO_WEBVIEW(m_impl)->historyManager()->clear();
}

void WebContainer::Destroy()
{
    STARFISH_ASSERT(m_impl);
    TO_WEBVIEW(m_impl)->destroy();
    m_impl = nullptr;

    GC_FREE(this);

    if (g_starFishInstance->webViewInstanceCount() == 0) {
        g_starFishInstance->destroy();
        g_starFishInstance = nullptr;
    }

    // do implicit calling GC funciton takes a lots time
    // we should remove this if possible
    GC_gcollect();
    GC_gcollect_and_unmap();
}

void WebContainer::Resume()
{
    STARFISH_ASSERT(m_impl);
    TO_WEBVIEW(m_impl)->platformWindow()->resume();
}

void WebContainer::Pause()
{
    STARFISH_ASSERT(m_impl);
    TO_WEBVIEW(m_impl)->platformWindow()->pause();
}

void WebContainer::SetSettings(const Settings& settings)
{
    STARFISH_ASSERT(m_impl);
    TO_WEBVIEW(m_impl)
        ->setCustomUserAgentString(
            StarFish::String::fromUTF8(settings.GetUserAgentString().c_str()));
    TO_WEBVIEW(m_impl)->setProxyURL(settings.GetProxyURL());
#ifdef STARFISH_ENABLE_HTTPCACHE
    if (TO_STARFISH(m_impl)->httpCache()) {
        TO_STARFISH(m_impl)->httpCache()->setCacheMode(settings.GetCacheMode());
    } else {
        STARFISH_LOG_ERROR(
            "Http Cache could not initialized. So Changing cache mode is no "
            "effect.. ");
    }
#endif
}

void WebContainer::RemoveJavascriptInterface(
    const std::string& exposedObjectName, const std::string& jsFunctionName)
{
    STARFISH_ASSERT(m_impl);
    StarFish::String* objectName =
        StarFish::String::fromUTF8(exposedObjectName.c_str());
    if (!jsFunctionName.empty()) {
        StarFish::String* functionName =
            StarFish::String::fromUTF8(jsFunctionName.c_str());
        StarFish::unregisterJavaScriptNativeInterface(
            TO_SCRIPT_BINDING_INSTANCE(m_impl), objectName, functionName);
    } else {
        StarFish::unregisterJavaScriptNativeInterface(
            TO_SCRIPT_BINDING_INSTANCE(m_impl), objectName);
    }
}
void WebContainer::ClearCache()
{
    STARFISH_ASSERT(m_impl);
#ifdef STARFISH_ENABLE_HTTPCACHE
    if (TO_STARFISH(m_impl)->httpCache() != nullptr) {
        TO_STARFISH(m_impl)->httpCache()->clear();
    }
#endif
}

void WebContainer::RegisterOnReceivedErrorHandler(
    const std::function<void(LWE::WebContainer*, LWE::ResourceError)>& cb)
{
    TO_WEBVIEW(m_impl)
        ->registerPublicWebViewHandler(
            std::string("OnReceivedError"),
            [this, cb](StarFish::String* url, int errorCode) -> void {
                // make error description
                cb(this, ResourceError(errorCode, std::string()));
            });
}

void WebContainer::RegisterOnPageParsedHandler(
    std::function<void(LWE::WebContainer*, const std::string&)> cb)
{
    TO_WEBVIEW(m_impl)
        ->registerPublicWebViewHandler(
            std::string("OnPageParsed"),
            [this, cb](StarFish::String* url, int errorCode) -> void {
                cb(this, url->toUTF8NonGCString());
            });
}

void WebContainer::RegisterOnPageLoadedHandler(
    std::function<void(LWE::WebContainer*, const std::string&)> cb)
{
    TO_WEBVIEW(m_impl)
        ->registerPublicWebViewHandler(
            std::string("OnPageLoaded"),
            [this, cb](StarFish::String* url, int errorCode) -> void {
                cb(this, url->toUTF8NonGCString());
            });
}

void WebContainer::RegisterOnPageStartedHandler(
    const std::function<void(LWE::WebContainer*, const std::string&)>& cb)
{
    TO_WEBVIEW(m_impl)
        ->registerPublicWebViewHandler(
            std::string("OnPageStarted"),
            [this, cb](StarFish::String* url, int errorCode) -> void {
                cb(this, url->toUTF8NonGCString());
            });
}

void WebContainer::RegisterOnLoadResourceHandler(
    const std::function<void(LWE::WebContainer*, const std::string&)>& cb)
{
    TO_WEBVIEW(m_impl)
        ->registerPublicWebViewHandler(
            std::string("OnLoadResource"),
            [this, cb](StarFish::String* url, int errorCode) -> void {
                cb(this, url->toUTF8NonGCString());
            });
}

void WebContainer::RegisterShouldOverrideUrlLoadingHandler(
    const std::function<bool(LWE::WebContainer*, const std::string&)>& cb)
{
    TO_WEBVIEW(m_impl)
        ->registerPublicWebViewHandler(
            std::string("shouldOverrideUrlLoading"),
            [this, cb](void* param) -> void {
                struct Param : public gc {
                    StarFish::ResourceURL* url;
                    StarFish::ResourceURL* referrerUrl;
                    bool canNavigate;
                    bool force;

                    static void* operator new(size_t s)
                    {
                        return GC_MALLOC_UNCOLLECTABLE(s);
                    }
                };

                Param* p = (Param*)param;
                bool ret =
                    cb(this, p->url->urlString()->toUTF8NonGCString().data());

                if ((ret == false) && p->canNavigate) {
                    // continue loading
                    TO_WEBVIEW(m_impl)
                        ->messageLoop()
                        ->invokeNavigate(TO_WEBVIEW(m_impl), p->url,
                                         p->referrerUrl, true);
                }
                delete p;
            });
}

void WebContainer::RegisterOnDownloadStartHandler(
    const std::function<void(LWE::WebContainer*, const std::string&,
                             const std::string&, const std::string&,
                             const std::string&, long)>& cb)
{
    TO_WEBVIEW(m_impl)
        ->registerPublicWebViewHandler(
            std::string("onDownloadStart"), [this, cb](void* param) -> void {
                struct Param {
                    std::string url;
                    std::string userAgent;
                    std::string contentDisposition;
                    std::string mimetype;
                    long contentLength;
                };
                STARFISH_LOG_ERROR(
                    "Http Cache could not initialized. So Changing cache mode "
                    "is no effect.. ");
                Param* p = (Param*)param;
                cb(this, p->url, p->userAgent, p->contentDisposition,
                   p->mimetype, p->contentLength);
                delete p;
            });
}

void WebContainer::RegisterShowDropdownMenuHandler(
    const std::function<void(LWE::WebContainer*,
                             const std::vector<std::string>*, int)>& cb)
{
    TO_WEBVIEW(m_impl)
        ->platformWindow()
        ->registerCallbackHandler(std::string("showDropdownMenu"),
                                  [this, cb](void* param) -> void {
                                      struct Param {
                                          std::vector<std::string>* list;
                                          int checkedPosition;
                                      };

                                      Param* p = (Param*)param;
                                      cb(this, p->list, p->checkedPosition);
                                      delete p->list;
                                      delete p;
                                  });
}

void WebContainer::RegisterShowAlertHandler(
    const std::function<void(LWE::WebContainer*, const std::string&,
                             const std::string&)>& cb)
{
    TO_WEBVIEW(m_impl)
        ->platformWindow()
        ->registerCallbackHandler(std::string("showAlert"),
                                  [this, cb](void* param) -> void {
                                      struct Param {
                                          std::string title;
                                          std::string message;
                                      };

                                      Param* p = (Param*)param;
                                      cb(this, p->title, p->message);
                                      delete p;
                                  });
}

void WebContainer::RegisterCustomFileResourceRequestHandlers(
    std::function<const char*(const char* path)> resolveFilePathCallback,
    std::function<void*(const char* path)> fileOpenCallback,
    std::function<size_t(uint8_t* destBuffer, size_t size, void* handle)>
        fileReadCallback,
    std::function<long int(void* handle)> fileLengthCallback,
    std::function<void(void* handle)> fileCloseCallback)
{
    TO_WEBVIEW(m_impl)
        ->registerCustomFileResourceRequestCallbacks(
            resolveFilePathCallback, fileOpenCallback, fileReadCallback,
            fileLengthCallback, fileCloseCallback);
}

void WebContainer::CallHandler(const std::string& handler, void* param)
{
    TO_WEBVIEW(m_impl)->platformWindow()->callHandler(handler, param);
}

size_t WebContainer::Width()
{
    return TO_WEBVIEW(m_impl)->platformWindow()->width();
}

size_t WebContainer::Height()
{
    return TO_WEBVIEW(m_impl)->platformWindow()->height();
}

void WebContainer::RegisterOnProgressChangedHandler(
    const std::function<void(LWE::WebContainer*, int)>& cb)
{
    TO_WEBVIEW(m_impl)
        ->registerPublicWebViewHandler(
            std::string("OnProgressChanged"),
            [this, cb](StarFish::String* url, int newProgress) -> void {
                cb(this, newProgress);
            });
}

void WebContainer::SetUserAgentString(const std::string& userAgent)
{
    TO_WEBVIEW(m_impl)
        ->setCustomUserAgentString(
            StarFish::String::fromUTF8(userAgent.c_str()));
}

void WebContainer::SetCacheMode(int mode)
{
#ifdef STARFISH_ENABLE_HTTPCACHE
    if (TO_STARFISH(m_impl)->httpCache() != nullptr) {
        TO_STARFISH(m_impl)->httpCache()->setCacheMode(mode);
    }
#endif
}

void WebContainer::SetDefaultFontSize(uint32_t size)
{
    STARFISH_RELEASE_ASSERT(1 <= size && size <= 72);
    TO_WEBVIEW(m_impl)->setDefaultFontSize(size);
}

void WebContainer::DispatchMouseMoveEvent(MouseButtonValue button,
                                          MouseButtonsValue buttons, double x,
                                          double y)
{
    TO_WEBVIEW(m_impl)
        ->platformWindow()
        ->dispatchMouseEvent(::StarFish::MouseEventKind::MouseEventMove,
                             ::StarFish::MouseData(button, buttons, x, y, 0));
}

void WebContainer::DispatchMouseDownEvent(MouseButtonValue button,
                                          MouseButtonsValue buttons, double x,
                                          double y)
{
    TO_WEBVIEW(m_impl)
        ->platformWindow()
        ->dispatchMouseEvent(::StarFish::MouseEventKind::MouseEventDown,
                             ::StarFish::MouseData(button, buttons, x, y, 0));
}

void WebContainer::DispatchMouseUpEvent(MouseButtonValue button,
                                        MouseButtonsValue buttons, double x,
                                        double y)
{
    TO_WEBVIEW(m_impl)
        ->platformWindow()
        ->dispatchMouseEvent(::StarFish::MouseEventKind::MouseEventUp,
                             ::StarFish::MouseData(button, buttons, x, y, 0));
}

void WebContainer::DispatchMouseWheelEvent(double x, double y, int delta)
{
    TO_WEBVIEW(m_impl)
        ->platformWindow()
        ->dispatchMouseWheelEvent(x, y, delta, true);
}

void WebContainer::DispatchKeyDownEvent(KeyValue keyCode)
{
    TO_WEBVIEW(m_impl)
        ->platformWindow()
        ->dispatchKeyEvent(::StarFish::KeyEventKind::KeyEventDown,
                           ::StarFish::PlatformKeyEventData(keyCode));
}

void WebContainer::DispatchKeyPressEvent(KeyValue keyCode)
{
    TO_WEBVIEW(m_impl)
        ->platformWindow()
        ->dispatchKeyEvent(::StarFish::KeyEventKind::KeyEventPress,
                           ::StarFish::PlatformKeyEventData(keyCode));
}

void WebContainer::DispatchKeyUpEvent(KeyValue keyCode)
{
    TO_WEBVIEW(m_impl)
        ->platformWindow()
        ->dispatchKeyEvent(::StarFish::KeyEventKind::KeyEventUp,
                           ::StarFish::PlatformKeyEventData(keyCode));
}

void WebContainer::DispatchCompositionStartEvent(
    const std::string& soFarCompositiedString)
{
    TO_WEBVIEW(m_impl)
        ->platformWindow()
        ->dispatchCompositionEvent(
            ::StarFish::CompositionEventKind::CompositionEventStart,
            ::StarFish::String::fromUTF8(soFarCompositiedString.data(),
                                         soFarCompositiedString.length()));
}

void WebContainer::DispatchCompositionUpdateEvent(
    const std::string& soFarCompositiedString)
{
    TO_WEBVIEW(m_impl)
        ->platformWindow()
        ->dispatchCompositionEvent(
            ::StarFish::CompositionEventKind::CompositionEventUpdate,
            ::StarFish::String::fromUTF8(soFarCompositiedString.data(),
                                         soFarCompositiedString.length()));
}

void WebContainer::DispatchCompositionEndEvent(
    const std::string& soFarCompositiedString)
{
    TO_WEBVIEW(m_impl)
        ->platformWindow()
        ->dispatchCompositionEvent(
            ::StarFish::CompositionEventKind::CompositionEventEnd,
            ::StarFish::String::fromUTF8(soFarCompositiedString.data(),
                                         soFarCompositiedString.length()));
}
void WebContainer::RegisterOnShowSoftwareKeyboardIfPossibleHandler(
    const std::function<void(LWE::WebContainer*)>& cb)
{
    TO_WEBVIEW(m_impl)
        ->platformWindow()
        ->registerShowSoftwareKeyboardIfPossibleCallback(
            [this, cb]() { cb(this); });
}

void WebContainer::RegisterOnHideSoftwareKeyboardIfPossibleHandler(
    const std::function<void(LWE::WebContainer*)>& cb)
{
    TO_WEBVIEW(m_impl)
        ->platformWindow()
        ->registerHideSoftwareKeyboardIfPossibleCallback(
            [this, cb]() { cb(this); });
}

void WebContainer::RegisterSetNeedsRenderingCallback(
    const std::function<void(LWE::WebContainer*, const std::function<void()>&
                                                     doRenderingFunction)>& cb)
{
    TO_WEBVIEW(m_impl)
        ->platformWindow()
        ->registerSetNeedsRenderingCallback(
            [this, cb](StarFish::PlatformWindow* wnd) {
                std::function<void()> fn = [wnd]() { wnd->rendering(); };
                cb(this, fn);
            });
}

void WebContainer::SetUserData(const std::string& key, void* data)
{
    TO_WEBVIEW(m_impl)->publicLayerUserDataMap()[key] = data;
}

void* WebContainer::GetUserData(const std::string& key)
{
    return TO_WEBVIEW(m_impl)->publicLayerUserDataMap()[key];
}

} // namespace LWE
