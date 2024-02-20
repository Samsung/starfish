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

#include "LWEWebContainerDelegate.h"
#include "ThreadedCallHelper.h"

#include "LWEDelegate.h"
#include "SettingsDelegate.h"
#include "ResourceErrorDelegate.h"

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
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/message_loop/Timer.h"
#include "core/modules/profiling/Profiling.h"
#include "core/modules/tts/TTS.h"
#include "core/dom/MouseEvent.h"
#include "core/dom/KeyboardEvent.h"
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

#define LWE_DEFAULT_FONT_SIZE 16
#define LWE_MIN_FONT_SIZE 1
#define LWE_MAX_FONT_SIZE 72

namespace LWEDelegate {
extern Starfish::Starfish* g_starfishInstance;

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

class WebContainerImpl : public WebContainer {
public:
    virtual void RegisterPreRenderingHandler(
        const std::function<RenderInfo(void)>& cb) override;
    virtual void RegisterOnRenderedHandler(
        const std::function<void(
            WebContainer*, const RenderResult& renderResult)>& cb) override;
    virtual void UpdateBuffer(void* buffer, unsigned width, unsigned height,
                              unsigned stride);

    virtual void AddIdleCallback(void (*callback)(void*), void* data) override;
    virtual size_t AddTimeout(void (*callback)(void*), void* data,
                              size_t timeoutInMS) override;
    void ClearTimeout(size_t handle) override;

    void RegisterCanRenderingHandler(
        const std::function<bool(WebContainer*)>& cb) override;

    Settings* GetSettings() override;
    void LoadURL(const std::string& url) override;
    std::string GetURL() override;
    void LoadData(const std::string& data) override;
    void Reload() override;
    void StopLoading() override;
    void GoBack() override;
    void GoForward() override;
    bool CanGoBack() override;
    bool CanGoForward() override;
    void AddJavaScriptInterface(
        const std::string& exposedObjectName, const std::string& jsFunctionName,
        std::function<std::string(const std::string&)> cb) override;
    std::string EvaluateJavaScript(const std::string& script) override;
    void EvaluateJavaScript(
        const std::string& script,
        std::function<void(const std::string&)> cb) override;
    void ClearHistory() override;
    void Destroy() override;
    void Pause() override;
    void Resume() override;

    void ResizeTo(size_t width, size_t height) override;

    void Focus() override;
    void Blur() override;

    void SetSettings(const Settings* settings) override;
    void RemoveJavascriptInterface(const std::string& exposedObjectName,
                                   const std::string& jsFunctionName) override;
    void ClearCache() override;

    void RegisterOnReceivedErrorHandler(
        const std::function<void(WebContainer*, ResourceError*)>& cb) override;
    void RegisterOnPageParsedHandler(
        std::function<void(WebContainer*, const std::string&)> cb) override;
    void RegisterOnPageLoadedHandler(
        std::function<void(WebContainer*, const std::string&)> cb) override;
    void RegisterOnPageStartedHandler(
        const std::function<void(WebContainer*, const std::string&)>& cb)
        override;
    void RegisterOnLoadResourceHandler(
        const std::function<void(WebContainer*, const std::string&)>& cb)
        override;
    void RegisterShouldOverrideUrlLoadingHandler(
        const std::function<bool(WebContainer*, const std::string&)>& cb)
        override;
    void RegisterOnProgressChangedHandler(
        const std::function<void(WebContainer*, int progress)>& cb) override;
    void RegisterOnDownloadStartHandler(
        const std::function<void(WebContainer*, const std::string&,
                                 const std::string&, const std::string&,
                                 const std::string&, long)>& cb) override;

    void RegisterShowDropdownMenuHandler(
        const std::function<void(WebContainer*, const std::vector<std::string>*,
                                 int)>& cb) override;
    void RegisterShowAlertHandler(
        const std::function<void(WebContainer*, const std::string&,
                                 const std::string&)>& cb) override;

    void RegisterCustomFileResourceRequestHandlers(
        std::function<const char*(const char* path)> resolveFilePathCallback,
        std::function<void*(const char* path)> fileOpenCallback,
        std::function<size_t(uint8_t* destBuffer, size_t size, void* handle)>
            fileReadCallback,
        std::function<long int(void* handle)> fileLengthCallback,
        std::function<void(void* handle)> fileCloseCallback) override;

    void RegisterDebuggerShouldInitHandler(
        const std::function<void(const std::string& url, int port,
                                 bool& shouldInit)>& cb) override;
    void RegisterDebuggerShouldContinueWaitingHandler(
        const std::function<void(const std::string& url, int port,
                                 bool& shouldWait)>& cb) override;

    void CallHandler(const std::string& handler, void* param) override;

    void SetUserAgentString(const std::string& userAgent) override;
    std::string GetUserAgentString() override;
    void SetCacheMode(int mode) override;
    int GetCacheMode() override;
    void SetDefaultFontSize(uint32_t size) override;
    uint32_t GetDefaultFontSize() override;

    void DispatchMouseMoveEvent(::LWE::MouseButtonValue button,
                                ::LWE::MouseButtonsValue buttons, double x,
                                double y) override;
    void DispatchMouseDownEvent(::LWE::MouseButtonValue button,
                                ::LWE::MouseButtonsValue buttons, double x,
                                double y) override;
    void DispatchMouseUpEvent(::LWE::MouseButtonValue button,
                              ::LWE::MouseButtonsValue buttons, double x,
                              double y) override;
    void DispatchMouseWheelEvent(double x, double y, int delta) override;
    void DispatchKeyDownEvent(::LWE::KeyValue keyCode) override;
    void DispatchKeyPressEvent(::LWE::KeyValue keyCode) override;
    void DispatchKeyUpEvent(::LWE::KeyValue keyCode) override;

    void DispatchCompositionStartEvent(
        const std::string& soFarCompositiedString) override;
    void DispatchCompositionUpdateEvent(
        const std::string& soFarCompositiedString) override;
    void DispatchCompositionEndEvent(
        const std::string& soFarCompositiedString) override;
    void RegisterOnShowSoftwareKeyboardIfPossibleHandler(
        const std::function<void(WebContainer*)>& cb) override;
    void RegisterOnHideSoftwareKeyboardIfPossibleHandler(
        const std::function<void(WebContainer*)>& cb) override;

