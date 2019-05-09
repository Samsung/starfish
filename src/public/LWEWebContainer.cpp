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

#include "StarfishConfig.h"
#include "Starfish.h"

#include "LWEWebView.h"

#include "platform/window/PlatformWindow.h"
#include "browser/history/HistoryManager.h"
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
#include "core/modules/tts/TTS.h"
#include "core/dom/MouseEvent.h"
#include "core/dom/KeyboardEvent.h"
#include "core/page/WebView.h"
#include "platform/network/http/HTTPCache.h"
#include "platform/event/PlatformKeyEventData.h"
#include "platform/loader/ResourceURL.h"
#include "platform/loader/ResourceLoader.h"

#include <EscargotPublic.h>

#define TO_STARFISH(ptr) (((Starfish::WebView*)ptr)->starfish())
#define TO_WEBVIEW(ptr) (((Starfish::WebView*)ptr))
#define TO_HISTORY(ptr) \
    ((Starfish::WebView*)ptr)->mainBrowsingContext()->window()->history()

#define TO_LOCATION(ptr) \
    ((Starfish::WebView*)ptr)->mainBrowsingContext()->window()->location()

#define TO_RESOURCE_LOADER(ptr) \
    ((Starfish::WebView*)ptr)   \
        ->mainBrowsingContext() \
        ->document()            \
        ->resourceLoader()

#define TO_SCRIPT_BINDING_INSTANCE(ptr) \
    ((Starfish::WebView*)ptr)           \
        ->mainBrowsingContext()         \
        ->window()                      \
        ->scriptBindingInstance()

#ifdef PORT_NEEDS_THREADED_PUBLIC_API
#define START_SIMPLE_THREADED_PUBLIC_API_WRAPPER Starfish::MessageLoop::runOnMainThreadSync([&]() -> size_t {
#define END_SIMPLE_THREADED_PUBLIC_API_WRAPPER \
    return 0;                                  \
    });
#else
#define START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
#define END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
#endif

#ifdef PORT_NEEDS_THREADED_PUBLIC_API
#define START_ASYNC_THREADED_PUBLIC_API_WRAPPER TO_WEBVIEW(m_impl)->messageLoop()->runOnMainThreadAsync([=]() -> void {
#define END_ASYNC_THREADED_PUBLIC_API_WRAPPER \
    });
#else
#define START_ASYNC_THREADED_PUBLIC_API_WRAPPER
#define END_ASYNC_THREADED_PUBLIC_API_WRAPPER
#endif

