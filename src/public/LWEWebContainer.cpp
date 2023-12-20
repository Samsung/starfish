/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#include "LWEWebView.h"

#include "LWEDelegate.h"

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
#include "core/modules/profiling/Profiling.h"
#include "core/modules/tts/TTS.h"
#include "core/dom/MouseEvent.h"
#include "core/dom/KeyboardEvent.h"
#include "core/page/WebView.h"
#include "platform/network/http/HTTPCache.h"
#include "platform/event/PlatformKeyEventData.h"
#include "platform/loader/ResourceURL.h"
#include "platform/loader/ResourceLoader.h"

#include <EscargotPublic.h>

#if defined(STARFISH_TIZEN_VERSION_5_0) && !defined(TIZEN_COMPAT_HEADER_5_0)
#error "Version Mismatch: You must build LWE on Tizen 5.5 Environment"
#endif

#if defined(STARFISH_TIZEN_VERSION_5_5) && defined(TIZEN_COMPAT_HEADER_5_0)
#error "Version Mismatch: You must build LWE on Tizen 5.0 Environment"
#endif

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

namespace LWEDelegate {
extern Starfish::Starfish* g_starfishInstance;
}

namespace LWE {

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
    if (!LWEDelegate::LWE::IsInitialized()) {
        STARFISH_LOG_ERROR(
            "You must call LWE::Initialize function before using WebContainer "
            "or WebView");
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }
    std::string customUserAgentString;
    std::string builtinPolyfillPathString;

    Starfish::LayoutUnit scaledWidth = width / devicePixelRatio;
    scaledWidth = scaledWidth.ceil();
    Starfish::LayoutUnit scaledHeight = height / devicePixelRatio;
    scaledHeight = scaledHeight.ceil();
    Starfish::ScreenInfo info;
    info.rect.setWidth(scaledWidth);
    info.rect.setHeight(scaledHeight);
    info.availableRect.setWidth(scaledWidth);
    info.availableRect.setHeight(scaledHeight);
    info.devicePixelRatio = devicePixelRatio;

    STARFISH_RELEASE_ASSERT(defaultFontName != nullptr);
    STARFISH_RELEASE_ASSERT(locale != nullptr);
    STARFISH_RELEASE_ASSERT(timezoneID != nullptr);

    ::Starfish::WebView* webView = ::Starfish::WebView::create(
        LWEDelegate::g_starfishInstance, locale, timezoneID, width, height,
        LWE_DEFAULT_FONT_SIZE,
        Starfish::String::createASCIIString(defaultFontName,
                                            strlen(defaultFontName)),
        info,
        Starfish::String::fromUTF8(customUserAgentString.data(),
                                   customUserAgentString.size()),
        Starfish::String::fromUTF8(builtinPolyfillPathString.data(),
                                   builtinPolyfillPathString.size()));
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

    STARFISH_RELEASE_ASSERT(defaultFontName != nullptr);
    STARFISH_RELEASE_ASSERT(locale != nullptr);
    STARFISH_RELEASE_ASSERT(timezoneID != nullptr);

#if defined(PORT_NEEDS_THREADED_PUBLIC_API)
    return (WebContainer*)Starfish::MessageLoop::runOnMainThreadSync(
        [&]() -> size_t {
            auto webView =
                createWebViewInstance(width, height, scaleFactor,
                                      defaultFontName, locale, timezoneID);

            WebContainer* newWebContainer = new (NoGC) WebContainer(webView);
            newWebContainer->UpdateBuffer(buffer, width, height, stride);
            return (size_t)newWebContainer;
        });
#else
    auto webView = createWebViewInstance(width, height, scaleFactor,
                                         defaultFontName, locale, timezoneID);
    WebContainer* newWebContainer = new (NoGC) WebContainer(webView);
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