    void SetUserData(const std::string& key, void* data) override;
    void* GetUserData(const std::string& key) override;

    std::string GetTitle() override;
    void ScrollTo(int x, int y) override;
    void ScrollBy(int x, int y) override;
    int GetScrollX() override;
    int GetScrollY() override;

    size_t Width() override;
    size_t Height() override;

    void RegisterSetNeedsRenderingCallback(
        const std::function<void(
            WebContainer*, const std::function<void()>& doRenderingFunction)>&
            cb) override;
    void SetDevicePixelRatio(float dpr) override;
    float GetDevicePixelRatio() override;

    WebContainerImpl(void* webView);

private:
    // use Destroy function instead of using delete operator
    virtual ~WebContainerImpl()
    {
    }

    void* m_impl;
};

WebContainer* WebContainer::CreateWithBuffer(void* buffer, unsigned width,
                                             unsigned height, unsigned stride,
                                             float scaleFactor,
                                             const char* defaultFontName,
                                             const char* locale,
                                             const char* timezoneID)
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

    return reinterpret_cast<WebContainer*>(
        ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
            [&]() -> size_t {
                auto webView =
                    createWebViewInstance(width, height, scaleFactor,
                                          defaultFontName, locale, timezoneID);

                WebContainer* newWebContainer =
                    new (NoGC) WebContainerImpl(webView);
                newWebContainer->UpdateBuffer(buffer, width, height, stride);
                return (size_t)newWebContainer;
            }));
#else
    STARFISH_LOG_ERROR("Cannot use this set of function within this port!");
    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    return nullptr;
#endif
}

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

    return reinterpret_cast<WebContainer*>(
        ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
            [&]() -> size_t {
                auto webView =
                    createWebViewInstance(width, height, scaleFactor,
                                          defaultFontName, locale, timezoneID);

                WebContainer* newWebContainer =
                    new (NoGC) WebContainerImpl(webView);

                return (size_t)newWebContainer;
            }));
}

void WebContainerImpl::UpdateBuffer(void* buffer, unsigned width,
                                    unsigned height, unsigned stride)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
        [&]() -> size_t {
            ResizeTo(width, height);
            TO_WEBVIEW(m_impl)->platformWindow()->updateDrawingBufferAddress(
                buffer, stride);
            return 0;
        });
}

void WebContainerImpl::RegisterPreRenderingHandler(
    const std::function<WebContainer::RenderInfo(void)>& cb)
{
#if !defined(PORT_WINDOW_BACKEND_GB)
    STARFISH_LOG_ERROR("Cannot use this set of function within this port!");
    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
#endif
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync([&]()
                                                                    -> size_t {
        TO_WEBVIEW(m_impl)->platformWindow()->registerRenderingPrepareCallback(
            [cb](void) -> Starfish::RenderInfo {
                WebContainer::RenderInfo tmp = cb();
                Starfish::RenderInfo result;
                result.updatedBufferAddress = tmp.updatedBufferAddress;
                result.bufferStride = tmp.bufferStride;

                return result;
            });
        return 0;
    });
}

void WebContainerImpl::RegisterOnRenderedHandler(
    const std::function<void(WebContainer*, const WebContainer::RenderResult&)>&
        cb)
{
#if !defined(PORT_WINDOW_BACKEND_GB)
    STARFISH_LOG_ERROR("Cannot use this set of function within this port!");
    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
#endif
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync([&]()
                                                                    -> size_t {
        TO_WEBVIEW(m_impl)->platformWindow()->registerRenderingFinishedCallback(
            [this, cb](const Starfish::RenderResult& renderResult) {
                WebContainer::RenderResult result;
                result.updatedX = (int)renderResult.updateRect.x();
                result.updatedY = (int)renderResult.updateRect.y();
                result.updatedWidth = (int)renderResult.updateRect.width();
                result.updatedHeight = (int)renderResult.updateRect.height();
                result.updatedBufferAddress = TO_WEBVIEW(m_impl)
                                                  ->platformWindow()
                                                  ->drawingBufferAddress();
                result.bufferImageWidth =
                    TO_WEBVIEW(m_impl)->platformWindow()->width();
                result.bufferImageHeight =
                    TO_WEBVIEW(m_impl)->platformWindow()->height();
                cb(this, result);
            });
        return 0;
    });
}

WebContainer* WebContainer::CreateGL(unsigned width, unsigned height,
                                     const OnGLMakeCurrent& onGLMakeCurrent,
                                     const OnGLSwapBuffers& onGLSwapBuffers,
                                     float devicePixelRatio,
                                     const char* defaultFontName,
                                     const char* locale, const char* timezoneID)
{
#if !defined(PORT_WINDOW_BACKEND_GL)
    STARFISH_LOG_ERROR("Cannot use this set of function within this port!");
    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
#endif
    return reinterpret_cast<WebContainer*>(
        ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
            [=]() -> size_t {
                auto webView =
                    createWebViewInstance(width, height, devicePixelRatio,
                                          defaultFontName, locale, timezoneID);

                WebContainer* newWebContainer =
                    new (NoGC) WebContainerImpl(webView);

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
            }));
}

#ifdef STARFISH_FLUTTER
#include <tbm_surface.h>
#endif