namespace LWE {

extern Starfish::Starfish* g_starfishInstance;

Settings::Settings(const std::string& default_ua, const std::string& ua)
    : m_defaultUserAgent(default_ua)
    , m_userAgent(ua)
#if defined(STARFISH_ENABLE_HTTPCACHE)
    , m_cacheMode(Starfish::HTTPCache::LOAD_DEFAULT)
#else
    , m_cacheMode(0)
#endif
    , m_defaultFontSize(LWE_DEFAULT_FONT_SIZE)
    , m_ttsMode(TTSMode::Default)
    , m_bgR(255)
    , m_bgG(255)
    , m_bgB(255)
    , m_bgA(255)
    , m_fgR(0)
    , m_fgG(0)
    , m_fgB(0)
    , m_fgA(255)
    , m_webSecurityMode(WebSecurityMode::Enable)
    , m_idleModeJob(IdleModeJob::IdleModeDefault)
    , m_idleModeCheckIntervalInMS(IdleModeCheckDefaultIntervalInMS)
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

TTSMode Settings::GetTTSMode() const
{
    return m_ttsMode;
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

void Settings::SetTTSMode(TTSMode mode)
{
    m_ttsMode = mode;
}

void Settings::SetBaseBackgroundColor(unsigned char r, unsigned char g,
                                      unsigned char b, unsigned char a)
{
    m_bgR = r;
    m_bgG = g;
    m_bgB = b;
    m_bgA = a;
}

void Settings::SetBaseForegroundColor(unsigned char r, unsigned char g,
                                      unsigned char b, unsigned char a)
{
    m_fgR = r;
    m_fgG = g;
    m_fgB = b;
    m_fgA = a;
}

void Settings::GetBaseBackgroundColor(unsigned char& r, unsigned char& g,
                                      unsigned char& b, unsigned char& a) const
{
    r = m_bgR;
    g = m_bgG;
    b = m_bgB;
    a = m_bgA;
}
void Settings::GetBaseForegroundColor(unsigned char& r, unsigned char& g,
                                      unsigned char& b, unsigned char& a) const
{
    r = m_fgR;
    g = m_fgG;
    b = m_fgB;
    a = m_fgA;
}

WebSecurityMode Settings::GetWebSecurityMode() const
{
    return m_webSecurityMode;
}

void Settings::SetWebSecurityMode(WebSecurityMode value)
{
    m_webSecurityMode = value;
}

IdleModeJob Settings::GetIdleModeJob() const
{
    return m_idleModeJob;
}

void Settings::SetIdleModeJob(IdleModeJob j)
{
    m_idleModeJob = j;
}

uint32_t Settings::GetIdleModeCheckIntervalInMS() const
{
    return m_idleModeCheckIntervalInMS;
}

void Settings::SetIdleModeCheckIntervalInMS(uint32_t intervalInMS)
{
    m_idleModeCheckIntervalInMS = intervalInMS;
}

ResourceError::ResourceError(int code, const std::string& description,
                             const std::string& url)
    : m_errorCode(code)
    , m_description(description)
    , m_url(url)
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

std::string ResourceError::GetUrl()
{
    return m_url;
}

static int convertErrorCode(Starfish::RequestErrorType errortype)
{
    return static_cast<int>(errortype);
}

static std::string convertErrorDescriton(Starfish::RequestErrorType errortype)
{
    // TODO:
    switch (errortype) {
    case Starfish::RequestErrorType::UnknownError:
        return "UnknownError";

    case Starfish::RequestErrorType::HostLookupError:
        return "HostLookupError";

    case Starfish::RequestErrorType::UnsupportedAuthSchemeError:
        return "UnsupportedAuthSchemeError";

    case Starfish::RequestErrorType::AuthenticationError:
        return "AuthenticationError";

    case Starfish::RequestErrorType::ProxyAuthenticationError:
        return "ProxyAuthenticationError";

    case Starfish::RequestErrorType::ConnectError:
        return "ConnectError";

    case Starfish::RequestErrorType::IOError:
        return "IOError";

    case Starfish::RequestErrorType::TimeoutError:
        return "TimeoutError";

    case Starfish::RequestErrorType::RedirectLoopError:
        return "RedirectLoopError";

    case Starfish::RequestErrorType::UnsupportedSchemeError:
        return "UnsupportedSchemeError";

    case Starfish::RequestErrorType::FailedSSLHandshakeError:
        return "FailedSSLHandshakeError";

    case Starfish::RequestErrorType::BadURLError:
        return "BadURLError";

    case Starfish::RequestErrorType::FileError:
        return "FileError";

    case Starfish::RequestErrorType::FileNotFoundError:
        return "FileNotFoundError";

    case Starfish::RequestErrorType::TooManyRequestError:
        return "TooManyRequestError";

    default:
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }
    return 0;
}

static Starfish::ScriptValue nativeCallbackFunction(
    Starfish::ScriptExecutionState state, Starfish::ScriptValue thisValue,
    size_t argc, Starfish::ScriptValue* argv, bool isNewExpression)
{
    auto callee = Starfish::toCalleeObject(state);
    if (callee) {
        void* data = callee->extraData();
        if (data) {
            Starfish::ScriptWrappable* w = (Starfish::ScriptWrappable*)data;
            if (w->isJavaScriptNativeHandler()) {
                Starfish::JavaScriptNativeHandler* jsNhandler =
                    (Starfish::JavaScriptNativeHandler*)w;
                Starfish::String* result = Starfish::String::emptyString;
                Starfish::String* param = Starfish::String::emptyString;
                if (argc > 0) {
                    Starfish::ScriptValue arg0 = argv[0];
                    param = Starfish::toBrowserString(state, arg0);
                }
                result = jsNhandler->callNativeHandler(param);
                return Starfish::createScriptValue(
                    Starfish::createScriptString(result));
            }
        }
    }
    return Starfish::scriptUndefined();
}

static Starfish::WebView* createWebViewInstance(unsigned width, unsigned height,
                                                float devicePixelRatio,
                                                const char* defaultFontName,
                                                const char* locale,
                                                const char* timezoneID)
{
    if (!LWE::IsInitialized()) {
        STARFISH_LOG_ERROR(
            "You must call LWE::Initialize function before using WebContainer "
            "or WebView");
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }
    std::string customUserAgentString;
    std::string builtinPolyfillPathString;

    Starfish::ScreenInfo info;
    info.rect.setWidth(width);
    info.rect.setHeight(height);
    info.availableRect.setWidth(width);
    info.availableRect.setHeight(height);
    info.devicePixelRatio = devicePixelRatio;

    ::Starfish::WebView* webView = ::Starfish::WebView::create(
        g_starfishInstance, locale, timezoneID, width, height,
        LWE_DEFAULT_FONT_SIZE,
        Starfish::String::createASCIIString(defaultFontName), info,
        Starfish::String::fromUTF8(customUserAgentString.data()),
        Starfish::String::fromUTF8(builtinPolyfillPathString.data()));
    return webView;
}

#if defined(STARFISH_TIZEN_VERSION_5_0)
WebContainer* WebContainer::Create(void* buffer, unsigned width,
                                   unsigned height, unsigned stride,
                                   float scaleFactor,
                                   const char* defaultFontName,
                                   const char* locale, const char* timezoneID)
{
#if defined(STARFISH_DALI)
#if !defined(PORT_WINDOW_BACKEND_GB)
    STARFISH_LOG_ERROR("Cannot use this set of function within this port!");
    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    return nullptr;
#endif

#if defined(PORT_NEEDS_THREADED_PUBLIC_API)
    return (WebContainer*)Starfish::MessageLoop::runOnMainThreadSync(
        [&]() -> size_t {
            auto webView =
                createWebViewInstance(width, height, scaleFactor,
                                      defaultFontName, locale, timezoneID);

            WebContainer* newWebContainer =
                new (GC_MALLOC_UNCOLLECTABLE(sizeof(WebView)))
                    WebContainer(webView);
            newWebContainer->UpdateBuffer(buffer, width, height, stride);
            return (size_t)newWebContainer;
        });
#else
    auto webView = createWebViewInstance(width, height, scaleFactor,
                                         defaultFontName, locale, timezoneID);
    WebContainer* newWebContainer =
        new (GC_MALLOC_UNCOLLECTABLE(sizeof(WebView))) WebContainer(webView);
    newWebContainer->UpdateBuffer(buffer, width, height, stride);
    return newWebContainer;
#endif
#else
    STARFISH_LOG_ERROR("Cannot use this set of function within this port!");
    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    return nullptr;
#endif
}
#else
WebContainer* WebContainer::Create(unsigned width, unsigned height,
                                   float scaleFactor,
                                   const char* defaultFontName,
                                   const char* locale, const char* timezoneID)
{
#if !defined(PORT_WINDOW_BACKEND_GB)
    STARFISH_LOG_ERROR("Cannot use this set of function within this port!");
    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    return nullptr;
#endif

#if defined(PORT_NEEDS_THREADED_PUBLIC_API)
    return (WebContainer*)Starfish::MessageLoop::runOnMainThreadSync(
        [&]() -> size_t {
            auto webView =
                createWebViewInstance(width, height, scaleFactor,
                                      defaultFontName, locale, timezoneID);

            WebContainer* newWebContainer =
                new (GC_MALLOC_UNCOLLECTABLE(sizeof(WebView)))
                    WebContainer(webView);

            return (size_t)newWebContainer;
        });
#else
    auto webView = createWebViewInstance(width, height, scaleFactor,
                                         defaultFontName, locale, timezoneID);
    WebContainer* newWebContainer =
        new (GC_MALLOC_UNCOLLECTABLE(sizeof(WebView))) WebContainer(webView);

    return newWebContainer;
#endif
}
#endif

#if defined(STARFISH_TIZEN_VERSION_5_0)
void WebContainer::UpdateBuffer(void* buffer, unsigned width, unsigned height,
                                unsigned stride)
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    ResizeTo(width, height);
    TO_WEBVIEW(m_impl)
        ->platformWindow()
        ->updateDrawingBufferAddress(buffer, stride);
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}
#endif

#if !defined(STARFISH_TIZEN_VERSION_5_0)
void WebContainer::RegisterPreRenderingHandler(
    const std::function<WebContainer::RenderInfo(void)>& cb)
{
#if !defined(PORT_WINDOW_BACKEND_GB)
    STARFISH_LOG_ERROR("Cannot use this set of function within this port!");
    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
#endif
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->platformWindow()
        ->registerRenderingPrepareCallback([cb](void) -> Starfish::RenderInfo {
            WebContainer::RenderInfo tmp = cb();
            Starfish::RenderInfo result;
            result.updatedBufferAddress = tmp.updatedBufferAddress;
            result.bufferStride = tmp.bufferStride;

            return result;
        });
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}
#endif

void WebContainer::RegisterOnRenderedHandler(
    const std::function<void(WebContainer*, const WebContainer::RenderResult&)>&
        cb)
{
#if !defined(PORT_WINDOW_BACKEND_GB)
    STARFISH_LOG_ERROR("Cannot use this set of function within this port!");
    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
#endif
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->platformWindow()
        ->registerRenderingFinishedCallback([this, cb](
            const Starfish::RenderResult& renderResult) {
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
    const std::function<void(WebContainer*, bool)>& onGLSwapBuffers,
    float devicePixelRatio, const char* defaultFontName, const char* locale,
    const char* timezoneID)
{
#if !defined(PORT_WINDOW_BACKEND_GL)
    STARFISH_LOG_ERROR("Cannot use this set of function within this port!");
    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
#endif

#if defined(PORT_NEEDS_THREADED_PUBLIC_API)
    return (WebContainer*)Starfish::MessageLoop::runOnMainThreadSync(
        [=]() -> size_t {
            auto webView =
                createWebViewInstance(width, height, devicePixelRatio,
                                      defaultFontName, locale, timezoneID);

            WebContainer* newWebContainer =
                new (GC_MALLOC_UNCOLLECTABLE(sizeof(WebView)))
                    WebContainer(webView);

            webView->platformWindow()->registerGLMakeCurrentCallback(
                [onGLMakeCurrent,
                 newWebContainer](Starfish::PlatformWindow* wnd) {
                    onGLMakeCurrent(newWebContainer);
                });

            webView->platformWindow()->registerGLSwapBuffersCallback(
                [onGLSwapBuffers, newWebContainer](
                    Starfish::PlatformWindow* wnd, bool mayNeedsSync) {
                    onGLSwapBuffers(newWebContainer, mayNeedsSync);
                });
            webView->platformWindow()->checkGLCompatibility();

            return (size_t)newWebContainer;
        });
#else
    auto webView = createWebViewInstance(width, height, devicePixelRatio,
                                         defaultFontName, locale, timezoneID);

    WebContainer* newWebContainer =
        new (GC_MALLOC_UNCOLLECTABLE(sizeof(WebView))) WebContainer(webView);

    webView->platformWindow()->registerGLMakeCurrentCallback(
        [onGLMakeCurrent, newWebContainer](Starfish::PlatformWindow* wnd) {
            onGLMakeCurrent(newWebContainer);
        });

    webView->platformWindow()->registerGLSwapBuffersCallback(
        [onGLSwapBuffers, newWebContainer](Starfish::PlatformWindow* wnd,
                                           bool mayNeedsSync) {
            onGLSwapBuffers(newWebContainer, mayNeedsSync);
        });

    return newWebContainer;
#endif
}

WebContainer* WebContainer::CreateHeadless(unsigned width, unsigned height,
                                           float devicePixelRatio,
                                           const char* defaultFontName,
                                           const char* locale,
                                           const char* timezoneID)
{
    auto webView = createWebViewInstance(width, height, devicePixelRatio,
                                         defaultFontName, locale, timezoneID);
    WebContainer* newWebContainer =
        new (GC_MALLOC_UNCOLLECTABLE(sizeof(WebView))) WebContainer(webView);

    return newWebContainer;
}

void WebContainer::ResizeTo(size_t width, size_t height)
{
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
    size_t ret = 0;
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    struct Data : public gc {
        void (*callback)(void*);
        void* data;
    };

    Data* d = new Data();
    d->callback = callback;
    d->data = data;
    ret = TO_WEBVIEW(m_impl)->timer()->addTimer(timeoutInMS, nullptr,
                                                [](void* data) {
                                                    Data* d = (Data*)data;
                                                    d->callback(d->data);
                                                },
                                                d, false);
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    return ret;
}

void WebContainer::ClearTimeout(size_t handle)
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)->timer()->removeTimer(handle);
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
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
        result.SetCacheMode(::Starfish::HTTPCache::LOAD_NO_CACHE);
    }
#endif
    result.SetProxyURL(TO_WEBVIEW(m_impl)->proxyURL());
#ifdef STARFISH_ENABLE_TTS
    result.SetTTSMode(TO_WEBVIEW(m_impl)->tts()->mode());
#endif
    result.SetWebSecurityMode(TO_WEBVIEW(m_impl)->getWebSecurityMode());
    result.SetIdleModeJob(TO_WEBVIEW(m_impl)->idleModeJob());
    result.SetIdleModeCheckIntervalInMS(
        TO_WEBVIEW(m_impl)->idleModeCheckIntervalInMS());
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    return result;
}