    STARFISH_RELEASE_ASSERT(defaultFontName != nullptr);
    STARFISH_RELEASE_ASSERT(locale != nullptr);
    STARFISH_RELEASE_ASSERT(timezoneID != nullptr);

#if defined(PORT_NEEDS_THREADED_PUBLIC_API)
    return (WebContainer*)Starfish::MessageLoop::runOnMainThreadSync(
        [&]() -> size_t {
            auto webView =
                createWebViewInstance(width, height, scaleFactor,
                                      defaultFontName, locale, timezoneID);

            WebContainer* newWebContainer = new (NoGC) WebContainer(webView);

            return (size_t)newWebContainer;
        });
#else
    auto webView = createWebViewInstance(width, height, scaleFactor,
                                         defaultFontName, locale, timezoneID);
    WebContainer* newWebContainer = new (NoGC) WebContainer(webView);

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
    TO_WEBVIEW(m_impl)->platformWindow()->updateDrawingBufferAddress(buffer,
                                                                     stride);
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
    TO_WEBVIEW(m_impl)->platformWindow()->registerRenderingPrepareCallback(
        [cb](void) -> Starfish::RenderInfo {
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
    TO_WEBVIEW(m_impl)->platformWindow()->registerRenderingFinishedCallback(
        [this, cb](const Starfish::RenderResult& renderResult) {
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

            WebContainer* newWebContainer = new (NoGC) WebContainer(webView);

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

            return (size_t)newWebContainer;
        });
#else
    auto webView = createWebViewInstance(width, height, devicePixelRatio,
                                         defaultFontName, locale, timezoneID);

    WebContainer* newWebContainer = new (NoGC) WebContainer(webView);

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

#ifdef STARFISH_FLUTTER
#include <tbm_surface.h>
#endif

WebContainer* WebContainer::CreateWithPlatformImage(
    unsigned width, unsigned height,
    const std::function<ExternalImageInfo(void)>& prepareImageCb,
    const std::function<void(WebContainer*, bool needsFlush)>& flushCb,
    float devicePixelRatio, const char* defaultFontName, const char* locale,
    const char* timezoneID)
{
#if defined(PORT_NEEDS_THREADED_PUBLIC_API)
    return (WebContainer*)Starfish::MessageLoop::runOnMainThreadSync(
        [=]() -> size_t {
            auto webView =
                createWebViewInstance(width, height, devicePixelRatio,
                                      defaultFontName, locale, timezoneID);

            WebContainer* newWebContainer = new (NoGC) WebContainer(webView);
            webView->platformWindow()->registerRenderingPrepareCallback(
                [prepareImageCb](void) -> Starfish::RenderInfo {
                    WebContainer::ExternalImageInfo buffer = prepareImageCb();
                    Starfish::RenderInfo result;
#ifdef STARFISH_FLUTTER
                    tbm_surface_info_s tbmSurfaceInfo;
                    if (tbm_surface_map((tbm_surface_h)buffer.imageAddress,
                                        TBM_SURF_OPTION_WRITE,
                                        &tbmSurfaceInfo) ==
                        TBM_SURFACE_ERROR_NONE) {
                        result.updatedBufferAddress =
                            tbmSurfaceInfo.planes[0].ptr;
                        result.bufferStride = tbmSurfaceInfo.planes[0].stride;
                    }
#endif
                    return result;
                });

            webView->platformWindow()->registerRenderingFinishedCallback(
                [newWebContainer,
                 flushCb](const Starfish::RenderResult& renderResult) {
                    flushCb(newWebContainer,
                            renderResult.didPaintingOrCompositing);
                });

            return (size_t)newWebContainer;
        });
#else
    auto webView = createWebViewInstance(width, height, devicePixelRatio,
                                         defaultFontName, locale, timezoneID);

    WebContainer* newWebContainer = new (NoGC) WebContainer(webView);
    webView->platformWindow()->registerRenderingPrepareCallback(
        [prepareImageCb](void) -> Starfish::RenderInfo {
            WebContainer::ExternalImageInfo tmp = prepareImageCb();
            Starfish::RenderInfo result;
            result.updatedBufferAddress = tmp.imageAddress;
            result.bufferStride = 0;
            return result;
        });

    webView->platformWindow()->registerRenderingFinishedCallback(
        [newWebContainer, flushCb](const Starfish::RenderResult& renderResult) {
            flushCb(newWebContainer, renderResult.didPaintingOrCompositing);
        });
    return newWebContainer;
#endif
}

WebContainer* WebContainer::CreateGLWithPlatformImage(
    unsigned width, unsigned height,
    const std::function<void(WebContainer*)>& onGLMakeCurrent,
    const std::function<void(WebContainer*, bool mayNeedsSync)>&
        onGLSwapBuffers,
    const std::function<ExternalImageInfo(void)>& prepareImageCb,
    const std::function<void(WebContainer*, bool needsFlush)>& flushCb,
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

            WebContainer* newWebContainer = new (NoGC) WebContainer(webView);

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

            webView->platformWindow()->registerRenderingPrepareCallback(
                [prepareImageCb](void) -> Starfish::RenderInfo {
                    WebContainer::ExternalImageInfo tmp = prepareImageCb();
                    Starfish::RenderInfo result;
                    result.updatedBufferAddress = tmp.imageAddress;
                    result.bufferStride = 0;
                    return result;
                });

            webView->platformWindow()->registerSurfaceFlushedCallback(
                [newWebContainer, flushCb](bool needsFlush) {
                    flushCb(newWebContainer, needsFlush);
                });

            return (size_t)newWebContainer;
        });
#else
    auto webView = createWebViewInstance(width, height, devicePixelRatio,
                                         defaultFontName, locale, timezoneID);

    WebContainer* newWebContainer = new (NoGC) WebContainer(webView);

    webView->platformWindow()->registerGLMakeCurrentCallback(
        [onGLMakeCurrent, newWebContainer](Starfish::PlatformWindow* wnd) {
            onGLMakeCurrent(newWebContainer);
        });

    webView->platformWindow()->registerGLSwapBuffersCallback(
        [onGLSwapBuffers, newWebContainer](Starfish::PlatformWindow* wnd,
                                           bool mayNeedsSync) {
            onGLSwapBuffers(newWebContainer, mayNeedsSync);
        });

    webView->platformWindow()->registerRenderingPrepareCallback(
        [prepareImageCb](void) -> Starfish::RenderInfo {
            WebContainer::ExternalImageInfo tmp = prepareImageCb();
            Starfish::RenderInfo result;
            result.updatedBufferAddress = tmp.imageAddress;
            result.bufferStride = 0;
            return result;
        });

    webView->platformWindow()->registerRenderingFinishedCallback(
        [newWebContainer, flushCb](const Starfish::RenderResult& renderResult) {
            flushCb(newWebContainer, renderResult.didPaintingOrCompositing);
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
    WebContainer* newWebContainer = new (NoGC) WebContainer(webView);

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
    TO_WEBVIEW(m_impl)->messageLoop()->addIdler(
        nullptr,
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
    ret = TO_WEBVIEW(m_impl)->timer()->addTimer(
        timeoutInMS, nullptr,
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

void WebContainer::RegisterCanRenderingHandler(
    const std::function<bool(WebContainer*)>& cb)
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)->platformWindow()->registerCanRenderingCallback(
        [this, cb](Starfish::PlatformWindow* wnd) -> bool { return cb(this); });
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

Settings WebContainer::GetSettings()
{
    Settings result(USER_AGENT(STARFISH_NAME, VERSION), "");

    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    result.SetUserAgentString(
        TO_WEBVIEW(m_impl)->userAgent()->toUTF8NonGCString());
#ifdef STARFISH_ENABLE_HTTPCACHE
    Nullable<::Starfish::HTTPCache*> cache = TO_STARFISH(m_impl)->httpCache();
    if (cache.hasValue()) {
        result.SetCacheMode(cache->cacheMode());
    } else {
        result.SetCacheMode(::Starfish::HTTPCache::LOAD_NO_CACHE);
    }
#endif
    result.SetProxyURL(TO_WEBVIEW(m_impl)->proxyURL());
#ifdef STARFISH_ENABLE_TTS
    result.SetTTSMode(TO_WEBVIEW(m_impl)->tts()->mode());
    result.SetTTSLanguage(TO_WEBVIEW(m_impl)->tts()->userLanguage());
#endif
    result.SetWebSecurityMode(TO_WEBVIEW(m_impl)->getWebSecurityMode());
    result.SetIdleModeJob(TO_WEBVIEW(m_impl)->idleModeJob());
    result.SetIdleModeCheckIntervalInMS(
        TO_WEBVIEW(m_impl)->idleModeCheckIntervalInMS());
    result.SetNeedsDownloadWebFontsEarly(
        TO_WEBVIEW(m_impl)->needsDownloadWebFontsEarly());
    result.SetNeedsDownScaleImageResourceLargerThan(
        TO_WEBVIEW(m_impl)->needsDownScaleImageResourceLargerThan());
#ifndef TIZEN_COMPAT_HEADER_5_0
    result.SetScrollbarVisible(TO_WEBVIEW(m_impl)->scrollbarVisible());
#endif
    result.SetUseExternalPopup(TO_WEBVIEW(m_impl)->useExternalPopup());
    result.SetUseSpatialNavigation(TO_WEBVIEW(m_impl)->useSpatialNavigation());
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    return result;
}

void WebContainer::LoadURL(const std::string& url)
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER

    TO_WEBVIEW(m_impl)->loadHTMLDocument(
        Starfish::String::fromUTF8(url.data(), url.size()));

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
        auto dataURI = Starfish::Base64Utils::encodeBase64HTMLDataURI(data);
        TO_WEBVIEW(m_impl)->loadHTMLDocument(
            Starfish::String::fromUTF8(dataURI.data(), dataURI.size()));
    } else {
        TO_WEBVIEW(m_impl)->loadHTMLDocument(
            Starfish::String::fromUTF8("about:blank"));
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

    Starfish::String* objectName = Starfish::String::fromUTF8(
        exposedObjectName.data(), exposedObjectName.size());
    Starfish::String* functionName = Starfish::String::fromUTF8(
        jsFunctionName.data(), jsFunctionName.size());

    TO_WEBVIEW(m_impl)->addJavaScriptNativeInterface(
        objectName, functionName,
        new Starfish::JavaScriptNativeHandler(TO_WEBVIEW(m_impl), functionName,
                                              cb),
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
              ->evaluateJavaScript(
                  Starfish::String::fromUTF8(script.data(), script.size()))
              ->toUTF8NonGCString();
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER

    return ret;
}

void WebContainer::EvaluateJavaScript(
    const std::string& script, std::function<void(const std::string&)> cb)
{
    struct Params {
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
                STARFISH_ASSERT(data != nullptr);
                Params* p = (Params*)data;
                p->webview->evaluateJavaScript(
                    Starfish::String::fromUTF8(p->script.data(),
                                               p->script.size()),
                    p->cb);
                delete p;
            },
            p);
    } else {
        p->webview->messageLoop()->addIdlerWithNoGCRootingInOtherThread(
            p->webview->mainBrowsingContext()->window(),
            [](size_t handle, void* data) {
                STARFISH_ASSERT(data != nullptr);
                Params* p = (Params*)data;

                p->webview->evaluateJavaScript(
                    Starfish::String::fromUTF8(p->script.data(),
                                               p->script.size()),
                    p->cb);
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
    STARFISH_UNIMPLEMENTED();
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
    TO_WEBVIEW(m_impl)->setCustomUserAgentString(
        Starfish::String::fromUTF8(settings.GetUserAgentString().data(),
                                   settings.GetUserAgentString().size()));
    TO_WEBVIEW(m_impl)->setProxyURL(settings.GetProxyURL());
#ifdef STARFISH_ENABLE_TTS
    TO_WEBVIEW(m_impl)->tts()->setMode(settings.GetTTSMode());
    TO_WEBVIEW(m_impl)->tts()->setUserLanguage(settings.GetTTSLanguage());
#endif
    unsigned char r, g, b, a;
    settings.GetBaseBackgroundColor(r, g, b, a);
    TO_WEBVIEW(m_impl)->setBaseBackgroundColor(
        Starfish::Unit::Color(r, g, b, a));
    settings.GetBaseForegroundColor(r, g, b, a);
    TO_WEBVIEW(m_impl)->setBaseForegroundColor(
        Starfish::Unit::Color(r, g, b, a));
#ifdef STARFISH_ENABLE_HTTPCACHE
    Nullable<::Starfish::HTTPCache*> cache = TO_STARFISH(m_impl)->httpCache();
    if (cache.hasValue()) {
        cache->setCacheMode(settings.GetCacheMode());
    } else {
        STARFISH_LOG_ERROR(
            "Http Cache could not initialized. So Changing cache mode is no "
            "effect.. ");
    }
#endif
    TO_WEBVIEW(m_impl)->setWebSecurityMode(settings.GetWebSecurityMode());
    TO_WEBVIEW(m_impl)->setIdleModeJob(settings.GetIdleModeJob());
    TO_WEBVIEW(m_impl)->setIdleModeCheckIntervalInMS(
        settings.GetIdleModeCheckIntervalInMS());
    TO_WEBVIEW(m_impl)->setNeedsDownloadWebFontsEarly(
        settings.NeedsDownloadWebFontsEarly());
    TO_WEBVIEW(m_impl)->setUseHttp2(settings.UseHttp2());
    TO_WEBVIEW(m_impl)->setNeedsDownScaleImageResourceLargerThan(
        settings.NeedsDownScaleImageResourceLargerThan());
#ifndef TIZEN_COMPAT_HEADER_5_0
    TO_WEBVIEW(m_impl)->setScrollbarVisible(settings.ScrollbarVisible());
#endif
    TO_WEBVIEW(m_impl)->setUseExternalPopup(settings.UseExternalPopup());
    TO_WEBVIEW(m_impl)->setUseSpatialNavigation(
        settings.UseSpatialNavigation());

    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::RemoveJavascriptInterface(
    const std::string& exposedObjectName, const std::string& jsFunctionName)
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
    Starfish::String* objectName = Starfish::String::fromUTF8(
        exposedObjectName.data(), exposedObjectName.size());
    Starfish::String* functionName = Starfish::String::fromUTF8(
        jsFunctionName.data(), jsFunctionName.size());

    TO_WEBVIEW(m_impl)->removeJavaScriptNativeInterface(objectName,
                                                        functionName);

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
    Nullable<::Starfish::HTTPCache*> cache = TO_STARFISH(m_impl)->httpCache();
    if (cache.hasValue()) {
        cache->clear();
    }
#endif
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::RegisterOnReceivedErrorHandler(
    const std::function<void(WebContainer*, ResourceError)>& cb)
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)->registerPublicWebViewHandler(
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
    TO_WEBVIEW(m_impl)->registerPublicWebViewHandler(
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
    TO_WEBVIEW(m_impl)->registerPublicWebViewHandler(
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
    TO_WEBVIEW(m_impl)->registerPublicWebViewHandler(
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
    TO_WEBVIEW(m_impl)->registerPublicWebViewHandler(
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
    TO_WEBVIEW(m_impl)->registerPublicWebViewHandler(
        Starfish::ShouldOverrideUrlLoading, [this, cb](void* param) -> void {
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
                TO_WEBVIEW(m_impl)->messageLoop()->invokeNavigate(
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
    TO_WEBVIEW(m_impl)->registerPublicWebViewHandler(
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
            cb(this, p->url, p->userAgent, p->contentDisposition, p->mimetype,
               p->contentLength);
            delete p;
        });
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::RegisterShowDropdownMenuHandler(
    const std::function<void(WebContainer*, const std::vector<std::string>*,
                             int)>& cb)
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)->platformWindow()->registerCallbackHandler(
        Starfish::WindowHandlerShowDropdownMenu,
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
    TO_WEBVIEW(m_impl)->platformWindow()->registerCallbackHandler(
        Starfish::WindowHandlerShowAlert, [this, cb](void* param) -> void {
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
    TO_WEBVIEW(m_impl)->registerCustomFileResourceRequestCallbacks(
        resolveFilePathCallback, fileOpenCallback, fileReadCallback,
        fileLengthCallback, fileCloseCallback);
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::CallHandler(const std::string& handler, void* param)
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
    if (handler.compare("onDropdownMenuItemSelected") == 0) {
        TO_WEBVIEW(m_impl)->platformWindow()->callHandler(
            Starfish::WindowHandlerOnDropdownMenuItemSelected, param);
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
    TO_WEBVIEW(m_impl)->registerPublicWebViewHandler(
        Starfish::OnProgressChanged, [this, cb](void* param) -> void {
            struct Param {
                int newProgress;
            };
            Param* p = (Param*)param;
            cb(this, p->newProgress);
        });
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::RegisterDebuggerShouldInitHandler(
    const std::function<void(const std::string& url, int port, bool& ret)>& cb)
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)->registerPublicWebViewHandler(
        Starfish::DebuggerShouldInit, [cb](void* param) -> void {
            struct Param {
                std::string url;
                int port;
                bool* ret;
            };
            Param* p = (Param*)param;
            cb(p->url, p->port, *p->ret);
        });
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::RegisterDebuggerShouldContinueWaitingHandler(
    const std::function<void(const std::string& url, int port, bool& ret)>& cb)
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)->registerPublicWebViewHandler(
        Starfish::DebuggerShouldContinueWaiting, [cb](void* param) -> void {
            struct Param {
                std::string url;
                int port;
                bool* ret;
            };
            Param* p = (Param*)param;
            cb(p->url, p->port, *p->ret);
        });
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::SetUserAgentString(const std::string& userAgent)
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)->setCustomUserAgentString(
        Starfish::String::fromUTF8(userAgent.data(), userAgent.size()));
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
    Nullable<::Starfish::HTTPCache*> cache = TO_STARFISH(m_impl)->httpCache();
    if (cache.hasValue()) {
        cache->setCacheMode(mode);
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
    Nullable<::Starfish::HTTPCache*> cache = TO_STARFISH(m_impl)->httpCache();
    if (cache.hasValue()) {
        ret = cache->cacheMode();
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
    TO_WEBVIEW(m_impl)->platformWindow()->dispatchMouseEvent(
        ::Starfish::MouseEventKind::MouseEventMove,
        ::Starfish::MouseData(button, buttons, x, y, 0, Starfish::timestamp()));
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::DispatchMouseDownEvent(MouseButtonValue button,
                                          MouseButtonsValue buttons, double x,
                                          double y)
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)->platformWindow()->dispatchMouseEvent(
        ::Starfish::MouseEventKind::MouseEventDown,
        ::Starfish::MouseData(button, buttons, x, y, 0, Starfish::timestamp()));
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::DispatchMouseUpEvent(MouseButtonValue button,
                                        MouseButtonsValue buttons, double x,
                                        double y)
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)->platformWindow()->dispatchMouseEvent(
        ::Starfish::MouseEventKind::MouseEventUp,
        ::Starfish::MouseData(button, buttons, x, y, 0, Starfish::timestamp()));
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::DispatchMouseWheelEvent(double x, double y, int delta)
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)->platformWindow()->dispatchMouseWheelEvent(x, y, delta,
                                                                  true);
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::DispatchKeyDownEvent(KeyValue keyCode)
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)->platformWindow()->dispatchKeyEvent(
        ::Starfish::KeyEventKind::KeyEventDown,
        ::Starfish::PlatformKeyEventData(keyCode));
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::DispatchKeyPressEvent(KeyValue keyCode)
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)->platformWindow()->dispatchKeyEvent(
        ::Starfish::KeyEventKind::KeyEventPress,
        ::Starfish::PlatformKeyEventData(keyCode));
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::DispatchKeyUpEvent(KeyValue keyCode)
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)->platformWindow()->dispatchKeyEvent(
        ::Starfish::KeyEventKind::KeyEventUp,
        ::Starfish::PlatformKeyEventData(keyCode));
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::DispatchCompositionStartEvent(
    const std::string& soFarCompositiedString)
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)->platformWindow()->dispatchCompositionEvent(
        ::Starfish::CompositionEventKind::CompositionEventStart,
        ::Starfish::String::fromUTF8(soFarCompositiedString.data(),
                                     soFarCompositiedString.length()),
        nullptr);
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::DispatchCompositionUpdateEvent(
    const std::string& soFarCompositiedString)
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)->platformWindow()->dispatchCompositionEvent(
        ::Starfish::CompositionEventKind::CompositionEventUpdate,
        ::Starfish::String::fromUTF8(soFarCompositiedString.data(),
                                     soFarCompositiedString.length()),
        nullptr);
    END_ASYNC_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::DispatchCompositionEndEvent(
    const std::string& soFarCompositiedString)
{
    START_ASYNC_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)->platformWindow()->dispatchCompositionEvent(
        ::Starfish::CompositionEventKind::CompositionEventEnd,
        ::Starfish::String::fromUTF8(soFarCompositiedString.data(),
                                     soFarCompositiedString.length()),
        nullptr);
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
    TO_WEBVIEW(m_impl)->platformWindow()->registerSetNeedsRenderingCallback(
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

std::string WebContainer::GetTitle()
{
    Starfish::String* ret = Starfish::String::emptyString;
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    ret = TO_WEBVIEW(m_impl)->mainBrowsingContext()->document()->title();
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    return ret->toUTF8NonGCString();
}

void WebContainer::ScrollTo(int x, int y)
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)->mainBrowsingContext()->window()->scrollTo((double)x,
                                                                  (double)y);
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

void WebContainer::ScrollBy(int x, int y)
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)->mainBrowsingContext()->window()->scrollBy((double)x,
                                                                  (double)y);
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

int WebContainer::GetScrollX()
{
    int x = 0;
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    x = (int)TO_WEBVIEW(m_impl)->mainBrowsingContext()->window()->scrollX();
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    return x;
}

int WebContainer::GetScrollY()
{
    int y = 0;
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    y = (int)TO_WEBVIEW(m_impl)->mainBrowsingContext()->window()->scrollY();
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    return y;
}

void WebContainer::SetDevicePixelRatio(float dpr)
{
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    TO_WEBVIEW(m_impl)->platformWindow()->setDevicePixelRatio(dpr);
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
}

float WebContainer::GetDevicePixelRatio()
{
    float dpr = 0;
    START_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    dpr = TO_WEBVIEW(m_impl)->platformWindow()->getDevicePixelRatio();
    END_SIMPLE_THREADED_PUBLIC_API_WRAPPER
    return dpr;
}

} // namespace LWE