WebContainer* WebContainer::CreateWithPlatformImage(
    unsigned width, unsigned height, const OnPrepareImage& prepareImageCb,
    const OnFlush& flushCb, float devicePixelRatio, const char* defaultFontName,
    const char* locale, const char* timezoneID)
{
    return reinterpret_cast<WebContainer*>(
        ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
            [=]() -> size_t {
                auto webView =
                    createWebViewInstance(width, height, devicePixelRatio,
                                          defaultFontName, locale, timezoneID);

                WebContainer* newWebContainer =
                    new (NoGC) WebContainerImpl(webView);
                webView->platformWindow()->registerRenderingPrepareCallback(
                    [prepareImageCb](void) -> Starfish::RenderInfo {
                        WebContainer::ExternalImageInfo buffer =
                            prepareImageCb();
                        Starfish::RenderInfo result;
#ifdef STARFISH_FLUTTER
                        tbm_surface_info_s tbmSurfaceInfo;
                        if (tbm_surface_map((tbm_surface_h)buffer.imageAddress,
                                            TBM_SURF_OPTION_WRITE,
                                            &tbmSurfaceInfo) ==
                            TBM_SURFACE_ERROR_NONE) {
                            result.updatedBufferAddress =
                                tbmSurfaceInfo.planes[0].ptr;
                            result.bufferStride =
                                tbmSurfaceInfo.planes[0].stride;
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
            }));
}

WebContainer* WebContainer::CreateGLWithPlatformImage(
    unsigned width, unsigned height, const OnGLMakeCurrent& onGLMakeCurrent,
    const OnGLSwapBuffers& onGLSwapBuffers,
    const OnPrepareImage& prepareImageCb, const OnFlush& flushCb,
    float devicePixelRatio, const char* defaultFontName, const char* locale,
    const char* timezoneID)
{
#if !defined(PORT_WINDOW_BACKEND_GL)
    STARFISH_LOG_ERROR("Cannot use this set of function within this port!");
    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
#endif
    return reinterpret_cast<WebContainer*>(
        ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
            [=]() -> size_t {
                auto webView =
                    createWebViewInstance(width, height, devicePixelRatio,
                                          defaultFontName, locale, timezoneID);

                WebContainer* newWebContainer =
                    new (NoGC) WebContainerImpl(webView);

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
            }));
}

WebContainer* WebContainer::CreateHeadless(unsigned width, unsigned height,
                                           float devicePixelRatio,
                                           const char* defaultFontName,
                                           const char* locale,
                                           const char* timezoneID)
{
    auto webView = createWebViewInstance(width, height, devicePixelRatio,
                                         defaultFontName, locale, timezoneID);
    WebContainer* newWebContainer = new (NoGC) WebContainerImpl(webView);

    return newWebContainer;
}

void WebContainerImpl::ResizeTo(size_t width, size_t height)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
        [&]() -> size_t {
            TO_WEBVIEW(m_impl)->platformWindow()->resizeTo((int)width,
                                                           (int)height);
            return 0;
        });
}

WebContainerImpl::WebContainerImpl(void* impl)
    : m_impl(impl)
{
}

void WebContainerImpl::AddIdleCallback(void (*callback)(void*), void* data)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
        [&]() -> size_t {
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
            return 0;
        });
}

size_t WebContainerImpl::AddTimeout(void (*callback)(void*), void* data,
                                    size_t timeoutInMS)
{
    size_t ret = 0;
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
        [&]() -> size_t {
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
            return 0;
        });
    return ret;
}

void WebContainerImpl::ClearTimeout(size_t handle)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        static_cast<Starfish::WebView*>(m_impl)->messageLoop(),
        [=]() -> void { TO_WEBVIEW(m_impl)->timer()->removeTimer(handle); });
}

void WebContainerImpl::RegisterCanRenderingHandler(
    const std::function<bool(WebContainer*)>& cb)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
        [&]() -> size_t {
            TO_WEBVIEW(m_impl)->platformWindow()->registerCanRenderingCallback(
                [this, cb](Starfish::PlatformWindow* wnd) -> bool {
                    return cb(this);
                });
            return 0;
        });
}

Settings* WebContainerImpl::GetSettings()
{
    Settings* result = Settings::Create(USER_AGENT(STARFISH_NAME, VERSION), "");

    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
        [&]() -> size_t {
            result->SetUserAgentString(
                TO_WEBVIEW(m_impl)->userAgent()->toUTF8NonGCString());
#ifdef STARFISH_ENABLE_HTTPCACHE
            Nullable<::Starfish::HTTPCache*> cache =
                TO_STARFISH(m_impl)->httpCache();
            if (cache.hasValue()) {
                result->SetCacheMode(cache->cacheMode());
            } else {
                result->SetCacheMode(::Starfish::HTTPCache::LOAD_NO_CACHE);
            }
#endif
            result->SetProxyURL(TO_WEBVIEW(m_impl)->proxyURL());
#ifdef STARFISH_ENABLE_TTS
            result->SetTTSMode(TO_WEBVIEW(m_impl)->tts()->mode());
            result->SetTTSLanguage(TO_WEBVIEW(m_impl)->tts()->userLanguage());
#endif
            result->SetWebSecurityMode(
                TO_WEBVIEW(m_impl)->getWebSecurityMode());
            result->SetIdleModeJob(TO_WEBVIEW(m_impl)->idleModeJob());
            result->SetIdleModeCheckIntervalInMS(
                TO_WEBVIEW(m_impl)->idleModeCheckIntervalInMS());
            result->SetNeedsDownloadWebFontsEarly(
                TO_WEBVIEW(m_impl)->needsDownloadWebFontsEarly());
            result->SetNeedsDownScaleImageResourceLargerThan(
                TO_WEBVIEW(m_impl)->needsDownScaleImageResourceLargerThan());
#ifndef TIZEN_COMPAT_HEADER_5_0
            result->SetScrollbarVisible(TO_WEBVIEW(m_impl)->scrollbarVisible());
#endif
            result->SetUseExternalPopup(TO_WEBVIEW(m_impl)->useExternalPopup());
            result->SetUseSpatialNavigation(
                TO_WEBVIEW(m_impl)->useSpatialNavigation());
            return 0;
        });
    return result;
}

void WebContainerImpl::LoadURL(const std::string& url)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        static_cast<Starfish::WebView*>(m_impl)->messageLoop(), [=]() -> void {
            TO_WEBVIEW(m_impl)->loadHTMLDocument(
                Starfish::String::fromUTF8(url.data(), url.size()));
        });
}

std::string WebContainerImpl::GetURL()
{
    std::string ret;
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
        [&]() -> size_t {
            ret = TO_LOCATION(m_impl)->url()->urlString()->toUTF8NonGCString();
            return 0;
        });
    return ret;
}

void WebContainerImpl::LoadData(const std::string& data)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        static_cast<Starfish::WebView*>(m_impl)->messageLoop(), [=]() -> void {
            if (data.size() > 0) {
                auto dataURI =
                    Starfish::Base64Utils::encodeBase64HTMLDataURI(data);
                TO_WEBVIEW(m_impl)->loadHTMLDocument(
                    Starfish::String::fromUTF8(dataURI.data(), dataURI.size()));
            } else {
                TO_WEBVIEW(m_impl)->loadHTMLDocument(
                    Starfish::String::fromUTF8("about:blank"));
            }
        });
}