void WebContainer::LoadURL(const std::string& url)
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER

    TO_WEBVIEW(m_impl)
        ->loadHTMLDocument(Starfish::String::fromUTF8(url.data()));

    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
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
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
    if (data.size() > 0) {
        auto dataURI = Starfish::StringUtils::toBase64HTMLDataURI(data);
        TO_WEBVIEW(m_impl)
            ->loadHTMLDocument(Starfish::String::fromUTF8(dataURI.data()));
    } else {
        TO_WEBVIEW(m_impl)
            ->loadHTMLDocument(Starfish::String::fromUTF8("about:blank"));
    }
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::Reload()
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
    TO_LOCATION(m_impl)->reload();
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::StopLoading()
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
    STARFISH_ASSERT(m_impl);
    TO_RESOURCE_LOADER(m_impl).clear();
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::GoBack()
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
    TO_HISTORY(m_impl)->back();
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::GoForward()
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
    TO_HISTORY(m_impl)->forward();
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}

bool WebContainer::CanGoBack()
{
    bool ret = false;
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    ret = TO_HISTORY(m_impl)->canGoBack();
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    return ret;
}

bool WebContainer::CanGoForward()
{
    bool ret = false;
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    ret = TO_HISTORY(m_impl)->canGoForward();
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    return ret;
}

