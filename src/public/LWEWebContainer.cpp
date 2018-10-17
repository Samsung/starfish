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

#ifdef PORT_NEEDS_THREADED_PUBLIC_API
#define START_SIMPLE_THREADED_PUBLIC_API_WRAPPER StarFish::MessageLoop::runOnMainThreadSync([&]() -> size_t {
#define END_SIMPLE_THREADED_PUBLIC_API_WRAPPER \
    return 0;                                  \
    });
#else
#define START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
#define END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
#endif

namespace LWE {

extern StarFish::StarFish* g_starFishInstance;

Settings::Settings(const std::string& default_ua, const std::string& ua)
    : m_defaultUserAgent(default_ua)
    , m_userAgent(ua)
#if defined(STARFISH_ENABLE_HTTPCACHE)
    , m_cacheMode(StarFish::HTTPCache::LOAD_DEFAULT)
#else
    , m_cacheMode(0)
#endif
    , m_defaultFontSize(LWE_DEFAULT_FONT_SIZE)
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

static StarFish::WebView* createWebViewInstance(unsigned width, unsigned height,
                                                float devicePixelRatio,
                                                const char* defaultFontName,
                                                const char* locale,
                                                const char* timezoneID)
{
    if (!LWE::IsInitialized()) {
        STARFISH_LOG_ERROR(
            "You must call LWE::Initialize function before using WebContainer "
            "or WebView");
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    std::string customUserAgentString;
    std::string builtinPolyfillPathString;

    StarFish::ScreenInfo info;
    info.rect.setWidth(width);
    info.rect.setHeight(height);
    info.availableRect.setWidth(width);
    info.availableRect.setHeight(height);
    info.devicePixelRatio = devicePixelRatio;

    ::StarFish::WebView* webView = ::StarFish::WebView::create(
        g_starFishInstance, locale, timezoneID, width, height,
        LWE_DEFAULT_FONT_SIZE,
        StarFish::String::createASCIIString(defaultFontName), info,
        StarFish::String::fromUTF8(customUserAgentString.data()),
        StarFish::String::fromUTF8(builtinPolyfillPathString.data()));
    return webView;
}

WebContainer* WebContainer::Create(void* buffer, unsigned width,
                                   unsigned height, unsigned stride,
                                   float scaleFactor,
                                   const char* defaultFontName,
                                   const char* locale, const char* timezoneID)
{
#if !defined(PORT_WINDOW_BACKEND_GB)
    STARFISH_LOG_ERROR("Cannot use this set of function within this port!");
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
    return nullptr;
#endif

#if defined(PORT_NEEDS_THREADED_PUBLIC_API)
    return (WebContainer*)StarFish::MessageLoop::runOnMainThreadSync(
        [&]() -> size_t {
            auto webView =
                createWebViewInstance(width, height, scaleFactor,
                                      defaultFontName, locale, timezoneID);

            webView->platformWindow()->updateDrawingBufferAddress(
                buffer, width, height, stride);

            WebContainer* newWebContainer =
                new (GC_MALLOC_UNCOLLECTABLE(sizeof(WebView)))
                    WebContainer(webView);

            return (size_t)newWebContainer;
        });
#else
    auto webView = createWebViewInstance(width, height, scaleFactor,
                                         defaultFontName, locale, timezoneID);

    webView->platformWindow()->updateDrawingBufferAddress(buffer, width, height,
                                                          stride);

    WebContainer* newWebContainer =
        new (GC_MALLOC_UNCOLLECTABLE(sizeof(WebView))) WebContainer(webView);

    return newWebContainer;
#endif
}

void WebContainer::UpdateBuffer(void* buffer, unsigned width, unsigned height,
                                unsigned stride)
{
#if !defined(PORT_WINDOW_BACKEND_GB)
    STARFISH_LOG_ERROR("Cannot use this set of function within this port!");
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
#endif

    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->platformWindow()
        ->updateDrawingBufferAddress(buffer, width, height, stride);
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::RegisterOnRenderedHandler(
    const std::function<void(WebContainer*, const WebContainer::RenderResult&)>&
        cb)
{
#if !defined(PORT_WINDOW_BACKEND_GB)
    STARFISH_LOG_ERROR("Cannot use this set of function within this port!");
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
#endif
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->platformWindow()
        ->registerRenderingFinishedCallback([this, cb](
            const StarFish::RenderResult& renderResult) {
            WebContainer::RenderResult result;
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
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

WebContainer* WebContainer::CreateGL(
    unsigned width, unsigned height,
    const std::function<void(WebContainer*)>& onGLMakeCurrent,
    const std::function<void(WebContainer*)>& onGLSwapBuffers,
    float devicePixelRatio, const char* defaultFontName, const char* locale,
    const char* timezoneID)
{
#if !defined(PORT_WINDOW_BACKEND_GL)
    STARFISH_LOG_ERROR("Cannot use this set of function within this port!");
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
#endif

#if defined(PORT_NEEDS_THREADED_PUBLIC_API)
    return (WebContainer*)StarFish::MessageLoop::runOnMainThreadSync(
        [=]() -> size_t {
            auto webView =
                createWebViewInstance(width, height, devicePixelRatio,
                                      defaultFontName, locale, timezoneID);

            WebContainer* newWebContainer =
                new (GC_MALLOC_UNCOLLECTABLE(sizeof(WebView)))
                    WebContainer(webView);

            webView->platformWindow()->registerGLMakeCurrentCallback(
                [onGLMakeCurrent,
                 newWebContainer](StarFish::PlatformWindow* wnd) {
                    onGLMakeCurrent(newWebContainer);
                });

            webView->platformWindow()->registerGLSwapBuffersCallback(
                [onGLSwapBuffers,
                 newWebContainer](StarFish::PlatformWindow* wnd) {
                    onGLSwapBuffers(newWebContainer);
                });

            return (size_t)newWebContainer;
        });
#else
    auto webView = createWebViewInstance(width, height, devicePixelRatio,
                                         defaultFontName, locale, timezoneID);

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
#endif
}

void WebContainer::ResizeTo(size_t width, size_t height)
{
#if !defined(PORT_WINDOW_BACKEND_GL)
    STARFISH_LOG_ERROR("Cannot use this set of function within this port!");
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
#endif
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)->platformWindow()->resizeTo((int)width, (int)height);
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

WebContainer::WebContainer(void* impl)
    : m_impl(impl)
{
}

void WebContainer::AddIdleCallback(void (*callback)(void*), void* data)
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
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
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

size_t WebContainer::AddTimeout(void (*callback)(void*), void* data,
                                size_t timeoutInMS)
{
    size_t ret;
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    struct Data : public gc {
        void (*callback)(void*);
        void* data;
    };

    Data* d = new Data();
    d->callback = callback;
    d->data = data;
    ret = TO_WEBVIEW(m_impl)->timer()->addTimer(
        timeoutInMS, nullptr,
        [](::StarFish::Window* window, void* data) {
            Data* d = (Data*)data;
            d->callback(d->data);
        },
        d, false);
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    return ret;
}

void WebContainer::ClearTimeout(size_t handle)
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)->timer()->removeTimer(handle);
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

Settings WebContainer::GetSettings()
{
    Settings result(USER_AGENT(STARFISH_NAME, VERSION), "");

    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    result.SetUserAgentString(
        TO_WEBVIEW(m_impl)->userAgent()->toUTF8NonGCString());
#ifdef STARFISH_ENABLE_HTTPCACHE
    if (TO_STARFISH(m_impl)->httpCache()) {
        result.SetCacheMode(TO_STARFISH(m_impl)->httpCache()->cacheMode());
    } else {
        result.SetCacheMode(::StarFish::HTTPCache::LOAD_NO_CACHE);
    }
#endif
    result.SetProxyURL(TO_WEBVIEW(m_impl)->proxyURL());
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER

    return result;
}

void WebContainer::LoadURL(const std::string& url)
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER

    TO_WEBVIEW(m_impl)
        ->loadHTMLDocument(StarFish::String::fromUTF8(url.data()));

    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

std::string WebContainer::GetURL()
{
    std::string ret;
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    ret = TO_LOCATION(m_impl)->url()->urlString()->toUTF8NonGCString();
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    return ret;
}

void WebContainer::LoadData(const std::string& data)
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    if (data.size() > 0) {
        auto dataURI = StarFish::StringUtils::toBase64HTMLDataURI(data);
        TO_WEBVIEW(m_impl)
            ->loadHTMLDocument(StarFish::String::fromUTF8(dataURI.data()));
    } else {
        TO_WEBVIEW(m_impl)
            ->loadHTMLDocument(StarFish::String::fromUTF8("about:blank"));
    }
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::Reload()
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    TO_LOCATION(m_impl)->reload();
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::StopLoading()
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    STARFISH_ASSERT(m_impl);
    TO_RESOURCE_LOADER(m_impl).clear();
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::GoBack()
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    TO_HISTORY(m_impl)->back();
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::GoForward()
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    TO_HISTORY(m_impl)->forward();
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

bool WebContainer::CanGoBack()
{
    STARFISH_ASSERT(m_impl);
    return TO_HISTORY(m_impl)->canGoBack();
}

bool WebContainer::CanGoForward()
{
    bool ret;
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    ret = TO_HISTORY(m_impl)->canGoForward();
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    return ret;
}

void WebContainer::AddJavaScriptInterface(
    const std::string& exposedObjectName, const std::string& jsFunctionName,
    std::function<std::string(const std::string&)> cb)
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER

    StarFish::String* objectName =
        StarFish::String::fromUTF8(exposedObjectName.c_str());
    StarFish::String* functionName =
        StarFish::String::fromUTF8(jsFunctionName.c_str());

    StarFish::registerJavaScriptNativeInterface(
        TO_SCRIPT_BINDING_INSTANCE(m_impl), objectName, functionName,
        new StarFish::JavaScriptNativeHandler(TO_WEBVIEW(m_impl), functionName,
                                              cb),
        nativeCallbackFunction);

    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

std::string WebContainer::EvaluateJavaScript(const std::string& script)
{
    std::string ret;

    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    ret = TO_WEBVIEW(m_impl)
              ->evaluateJavaScript(StarFish::String::fromUTF8(script.c_str()))
              ->toUTF8NonGCString();
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER

    return ret;
}

void WebContainer::ClearHistory()
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)->historyManager()->clear();
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::Destroy()
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)->destroy();
    m_impl = nullptr;

    GC_FREE(this);
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::Resume()
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)->platformWindow()->resume();
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::Pause()
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)->platformWindow()->pause();
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::Focus()
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    // TODO
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::Blur()
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)->platformWindow()->webView()->blur();
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::SetSettings(const Settings& settings)
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
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
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::RemoveJavascriptInterface(
    const std::string& exposedObjectName, const std::string& jsFunctionName)
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
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
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}
void WebContainer::ClearCache()
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
#ifdef STARFISH_ENABLE_HTTPCACHE
    if (TO_STARFISH(m_impl)->httpCache() != nullptr) {
        TO_STARFISH(m_impl)->httpCache()->clear();
    }