void WebContainerImpl::Reload()
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        static_cast<Starfish::WebView*>(m_impl)->messageLoop(),
        [=]() -> void { TO_LOCATION(m_impl)->reload(); });
}

void WebContainerImpl::StopLoading()
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        static_cast<Starfish::WebView*>(m_impl)->messageLoop(), [=]() -> void {
            STARFISH_ASSERT(m_impl);
            TO_RESOURCE_LOADER(m_impl).clear();
        });
}

void WebContainerImpl::GoBack()
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        static_cast<Starfish::WebView*>(m_impl)->messageLoop(),
        [=]() -> void { TO_HISTORY(m_impl)->back(); });
}

void WebContainerImpl::GoForward()
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        static_cast<Starfish::WebView*>(m_impl)->messageLoop(),
        [=]() -> void { TO_HISTORY(m_impl)->forward(); });
}

bool WebContainerImpl::CanGoBack()
{
    bool ret = false;
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
        [&]() -> size_t {
            ret = TO_HISTORY(m_impl)->canGoBack();
            return 0;
        });
    return ret;
}

bool WebContainerImpl::CanGoForward()
{
    bool ret = false;
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
        [&]() -> size_t {
            ret = TO_HISTORY(m_impl)->canGoForward();
            return 0;
        });
    return ret;
}

void WebContainerImpl::AddJavaScriptInterface(
    const std::string& exposedObjectName, const std::string& jsFunctionName,
    std::function<std::string(const std::string&)> cb)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        static_cast<Starfish::WebView*>(m_impl)->messageLoop(), [=]() -> void {
            Starfish::String* objectName = Starfish::String::fromUTF8(
                exposedObjectName.data(), exposedObjectName.size());
            Starfish::String* functionName = Starfish::String::fromUTF8(
                jsFunctionName.data(), jsFunctionName.size());

            TO_WEBVIEW(m_impl)->addJavaScriptNativeInterface(
                objectName, functionName,
                new Starfish::JavaScriptNativeHandler(TO_WEBVIEW(m_impl),
                                                      functionName, cb),
                nativeCallbackFunction);

            if (TO_WEBVIEW(m_impl)->mainBrowsingContext()) {
                Starfish::registerJavaScriptNativeInterface(
                    TO_SCRIPT_BINDING_INSTANCE(m_impl), objectName,
                    functionName,
                    new Starfish::JavaScriptNativeHandler(TO_WEBVIEW(m_impl),
                                                          functionName, cb),
                    nativeCallbackFunction);
            }
        });
}

std::string WebContainerImpl::EvaluateJavaScript(const std::string& script)
{
    std::string ret;

    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
        [&]() -> size_t {
            ret = TO_WEBVIEW(m_impl)
                      ->evaluateJavaScript(Starfish::String::fromUTF8(
                          script.data(), script.size()))
                      ->toUTF8NonGCString();
            return 0;
        });

    return ret;
}

void WebContainerImpl::EvaluateJavaScript(
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

void WebContainerImpl::ClearHistory()
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        static_cast<Starfish::WebView*>(m_impl)->messageLoop(),
        [=]() -> void { TO_WEBVIEW(m_impl)->historyManager()->clear(); });
}

void WebContainerImpl::Destroy()
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
        [&]() -> size_t {
            TO_WEBVIEW(m_impl)->destroy();
            m_impl = nullptr;

            GC_FREE(this);
            return 0;
        });
}

void WebContainerImpl::Resume()
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
        [&]() -> size_t {
            TO_WEBVIEW(m_impl)->platformWindow()->resume();
            return 0;
        });
}

void WebContainerImpl::Pause()
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
        [&]() -> size_t {
            TO_WEBVIEW(m_impl)->platformWindow()->pause();
            return 0;
        });
}

void WebContainerImpl::Focus()
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        static_cast<Starfish::WebView*>(m_impl)->messageLoop(), [=]() -> void {
            // TODO
            STARFISH_UNIMPLEMENTED();
        });
}

void WebContainerImpl::Blur()
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        static_cast<Starfish::WebView*>(m_impl)->messageLoop(), [=]() -> void {
            TO_WEBVIEW(m_impl)->platformWindow()->webView()->blur();
        });
}

void WebContainerImpl::SetSettings(const Settings* settings)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        static_cast<Starfish::WebView*>(m_impl)->messageLoop(), [=]() -> void {
            TO_WEBVIEW(m_impl)->setCustomUserAgentString(
                Starfish::String::fromUTF8(
                    settings->GetUserAgentString().data(),
                    settings->GetUserAgentString().size()));
            TO_WEBVIEW(m_impl)->setProxyURL(settings->GetProxyURL());
#ifdef STARFISH_ENABLE_TTS
            TO_WEBVIEW(m_impl)->tts()->setMode(settings->GetTTSMode());
            TO_WEBVIEW(m_impl)->tts()->setUserLanguage(
                settings->GetTTSLanguage());
#endif
            unsigned char r, g, b, a;
            settings->GetBaseBackgroundColor(r, g, b, a);
            TO_WEBVIEW(m_impl)->setBaseBackgroundColor(
                Starfish::Unit::Color(r, g, b, a));
            settings->GetBaseForegroundColor(r, g, b, a);
            TO_WEBVIEW(m_impl)->setBaseForegroundColor(
                Starfish::Unit::Color(r, g, b, a));
#ifdef STARFISH_ENABLE_HTTPCACHE
            Nullable<::Starfish::HTTPCache*> cache =
                TO_STARFISH(m_impl)->httpCache();
            if (cache.hasValue()) {
                cache->setCacheMode(settings->GetCacheMode());
            } else {
                STARFISH_LOG_ERROR(
                    "Http Cache could not initialized. So Changing cache mode "
                    "is no "
                    "effect.. ");
            }
#endif
            TO_WEBVIEW(m_impl)->setWebSecurityMode(
                settings->GetWebSecurityMode());
            TO_WEBVIEW(m_impl)->setIdleModeJob(settings->GetIdleModeJob());
            TO_WEBVIEW(m_impl)->setIdleModeCheckIntervalInMS(
                settings->GetIdleModeCheckIntervalInMS());
            TO_WEBVIEW(m_impl)->setNeedsDownloadWebFontsEarly(
                settings->NeedsDownloadWebFontsEarly());
            TO_WEBVIEW(m_impl)->setUseHttp2(settings->UseHttp2());
            TO_WEBVIEW(m_impl)->setNeedsDownScaleImageResourceLargerThan(
                settings->NeedsDownScaleImageResourceLargerThan());
#ifndef TIZEN_COMPAT_HEADER_5_0
            TO_WEBVIEW(m_impl)->setScrollbarVisible(
                settings->ScrollbarVisible());
#endif
            TO_WEBVIEW(m_impl)->setUseExternalPopup(
                settings->UseExternalPopup());
            TO_WEBVIEW(m_impl)->setUseSpatialNavigation(
                settings->UseSpatialNavigation());

            delete settings;
        });
}