void WebContainer::AddJavaScriptInterface(
    const std::string& exposedObjectName, const std::string& jsFunctionName,
    std::function<std::string(const std::string&)> cb)
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER

    Starfish::String* objectName =
        Starfish::String::fromUTF8(exposedObjectName.c_str());
    Starfish::String* functionName =
        Starfish::String::fromUTF8(jsFunctionName.c_str());

    TO_WEBVIEW(m_impl)
        ->addJavaScriptNativeInterface(
            objectName, functionName, new Starfish::JavaScriptNativeHandler(
                                          TO_WEBVIEW(m_impl), functionName, cb),
            nativeCallbackFunction);

    if (TO_WEBVIEW(m_impl)->mainBrowsingContext()) {
        Starfish::registerJavaScriptNativeInterface(
            TO_SCRIPT_BINDING_INSTANCE(m_impl), objectName, functionName,
            new Starfish::JavaScriptNativeHandler(TO_WEBVIEW(m_impl),
                                                  functionName, cb),
            nativeCallbackFunction);
    }

    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}

std::string WebContainer::EvaluateJavaScript(const std::string& script)
{
    std::string ret;

    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    ret = TO_WEBVIEW(m_impl)
              ->evaluateJavaScript(Starfish::String::fromUTF8(script.c_str()))
              ->toUTF8NonGCString();
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER

    return ret;
}