#endif
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::RegisterOnReceivedErrorHandler(
    const std::function<void(WebContainer*, ResourceError)>& cb)
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->registerPublicWebViewHandler(
            StarFish::OnReceivedError, [this, cb](void* param) -> void {
                struct Param {
                    int errorCode;
                };
                Param* p = (Param*)param;
                // make error description
                cb(this, ResourceError(p->errorCode, std::string()));
            });
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::RegisterOnPageParsedHandler(
    std::function<void(WebContainer*, const std::string&)> cb)
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->registerPublicWebViewHandler(
            StarFish::OnPageParsed, [this, cb](void* param) -> void {
                struct Param {
                    StarFish::String* url;
                };
                Param* p = (Param*)param;
                cb(this, p->url->toUTF8NonGCString());
            });
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::RegisterOnPageLoadedHandler(
    std::function<void(WebContainer*, const std::string&)> cb)
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->registerPublicWebViewHandler(
            StarFish::OnPageLoaded, [this, cb](void* param) -> void {
                struct Param {
                    StarFish::String* url;
                };
                Param* p = (Param*)param;
                cb(this, p->url->toUTF8NonGCString());
            });
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::RegisterOnPageStartedHandler(
    const std::function<void(WebContainer*, const std::string&)>& cb)
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->registerPublicWebViewHandler(
            StarFish::OnPageStarted, [this, cb](void* param) -> void {
                struct Param {
                    StarFish::String* url;
                };
                Param* p = (Param*)param;
                cb(this, p->url->toUTF8NonGCString());
            });
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::RegisterOnLoadResourceHandler(
    const std::function<void(WebContainer*, const std::string&)>& cb)
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->registerPublicWebViewHandler(
            StarFish::OnLoadResource, [this, cb](void* param) -> void {
                struct Param {
                    StarFish::String* url;
                };
                Param* p = (Param*)param;
                cb(this, p->url->toUTF8NonGCString());
            });
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::RegisterShouldOverrideUrlLoadingHandler(
    const std::function<bool(WebContainer*, const std::string&)>& cb)
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->registerPublicWebViewHandler(
            StarFish::ShouldOverrideUrlLoading,
            [this, cb](void* param) -> void {
                struct Param {
                    StarFish::ResourceURL* url;
                    StarFish::ResourceURL* referrerUrl;
                    bool canNavigate;
                    bool force;
                };
                Param* p = (Param*)param;
                bool ret =
                    cb(this, p->url->urlString()->toUTF8NonGCString().data());
                if ((ret == false) && p->canNavigate) {
                    // continue loading
                    TO_WEBVIEW(m_impl)
                        ->messageLoop()
                        ->invokeNavigate(
                            TO_WEBVIEW(m_impl), p->url, p->referrerUrl,
                            StarFish::HistoryManagerAction::Add, true);
                }
            });
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::RegisterOnDownloadStartHandler(
    const std::function<void(WebContainer*, const std::string&,
                             const std::string&, const std::string&,
                             const std::string&, long)>& cb)
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->registerPublicWebViewHandler(
            StarFish::OnDownloadStart, [this, cb](void* param) -> void {
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
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::RegisterShowDropdownMenuHandler(
    const std::function<void(WebContainer*, const std::vector<std::string>*,
                             int)>& cb)
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->platformWindow()
        ->registerCallbackHandler(StarFish::WindowHandlerShowDropdownMenu,
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
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::RegisterShowAlertHandler(
    const std::function<void(WebContainer*, const std::string&,
                             const std::string&)>& cb)
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->platformWindow()
        ->registerCallbackHandler(StarFish::WindowHandlerShowAlert,
                                  [this, cb](void* param) -> void {
                                      struct Param {
                                          std::string title;
                                          std::string message;
                                      };

                                      Param* p = (Param*)param;
                                      cb(this, p->title, p->message);
                                      delete p;
                                  });
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::RegisterCustomFileResourceRequestHandlers(
    std::function<const char*(const char* path)> resolveFilePathCallback,
    std::function<void*(const char* path)> fileOpenCallback,
    std::function<size_t(uint8_t* destBuffer, size_t size, void* handle)>
        fileReadCallback,
    std::function<long int(void* handle)> fileLengthCallback,
    std::function<void(void* handle)> fileCloseCallback)
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->registerCustomFileResourceRequestCallbacks(
            resolveFilePathCallback, fileOpenCallback, fileReadCallback,
            fileLengthCallback, fileCloseCallback);
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::CallHandler(const std::string& handler, void* param)
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    if (handler.compare("onDropdownMenuItemSelected") == 0) {
        TO_WEBVIEW(m_impl)
            ->platformWindow()
            ->callHandler(StarFish::WindowHandlerOnDropdownMenuItemSelected,
                          param);
    }
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

size_t WebContainer::Width()
{
    size_t ret;
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    ret = TO_WEBVIEW(m_impl)->platformWindow()->width();
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    return ret;
}

size_t WebContainer::Height()
{
    size_t ret;
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    ret = TO_WEBVIEW(m_impl)->platformWindow()->height();
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    return ret;
}

void WebContainer::RegisterOnProgressChangedHandler(
    const std::function<void(WebContainer*, int)>& cb)
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->registerPublicWebViewHandler(StarFish::OnProgressChanged,
                                       [this, cb](void* param) -> void {
                                           struct Param {
                                               int newProgress;
                                           };
                                           Param* p = (Param*)param;
                                           cb(this, p->newProgress);
                                       });
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::SetUserAgentString(const std::string& userAgent)
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->setCustomUserAgentString(
            StarFish::String::fromUTF8(userAgent.c_str()));
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::SetCacheMode(int mode)
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
#ifdef STARFISH_ENABLE_HTTPCACHE
    if (TO_STARFISH(m_impl)->httpCache() != nullptr) {
        TO_STARFISH(m_impl)->httpCache()->setCacheMode(mode);
    }
#endif
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::SetDefaultFontSize(uint32_t size)
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    STARFISH_RELEASE_ASSERT(1 <= size && size <= 72);
    TO_WEBVIEW(m_impl)->setDefaultFontSize(size);
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::DispatchMouseMoveEvent(MouseButtonValue button,
                                          MouseButtonsValue buttons, double x,
                                          double y)
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->platformWindow()
        ->dispatchMouseEvent(::StarFish::MouseEventKind::MouseEventMove,
                             ::StarFish::MouseData(button, buttons, x, y, 0));
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::DispatchMouseDownEvent(MouseButtonValue button,
                                          MouseButtonsValue buttons, double x,
                                          double y)
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->platformWindow()
        ->dispatchMouseEvent(::StarFish::MouseEventKind::MouseEventDown,
                             ::StarFish::MouseData(button, buttons, x, y, 0));
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::DispatchMouseUpEvent(MouseButtonValue button,
                                        MouseButtonsValue buttons, double x,
                                        double y)
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->platformWindow()
        ->dispatchMouseEvent(::StarFish::MouseEventKind::MouseEventUp,
                             ::StarFish::MouseData(button, buttons, x, y, 0));
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::DispatchMouseWheelEvent(double x, double y, int delta)
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->platformWindow()
        ->dispatchMouseWheelEvent(x, y, delta, true);
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::DispatchKeyDownEvent(KeyValue keyCode)
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->platformWindow()
        ->dispatchKeyEvent(::StarFish::KeyEventKind::KeyEventDown,
                           ::StarFish::PlatformKeyEventData(keyCode));
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::DispatchKeyPressEvent(KeyValue keyCode)
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->platformWindow()
        ->dispatchKeyEvent(::StarFish::KeyEventKind::KeyEventPress,
                           ::StarFish::PlatformKeyEventData(keyCode));
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::DispatchKeyUpEvent(KeyValue keyCode)
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->platformWindow()
        ->dispatchKeyEvent(::StarFish::KeyEventKind::KeyEventUp,
                           ::StarFish::PlatformKeyEventData(keyCode));
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::DispatchCompositionStartEvent(
    const std::string& soFarCompositiedString)
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->platformWindow()
        ->dispatchCompositionEvent(
            ::StarFish::CompositionEventKind::CompositionEventStart,
            ::StarFish::String::fromUTF8(soFarCompositiedString.data(),
                                         soFarCompositiedString.length()));
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::DispatchCompositionUpdateEvent(
    const std::string& soFarCompositiedString)
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->platformWindow()
        ->dispatchCompositionEvent(
            ::StarFish::CompositionEventKind::CompositionEventUpdate,
            ::StarFish::String::fromUTF8(soFarCompositiedString.data(),
                                         soFarCompositiedString.length()));
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::DispatchCompositionEndEvent(
    const std::string& soFarCompositiedString)
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->platformWindow()
        ->dispatchCompositionEvent(
            ::StarFish::CompositionEventKind::CompositionEventEnd,
            ::StarFish::String::fromUTF8(soFarCompositiedString.data(),
                                         soFarCompositiedString.length()));
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}
void WebContainer::RegisterOnShowSoftwareKeyboardIfPossibleHandler(
    const std::function<void(WebContainer*)>& cb)
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->platformWindow()
        ->registerShowSoftwareKeyboardIfPossibleCallback(
            [this, cb]() { cb(this); });
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::RegisterOnHideSoftwareKeyboardIfPossibleHandler(
    const std::function<void(WebContainer*)>& cb)
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->platformWindow()
        ->registerHideSoftwareKeyboardIfPossibleCallback(
            [this, cb]() { cb(this); });
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::RegisterSetNeedsRenderingCallback(
    const std::function<void(
        WebContainer*, const std::function<void()>& doRenderingFunction)>& cb)
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->platformWindow()
        ->registerSetNeedsRenderingCallback(
            [this, cb](StarFish::PlatformWindow* wnd) {
                std::function<void()> fn = [wnd]() {
                    auto p = wnd;
                    p->rendering();
                };
                cb(this, fn);
            });
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::SetUserData(const std::string& key, void* data)
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)->publicLayerUserDataMap()[key] = data;
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

void* WebContainer::GetUserData(const std::string& key)
{
    void* ret;

    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    ret = TO_WEBVIEW(m_impl)->publicLayerUserDataMap()[key];
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER

    return ret;
}

} // namespace LWE