void WebContainerImpl::RemoveJavascriptInterface(
    const std::string& exposedObjectName, const std::string& jsFunctionName)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        static_cast<Starfish::WebView*>(m_impl)->messageLoop(), [=]() -> void {
            Starfish::String* objectName = Starfish::String::fromUTF8(
                exposedObjectName.data(), exposedObjectName.size());
            Starfish::String* functionName = Starfish::String::fromUTF8(
                jsFunctionName.data(), jsFunctionName.size());

            TO_WEBVIEW(m_impl)->removeJavaScriptNativeInterface(objectName,
                                                                functionName);

            if (!jsFunctionName.empty()) {
                Starfish::unregisterJavaScriptNativeInterface(
                    TO_SCRIPT_BINDING_INSTANCE(m_impl), objectName,
                    functionName);
            } else {
                Starfish::unregisterJavaScriptNativeInterface(
                    TO_SCRIPT_BINDING_INSTANCE(m_impl), objectName);
            }
        });
}
void WebContainerImpl::ClearCache()
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        static_cast<Starfish::WebView*>(m_impl)->messageLoop(), [=]() -> void {
#ifdef STARFISH_ENABLE_HTTPCACHE
            Nullable<::Starfish::HTTPCache*> cache =
                TO_STARFISH(m_impl)->httpCache();
            if (cache.hasValue()) {
                cache->clear();
            }
#endif
        });
}

void WebContainerImpl::RegisterOnReceivedErrorHandler(
    const std::function<void(WebContainer*, ResourceError*)>& cb)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        static_cast<Starfish::WebView*>(m_impl)->messageLoop(), [=]() -> void {
            TO_WEBVIEW(m_impl)->registerPublicWebViewHandler(
                Starfish::OnReceivedError, [this, cb](void* param) -> void {
                    struct Param {
                        Starfish::RequestErrorType errorCode;
                        Starfish::String* url;
                    };
                    Param* p = (Param*)param;
                    ResourceError* error = ResourceError::Create(
                        convertErrorCode(p->errorCode),
                        convertErrorDescriton(p->errorCode),
                        p->url->toUTF8NonGCString());
                    cb(this, error);
                    delete error;
                });
        });
}

void WebContainerImpl::RegisterOnPageParsedHandler(
    std::function<void(WebContainer*, const std::string&)> cb)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        static_cast<Starfish::WebView*>(m_impl)->messageLoop(), [=]() -> void {
            TO_WEBVIEW(m_impl)->registerPublicWebViewHandler(
                Starfish::OnPageParsed, [this, cb](void* param) -> void {
                    struct Param {
                        Starfish::String* url;
                    };
                    Param* p = (Param*)param;
                    cb(this, p->url->toUTF8NonGCString());
                });
        });
}

void WebContainerImpl::RegisterOnPageLoadedHandler(
    std::function<void(WebContainer*, const std::string&)> cb)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        static_cast<Starfish::WebView*>(m_impl)->messageLoop(), [=]() -> void {
            TO_WEBVIEW(m_impl)->registerPublicWebViewHandler(
                Starfish::OnPageLoaded, [this, cb](void* param) -> void {
                    struct Param {
                        Starfish::String* url;
                    };
                    Param* p = (Param*)param;
                    cb(this, p->url->toUTF8NonGCString());
                });
        });
}

void WebContainerImpl::RegisterOnPageStartedHandler(
    const std::function<void(WebContainer*, const std::string&)>& cb)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        static_cast<Starfish::WebView*>(m_impl)->messageLoop(), [=]() -> void {
            TO_WEBVIEW(m_impl)->registerPublicWebViewHandler(
                Starfish::OnPageStarted, [this, cb](void* param) -> void {
                    struct Param {
                        Starfish::String* url;
                    };
                    Param* p = (Param*)param;
                    cb(this, p->url->toUTF8NonGCString());
                });
        });
}

void WebContainerImpl::RegisterOnLoadResourceHandler(
    const std::function<void(WebContainer*, const std::string&)>& cb)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        static_cast<Starfish::WebView*>(m_impl)->messageLoop(), [=]() -> void {
            TO_WEBVIEW(m_impl)->registerPublicWebViewHandler(
                Starfish::OnLoadResource, [this, cb](void* param) -> void {
                    struct Param {
                        Starfish::String* url;
                    };
                    Param* p = (Param*)param;
                    cb(this, p->url->toUTF8NonGCString());
                });
        });
}

void WebContainerImpl::RegisterShouldOverrideUrlLoadingHandler(
    const std::function<bool(WebContainer*, const std::string&)>& cb)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        static_cast<Starfish::WebView*>(m_impl)->messageLoop(), [=]() -> void {
            TO_WEBVIEW(m_impl)->registerPublicWebViewHandler(
                Starfish::ShouldOverrideUrlLoading,
                [this, cb](void* param) -> void {
                    struct Param {
                        Starfish::ResourceURL* url;
                        Starfish::ReferrerURL* referrerURL;
                        bool canNavigate;
                        bool force;
                    };
                    Param* p = (Param*)param;
                    bool ret = cb(
                        this, p->url->urlString()->toUTF8NonGCString().data());
                    if ((ret == false) && p->canNavigate) {
                        // continue loading
                        TO_WEBVIEW(m_impl)->navigateAsync(
                            p->url, Starfish::HistoryManagerAction::Add,
                            p->referrerURL);
                    }
                });
        });
}