void WebContainer::EvaluateJavaScript(
    const std::string& script, std::function<void(const std::string&)> cb)
{
    struct Params : public gc {
        Starfish::WebView* webview;
        std::string script;
        std::function<void(const std::string&)> cb;
    };
    Params* p = new Params;
    p->webview = TO_WEBVIEW(m_impl);
    p->script = script;
    p->cb = cb;

    if (Starfish::isMainThread()) {
        p->webview->messageLoop()->addIdler(
            p->webview->mainBrowsingContext()->window(),
            [](size_t handle, void* data) {
                Params* p = (Params*)data;
                p->webview->evaluateJavaScript(
                    Starfish::String::fromUTF8(p->script.c_str()), p->cb);
                delete p;
            },
            p);
    } else {
        p->webview->messageLoop()->addIdlerWithNoGCRootingInOtherThread(
            p->webview->mainBrowsingContext()->window(),
            [](size_t handle, void* data) {
                Params* p = (Params*)data;

                p->webview->evaluateJavaScript(
                    Starfish::String::fromUTF8(p->script.c_str()), p->cb);
                delete p;
            },
            p);
    }
}

void WebContainer::ClearHistory()
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)->historyManager()->clear();
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
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
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
    // TODO
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::Blur()
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)->platformWindow()->webView()->blur();
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::SetSettings(const Settings& settings)
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->setCustomUserAgentString(
            Starfish::String::fromUTF8(settings.GetUserAgentString().c_str()));
    TO_WEBVIEW(m_impl)->setProxyURL(settings.GetProxyURL());