void WebContainerImpl::RegisterOnDownloadStartHandler(
    const std::function<void(WebContainer*, const std::string&,
                             const std::string&, const std::string&,
                             const std::string&, long)>& cb)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        static_cast<Starfish::WebView*>(m_impl)->messageLoop(), [=]() -> void {
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
                        "Http Cache could not initialized. So Changing cache "
                        "mode is no effect.. ");
                    Param* p = (Param*)param;
                    cb(this, p->url, p->userAgent, p->contentDisposition,
                       p->mimetype, p->contentLength);
                    delete p;
                });
        });
}

void WebContainerImpl::RegisterShowDropdownMenuHandler(
    const std::function<void(WebContainer*, const std::vector<std::string>*,
                             int)>& cb)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        static_cast<Starfish::WebView*>(m_impl)->messageLoop(), [=]() -> void {
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
        });
}

void WebContainerImpl::RegisterShowAlertHandler(
    const std::function<void(WebContainer*, const std::string&,
                             const std::string&)>& cb)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        static_cast<Starfish::WebView*>(m_impl)->messageLoop(), [=]() -> void {
            TO_WEBVIEW(m_impl)->platformWindow()->registerCallbackHandler(
                Starfish::WindowHandlerShowAlert,
                [this, cb](void* param) -> void {
                    struct Param {
                        std::string title;
                        std::string message;
                    };

                    Param* p = (Param*)param;
                    cb(this, p->title, p->message);
                    delete p;
                });
        });
}

void WebContainerImpl::RegisterCustomFileResourceRequestHandlers(
    std::function<const char*(const char* path)> resolveFilePathCallback,
    std::function<void*(const char* path)> fileOpenCallback,
    std::function<size_t(uint8_t* destBuffer, size_t size, void* handle)>
        fileReadCallback,
    std::function<long int(void* handle)> fileLengthCallback,
    std::function<void(void* handle)> fileCloseCallback)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        static_cast<Starfish::WebView*>(m_impl)->messageLoop(), [=]() -> void {
            TO_WEBVIEW(m_impl)->registerCustomFileResourceRequestCallbacks(
                resolveFilePathCallback, fileOpenCallback, fileReadCallback,
                fileLengthCallback, fileCloseCallback);
        });
}

void WebContainerImpl::CallHandler(const std::string& handler, void* param)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        static_cast<Starfish::WebView*>(m_impl)->messageLoop(), [=]() -> void {
            if (handler.compare("onDropdownMenuItemSelected") == 0) {
                TO_WEBVIEW(m_impl)->platformWindow()->callHandler(
                    Starfish::WindowHandlerOnDropdownMenuItemSelected, param);
            }
        });
}

size_t WebContainerImpl::Width()
{
    size_t ret = 0;
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
        [&]() -> size_t {
            ret = TO_WEBVIEW(m_impl)->platformWindow()->width();
            return 0;
        });

    return ret;
}

size_t WebContainerImpl::Height()
{
    size_t ret = 0;
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
        [&]() -> size_t {
            ret = TO_WEBVIEW(m_impl)->platformWindow()->height();
            return 0;
        });
    return ret;
}

void WebContainerImpl::RegisterOnProgressChangedHandler(
    const std::function<void(WebContainer*, int)>& cb)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        static_cast<Starfish::WebView*>(m_impl)->messageLoop(), [=]() -> void {
            TO_WEBVIEW(m_impl)->registerPublicWebViewHandler(
                Starfish::OnProgressChanged, [this, cb](void* param) -> void {
                    struct Param {
                        int newProgress;
                    };
                    Param* p = (Param*)param;
                    cb(this, p->newProgress);
                });
        });
}

void WebContainerImpl::RegisterDebuggerShouldInitHandler(
    const std::function<void(const std::string& url, int port, bool& ret)>& cb)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        static_cast<Starfish::WebView*>(m_impl)->messageLoop(), [=]() -> void {
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
        });
}

void WebContainerImpl::RegisterDebuggerShouldContinueWaitingHandler(
    const std::function<void(const std::string& url, int port, bool& ret)>& cb)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        static_cast<Starfish::WebView*>(m_impl)->messageLoop(), [=]() -> void {
            TO_WEBVIEW(m_impl)->registerPublicWebViewHandler(
                Starfish::DebuggerShouldContinueWaiting,
                [cb](void* param) -> void {
                    struct Param {
                        std::string url;
                        int port;
                        bool* ret;
                    };
                    Param* p = (Param*)param;
                    cb(p->url, p->port, *p->ret);
                });
        });
}

void WebContainerImpl::SetUserAgentString(const std::string& userAgent)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
        [&]() -> size_t {
            TO_WEBVIEW(m_impl)->setCustomUserAgentString(
                Starfish::String::fromUTF8(userAgent.data(), userAgent.size()));
            return 0;
        });
}

std::string WebContainerImpl::GetUserAgentString()
{
    std::string ret;
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
        [&]() -> size_t {
            ret = TO_WEBVIEW(m_impl)->userAgent()->toUTF8NonGCString();
            return 0;
        });
    return ret;
}

void WebContainerImpl::SetCacheMode(int mode)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
        [&]() -> size_t {
#ifdef STARFISH_ENABLE_HTTPCACHE
            Nullable<::Starfish::HTTPCache*> cache =
                TO_STARFISH(m_impl)->httpCache();
            if (cache.hasValue()) {
                cache->setCacheMode(mode);
            }
#endif
            return 0;
        });
}

int WebContainerImpl::GetCacheMode()
{
    int ret = 0;
#ifdef STARFISH_ENABLE_HTTPCACHE
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
        [&]() -> size_t {
            Nullable<::Starfish::HTTPCache*> cache =
                TO_STARFISH(m_impl)->httpCache();
            if (cache.hasValue()) {
                ret = cache->cacheMode();
            }
            return 0;
        });
#endif
    return ret;
}

void WebContainerImpl::SetDefaultFontSize(uint32_t size)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
        [&]() -> size_t {
            if (LWE_MIN_FONT_SIZE <= size && size <= LWE_MAX_FONT_SIZE) {
                TO_WEBVIEW(m_impl)->setDefaultFontSize(size);
            }
            return 0;
        });
}

uint32_t WebContainerImpl::GetDefaultFontSize()
{
    uint32_t ret = 0;
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
        [&]() -> size_t {
            ret = TO_WEBVIEW(m_impl)->defaultFontSize();
            return 0;
        });
    return ret;
}

void WebContainerImpl::DispatchMouseMoveEvent(MouseButtonValue button,
                                              MouseButtonsValue buttons,
                                              double x, double y)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        static_cast<Starfish::WebView*>(m_impl)->messageLoop(), [=]() -> void {
            TO_WEBVIEW(m_impl)->platformWindow()->dispatchMouseEvent(
                ::Starfish::MouseEventKind::MouseEventMove,
                ::Starfish::MouseData(button, buttons, x, y, 0,
                                      Starfish::timestamp()));
        });
}

void WebContainerImpl::DispatchMouseDownEvent(MouseButtonValue button,
                                              MouseButtonsValue buttons,
                                              double x, double y)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        static_cast<Starfish::WebView*>(m_impl)->messageLoop(), [=]() -> void {
            TO_WEBVIEW(m_impl)->platformWindow()->dispatchMouseEvent(
                ::Starfish::MouseEventKind::MouseEventDown,
                ::Starfish::MouseData(button, buttons, x, y, 0,
                                      Starfish::timestamp()));
        });
}

void WebContainerImpl::DispatchMouseUpEvent(MouseButtonValue button,
                                            MouseButtonsValue buttons, double x,
                                            double y)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        static_cast<Starfish::WebView*>(m_impl)->messageLoop(), [=]() -> void {
            TO_WEBVIEW(m_impl)->platformWindow()->dispatchMouseEvent(
                ::Starfish::MouseEventKind::MouseEventUp,
                ::Starfish::MouseData(button, buttons, x, y, 0,
                                      Starfish::timestamp()));
        });
}

void WebContainerImpl::DispatchMouseWheelEvent(double x, double y, int delta)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        static_cast<Starfish::WebView*>(m_impl)->messageLoop(), [=]() -> void {
            TO_WEBVIEW(m_impl)->platformWindow()->dispatchMouseWheelEvent(
                x, y, delta, true);
        });
}

void WebContainerImpl::DispatchKeyDownEvent(KeyValue keyCode)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        static_cast<Starfish::WebView*>(m_impl)->messageLoop(), [=]() -> void {
            TO_WEBVIEW(m_impl)->platformWindow()->dispatchKeyEvent(
                ::Starfish::KeyEventKind::KeyEventDown,
                ::Starfish::PlatformKeyEventData(keyCode));
        });
}

void WebContainerImpl::DispatchKeyPressEvent(KeyValue keyCode)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        static_cast<Starfish::WebView*>(m_impl)->messageLoop(), [=]() -> void {
            TO_WEBVIEW(m_impl)->platformWindow()->dispatchKeyEvent(
                ::Starfish::KeyEventKind::KeyEventPress,
                ::Starfish::PlatformKeyEventData(keyCode));
        });
}

void WebContainerImpl::DispatchKeyUpEvent(KeyValue keyCode)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        static_cast<Starfish::WebView*>(m_impl)->messageLoop(), [=]() -> void {
            TO_WEBVIEW(m_impl)->platformWindow()->dispatchKeyEvent(
                ::Starfish::KeyEventKind::KeyEventUp,
                ::Starfish::PlatformKeyEventData(keyCode));
        });
}

void WebContainerImpl::DispatchCompositionStartEvent(
    const std::string& soFarCompositiedString)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        static_cast<Starfish::WebView*>(m_impl)->messageLoop(), [=]() -> void {
            TO_WEBVIEW(m_impl)->platformWindow()->dispatchCompositionEvent(
                ::Starfish::CompositionEventKind::CompositionEventStart,
                ::Starfish::String::fromUTF8(soFarCompositiedString.data(),
                                             soFarCompositiedString.length()),
                nullptr);
        });
}

void WebContainerImpl::DispatchCompositionUpdateEvent(
    const std::string& soFarCompositiedString)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        static_cast<Starfish::WebView*>(m_impl)->messageLoop(), [=]() -> void {
            TO_WEBVIEW(m_impl)->platformWindow()->dispatchCompositionEvent(
                ::Starfish::CompositionEventKind::CompositionEventUpdate,
                ::Starfish::String::fromUTF8(soFarCompositiedString.data(),
                                             soFarCompositiedString.length()),
                nullptr);
        });
}

void WebContainerImpl::DispatchCompositionEndEvent(
    const std::string& soFarCompositiedString)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        static_cast<Starfish::WebView*>(m_impl)->messageLoop(), [=]() -> void {
            TO_WEBVIEW(m_impl)->platformWindow()->dispatchCompositionEvent(
                ::Starfish::CompositionEventKind::CompositionEventEnd,
                ::Starfish::String::fromUTF8(soFarCompositiedString.data(),
                                             soFarCompositiedString.length()),
                nullptr);
        });
}
void WebContainerImpl::RegisterOnShowSoftwareKeyboardIfPossibleHandler(
    const std::function<void(WebContainer*)>& cb)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        static_cast<Starfish::WebView*>(m_impl)->messageLoop(), [=]() -> void {
            TO_WEBVIEW(m_impl)
                ->platformWindow()
                ->registerShowSoftwareKeyboardIfPossibleCallback(
                    [this, cb]() { cb(this); });
        });
}

void WebContainerImpl::RegisterOnHideSoftwareKeyboardIfPossibleHandler(
    const std::function<void(WebContainer*)>& cb)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        static_cast<Starfish::WebView*>(m_impl)->messageLoop(), [=]() -> void {
            TO_WEBVIEW(m_impl)
                ->platformWindow()
                ->registerHideSoftwareKeyboardIfPossibleCallback(
                    [this, cb]() { cb(this); });
        });
}

void WebContainerImpl::RegisterSetNeedsRenderingCallback(
    const std::function<void(
        WebContainer*, const std::function<void()>& doRenderingFunction)>& cb)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        static_cast<Starfish::WebView*>(m_impl)->messageLoop(), [=]() -> void {
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
        });
}

void WebContainerImpl::SetUserData(const std::string& key, void* data)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        static_cast<Starfish::WebView*>(m_impl)->messageLoop(), [=]() -> void {
            TO_WEBVIEW(m_impl)->publicLayerUserDataMap()[key] = data;
        });
}

void* WebContainerImpl::GetUserData(const std::string& key)
{
    void* ret = nullptr;

    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
        [&]() -> size_t {
            ret = TO_WEBVIEW(m_impl)->publicLayerUserDataMap()[key];
            return 0;
        });

    return ret;
}