#ifdef STARFISH_ENABLE_TTS
    TO_WEBVIEW(m_impl)->tts()->setMode(settings.GetTTSMode());
#endif
    unsigned char r, g, b, a;
    settings.GetBaseBackgroundColor(r, g, b, a);
    TO_WEBVIEW(m_impl)
        ->setBaseBackgroundColor(Starfish::Unit::Color(r, g, b, a));
    settings.GetBaseForegroundColor(r, g, b, a);
    TO_WEBVIEW(m_impl)
        ->setBaseForegroundColor(Starfish::Unit::Color(r, g, b, a));
#ifdef STARFISH_ENABLE_HTTPCACHE
    if (TO_STARFISH(m_impl)->httpCache()) {
        TO_STARFISH(m_impl)->httpCache()->setCacheMode(settings.GetCacheMode());
    } else {
        STARFISH_LOG_ERROR(
            "Http Cache could not initialized. So Changing cache mode is no "
            "effect.. ");
    }
#endif
    TO_WEBVIEW(m_impl)->setWebSecurityMode(settings.GetWebSecurityMode());
    TO_WEBVIEW(m_impl)->setIdleModeJob(settings.GetIdleModeJob());
    TO_WEBVIEW(m_impl)
        ->setIdleModeCheckIntervalInMS(settings.GetIdleModeCheckIntervalInMS());
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::RemoveJavascriptInterface(
    const std::string& exposedObjectName, const std::string& jsFunctionName)
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
    Starfish::String* objectName =
        Starfish::String::fromUTF8(exposedObjectName.c_str());
    Starfish::String* functionName =
        Starfish::String::fromUTF8(jsFunctionName.c_str());

    TO_WEBVIEW(m_impl)
        ->removeJavaScriptNativeInterface(objectName, functionName);

    if (!jsFunctionName.empty()) {
        Starfish::unregisterJavaScriptNativeInterface(
            TO_SCRIPT_BINDING_INSTANCE(m_impl), objectName, functionName);
    } else {
        Starfish::unregisterJavaScriptNativeInterface(
            TO_SCRIPT_BINDING_INSTANCE(m_impl), objectName);
    }
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}
void WebContainer::ClearCache()
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
#ifdef STARFISH_ENABLE_HTTPCACHE
    if (TO_STARFISH(m_impl)->httpCache() != nullptr) {
        TO_STARFISH(m_impl)->httpCache()->clear();
    }