std::string WebContainerImpl::GetTitle()
{
    Starfish::String* ret = Starfish::String::emptyString;
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
        [&]() -> size_t {
            ret =
                TO_WEBVIEW(m_impl)->mainBrowsingContext()->document()->title();
            return 0;
        });
    return ret->toUTF8NonGCString();
}

void WebContainerImpl::ScrollTo(int x, int y)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
        [&]() -> size_t {
            TO_WEBVIEW(m_impl)->mainBrowsingContext()->window()->scrollTo(
                (double)x, (double)y);
            return 0;
        });
}

void WebContainerImpl::ScrollBy(int x, int y)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
        [&]() -> size_t {
            TO_WEBVIEW(m_impl)->mainBrowsingContext()->window()->scrollBy(
                (double)x, (double)y);
            return 0;
        });
}

int WebContainerImpl::GetScrollX()
{
    int x = 0;
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync([&]()
                                                                    -> size_t {
        x = (int)TO_WEBVIEW(m_impl)->mainBrowsingContext()->window()->scrollX();
        return 0;
    });
    return x;
}

int WebContainerImpl::GetScrollY()
{
    int y = 0;
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync([&]()
                                                                    -> size_t {
        y = (int)TO_WEBVIEW(m_impl)->mainBrowsingContext()->window()->scrollY();
        return 0;
    });
    return y;
}

void WebContainerImpl::SetDevicePixelRatio(float dpr)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
        [&]() -> size_t {
            TO_WEBVIEW(m_impl)->platformWindow()->setDevicePixelRatio(dpr);
            return 0;
        });
}

float WebContainerImpl::GetDevicePixelRatio()
{
    float dpr = 0;
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
        [&]() -> size_t {
            dpr = TO_WEBVIEW(m_impl)->platformWindow()->getDevicePixelRatio();
            return 0;
        });
    return dpr;
}

} // namespace LWEDelegate

extern "C" {

uintptr_t LWEDelegate_WebContainer_Create(unsigned width, unsigned height,
                                          float devicePixelRatio,
                                          const char* defaultFontName,
                                          const char* locale,
                                          const char* timezoneID)
{
    return reinterpret_cast<uintptr_t>(LWEDelegate::WebContainer::Create(
        width, height, devicePixelRatio, defaultFontName, locale, timezoneID));
}

uintptr_t LWEDelegate_WebContainer_CreateWithBuffer(
    void* buffer, unsigned bufferWidth, unsigned bufferHeight,
    unsigned bufferStride, float devicePixelRatio, const char* defaultFontName,
    const char* locale, const char* timezoneID)
{
    return reinterpret_cast<uintptr_t>(
        LWEDelegate::WebContainer::CreateWithBuffer(
            buffer, bufferWidth, bufferHeight, bufferStride, devicePixelRatio,
            defaultFontName, locale, timezoneID));
}

uintptr_t LWEDelegate_WebContainer_Create_With_PlatformImage(
    unsigned width, unsigned height, uintptr_t prepareImageCb,
    uintptr_t flushCb, float devicePixelRatio, const char* defaultFontName,
    const char* locale, const char* timezoneID)
{
    auto* onPrepareImagePtr =
        reinterpret_cast<const LWEDelegate::WebContainer::OnPrepareImage*>(
            prepareImageCb);
    auto* onFlushPtr =
        reinterpret_cast<const LWEDelegate::WebContainer::OnFlush*>(flushCb);

    return reinterpret_cast<uintptr_t>(
        LWEDelegate::WebContainer::CreateWithPlatformImage(
            width, height, *onPrepareImagePtr, *onFlushPtr, devicePixelRatio,
            defaultFontName, locale, timezoneID));
}

uintptr_t LWEDelegate_WebContainer_CreateGL(
    unsigned width, unsigned height, uintptr_t onGLMakeCurrent,
    uintptr_t onGLSwapBuffers, float devicePixelRatio,
    const char* defaultFontName, const char* locale, const char* timezoneID)
{
    auto* onGLMakeCurrentPtr =
        reinterpret_cast<const LWEDelegate::WebContainer::OnGLMakeCurrent*>(
            onGLMakeCurrent);
    auto* onGLSwapBuffersPtr =
        reinterpret_cast<const LWEDelegate::WebContainer::OnGLSwapBuffers*>(
            onGLSwapBuffers);
    return reinterpret_cast<uintptr_t>(LWEDelegate::WebContainer::CreateGL(
        width, height, *onGLMakeCurrentPtr, *onGLSwapBuffersPtr,
        devicePixelRatio, defaultFontName, locale, timezoneID));
}

uintptr_t LWEDelegate_WebContainer_CreateGLWithPlatformImage(
    unsigned width, unsigned height, uintptr_t onGLMakeCurrent,
    uintptr_t onGLSwapBuffers, uintptr_t prepareImageCb, uintptr_t flushCb,
    float devicePixelRatio, const char* defaultFontName, const char* locale,
    const char* timezoneID)
{
    auto* onGLMakeCurrentPtr =
        reinterpret_cast<const LWEDelegate::WebContainer::OnGLMakeCurrent*>(
            onGLMakeCurrent);
    auto* onGLSwapBuffersPtr =
        reinterpret_cast<const LWEDelegate::WebContainer::OnGLSwapBuffers*>(
            onGLSwapBuffers);
    auto* onPrepareImagePtr =
        reinterpret_cast<const LWEDelegate::WebContainer::OnPrepareImage*>(
            prepareImageCb);
    auto* onFlushPtr =
        reinterpret_cast<const LWEDelegate::WebContainer::OnFlush*>(flushCb);
    return reinterpret_cast<uintptr_t>(
        LWEDelegate::WebContainer::CreateGLWithPlatformImage(
            width, height, *onGLMakeCurrentPtr, *onGLSwapBuffersPtr,
            *onPrepareImagePtr, *onFlushPtr, devicePixelRatio, defaultFontName,
            locale, timezoneID));
}
uintptr_t LWEDelegate_WebContainer_CreateHeadless(
    unsigned width, unsigned height, float devicePixelRatio,
    const char* defaultFontName, const char* locale, const char* timezoneID)
{
    return reinterpret_cast<uintptr_t>(
        LWEDelegate::WebContainer::CreateHeadless(
            width, height, devicePixelRatio, defaultFontName, locale,
            timezoneID));
}
}