#endif
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::RegisterOnReceivedErrorHandler(
    const std::function<void(WebContainer*, ResourceError)>& cb)
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->registerPublicWebViewHandler(
            Starfish::OnReceivedError, [this, cb](void* param) -> void {
                struct Param {
                    Starfish::RequestErrorType errorCode;
                    Starfish::String* url;
                };
                Param* p = (Param*)param;
                cb(this, ResourceError(convertErrorCode(p->errorCode),
                                       convertErrorDescriton(p->errorCode),
                                       p->url->toUTF8NonGCString()));
            });
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::RegisterOnPageParsedHandler(
    std::function<void(WebContainer*, const std::string&)> cb)
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->registerPublicWebViewHandler(
            Starfish::OnPageParsed, [this, cb](void* param) -> void {
                struct Param {
                    Starfish::String* url;
                };
                Param* p = (Param*)param;
                cb(this, p->url->toUTF8NonGCString());
            });
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::RegisterOnPageLoadedHandler(
    std::function<void(WebContainer*, const std::string&)> cb)
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->registerPublicWebViewHandler(
            Starfish::OnPageLoaded, [this, cb](void* param) -> void {
                struct Param {
                    Starfish::String* url;
                };
                Param* p = (Param*)param;
                cb(this, p->url->toUTF8NonGCString());
            });
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::RegisterOnPageStartedHandler(
    const std::function<void(WebContainer*, const std::string&)>& cb)
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->registerPublicWebViewHandler(
            Starfish::OnPageStarted, [this, cb](void* param) -> void {
                struct Param {
                    Starfish::String* url;
                };
                Param* p = (Param*)param;
                cb(this, p->url->toUTF8NonGCString());
            });
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::RegisterOnLoadResourceHandler(
    const std::function<void(WebContainer*, const std::string&)>& cb)
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->registerPublicWebViewHandler(
            Starfish::OnLoadResource, [this, cb](void* param) -> void {
                struct Param {
                    Starfish::String* url;
                };
                Param* p = (Param*)param;
                cb(this, p->url->toUTF8NonGCString());
            });
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::RegisterShouldOverrideUrlLoadingHandler(
    const std::function<bool(WebContainer*, const std::string&)>& cb)
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->registerPublicWebViewHandler(
            Starfish::ShouldOverrideUrlLoading,
            [this, cb](void* param) -> void {
                struct Param {
                    Starfish::ResourceURL* url;
                    Starfish::ReferrerURL* referrerURL;
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
                            TO_WEBVIEW(m_impl), p->url, p->referrerURL,
                            Starfish::HistoryManagerAction::Add, true);
                }
            });
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::RegisterOnDownloadStartHandler(
    const std::function<void(WebContainer*, const std::string&,
                             const std::string&, const std::string&,
                             const std::string&, long)>& cb)
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->registerPublicWebViewHandler(
            Starfish::OnDownloadStart, [this, cb](void* param) -> void {
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
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::RegisterShowDropdownMenuHandler(
    const std::function<void(WebContainer*, const std::vector<std::string>*,
                             int)>& cb)
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->platformWindow()
        ->registerCallbackHandler(Starfish::WindowHandlerShowDropdownMenu,
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
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::RegisterShowAlertHandler(
    const std::function<void(WebContainer*, const std::string&,
                             const std::string&)>& cb)
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->platformWindow()
        ->registerCallbackHandler(Starfish::WindowHandlerShowAlert,
                                  [this, cb](void* param) -> void {
                                      struct Param {
                                          std::string title;
                                          std::string message;
                                      };

                                      Param* p = (Param*)param;
                                      cb(this, p->title, p->message);
                                      delete p;
                                  });
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::RegisterCustomFileResourceRequestHandlers(
    std::function<const char*(const char* path)> resolveFilePathCallback,
    std::function<void*(const char* path)> fileOpenCallback,
    std::function<size_t(uint8_t* destBuffer, size_t size, void* handle)>
        fileReadCallback,
    std::function<long int(void* handle)> fileLengthCallback,
    std::function<void(void* handle)> fileCloseCallback)
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->registerCustomFileResourceRequestCallbacks(
            resolveFilePathCallback, fileOpenCallback, fileReadCallback,
            fileLengthCallback, fileCloseCallback);
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::CallHandler(const std::string& handler, void* param)
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
    if (handler.compare("onDropdownMenuItemSelected") == 0) {
        TO_WEBVIEW(m_impl)
            ->platformWindow()
            ->callHandler(Starfish::WindowHandlerOnDropdownMenuItemSelected,
                          param);
    }
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}

size_t WebContainer::Width()
{
    size_t ret = 0;
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    ret = TO_WEBVIEW(m_impl)->platformWindow()->width();
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    return ret;
}

size_t WebContainer::Height()
{
    size_t ret = 0;
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    ret = TO_WEBVIEW(m_impl)->platformWindow()->height();
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    return ret;
}

void WebContainer::RegisterOnProgressChangedHandler(
    const std::function<void(WebContainer*, int)>& cb)
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->registerPublicWebViewHandler(Starfish::OnProgressChanged,
                                       [this, cb](void* param) -> void {
                                           struct Param {
                                               int newProgress;
                                           };
                                           Param* p = (Param*)param;
                                           cb(this, p->newProgress);
                                       });
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::SetUserAgentString(const std::string& userAgent)
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->setCustomUserAgentString(
            Starfish::String::fromUTF8(userAgent.c_str()));
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

#if !defined(STARFISH_TIZEN_VERSION_5_0)
std::string WebContainer::GetUserAgentString()
{
    std::string ret;
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    ret = TO_WEBVIEW(m_impl)->userAgent()->toUTF8NonGCString();
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    return ret;
}
#endif

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

#if !defined(STARFISH_TIZEN_VERSION_5_0)
int WebContainer::GetCacheMode()
{
    int ret = 0;
#ifdef STARFISH_ENABLE_HTTPCACHE
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    if (TO_STARFISH(m_impl)->httpCache() != nullptr) {
        ret = TO_STARFISH(m_impl)->httpCache()->cacheMode();
    }
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
#endif
    return ret;
}
#endif

void WebContainer::SetDefaultFontSize(uint32_t size)
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    if (LWE_MIN_FONT_SIZE <= size && size <= LWE_MAX_FONT_SIZE) {
        TO_WEBVIEW(m_impl)->setDefaultFontSize(size);
    }
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

#if !defined(STARFISH_TIZEN_VERSION_5_0)
uint32_t WebContainer::GetDefaultFontSize()
{
    uint32_t ret = 0;
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    ret = TO_WEBVIEW(m_impl)->defaultFontSize();
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    return ret;
}
#endif

void WebContainer::DispatchMouseMoveEvent(MouseButtonValue button,
                                          MouseButtonsValue buttons, double x,
                                          double y)
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->platformWindow()
        ->dispatchMouseEvent(::Starfish::MouseEventKind::MouseEventMove,
                             ::Starfish::MouseData(button, buttons, x, y, 0));
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::DispatchMouseDownEvent(MouseButtonValue button,
                                          MouseButtonsValue buttons, double x,
                                          double y)
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->platformWindow()
        ->dispatchMouseEvent(::Starfish::MouseEventKind::MouseEventDown,
                             ::Starfish::MouseData(button, buttons, x, y, 0));
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::DispatchMouseUpEvent(MouseButtonValue button,
                                        MouseButtonsValue buttons, double x,
                                        double y)
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->platformWindow()
        ->dispatchMouseEvent(::Starfish::MouseEventKind::MouseEventUp,
                             ::Starfish::MouseData(button, buttons, x, y, 0));
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::DispatchMouseWheelEvent(double x, double y, int delta)
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->platformWindow()
        ->dispatchMouseWheelEvent(x, y, delta, true);
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::DispatchKeyDownEvent(KeyValue keyCode)
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->platformWindow()
        ->dispatchKeyEvent(::Starfish::KeyEventKind::KeyEventDown,
                           ::Starfish::PlatformKeyEventData(keyCode));
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::DispatchKeyPressEvent(KeyValue keyCode)
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->platformWindow()
        ->dispatchKeyEvent(::Starfish::KeyEventKind::KeyEventPress,
                           ::Starfish::PlatformKeyEventData(keyCode));
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::DispatchKeyUpEvent(KeyValue keyCode)
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->platformWindow()
        ->dispatchKeyEvent(::Starfish::KeyEventKind::KeyEventUp,
                           ::Starfish::PlatformKeyEventData(keyCode));
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::DispatchCompositionStartEvent(
    const std::string& soFarCompositiedString)
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->platformWindow()
        ->dispatchCompositionEvent(
            ::Starfish::CompositionEventKind::CompositionEventStart,
            ::Starfish::String::fromUTF8(soFarCompositiedString.data(),
                                         soFarCompositiedString.length()));
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::DispatchCompositionUpdateEvent(
    const std::string& soFarCompositiedString)
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->platformWindow()
        ->dispatchCompositionEvent(
            ::Starfish::CompositionEventKind::CompositionEventUpdate,
            ::Starfish::String::fromUTF8(soFarCompositiedString.data(),
                                         soFarCompositiedString.length()));
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::DispatchCompositionEndEvent(
    const std::string& soFarCompositiedString)
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->platformWindow()
        ->dispatchCompositionEvent(
            ::Starfish::CompositionEventKind::CompositionEventEnd,
            ::Starfish::String::fromUTF8(soFarCompositiedString.data(),
                                         soFarCompositiedString.length()));
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}
void WebContainer::RegisterOnShowSoftwareKeyboardIfPossibleHandler(
    const std::function<void(WebContainer*)>& cb)
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->platformWindow()
        ->registerShowSoftwareKeyboardIfPossibleCallback(
            [this, cb]() { cb(this); });
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::RegisterOnHideSoftwareKeyboardIfPossibleHandler(
    const std::function<void(WebContainer*)>& cb)
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->platformWindow()
        ->registerHideSoftwareKeyboardIfPossibleCallback(
            [this, cb]() { cb(this); });
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::RegisterSetNeedsRenderingCallback(
    const std::function<void(
        WebContainer*, const std::function<void()>& doRenderingFunction)>& cb)
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)
        ->platformWindow()
        ->registerSetNeedsRenderingCallback(
            [this, cb](Starfish::PlatformWindow* wnd) {
                std::function<void()> fn = [wnd]() {
                    auto p = wnd;
                    p->rendering();
                };
                cb(this, fn);
            });
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::SetUserData(const std::string& key, void* data)
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)->publicLayerUserDataMap()[key] = data;
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}

void* WebContainer::GetUserData(const std::string& key)
{
    void* ret = nullptr;

    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    ret = TO_WEBVIEW(m_impl)->publicLayerUserDataMap()[key];
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER

    return ret;
}

} // namespace LWE
