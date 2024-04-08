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

#include "core/modules/renderer/Renderer.h"
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

static Starfish::WebView* createStarfishWebViewInstance(
    unsigned width, unsigned height, float devicePixelRatio,
    const char* defaultFontName, const char* locale, const char* timezoneID)
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

    void RegisterOnIdleHandler(
        const std::function<void(WebContainer*)>& cb) override;

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

    virtual void RegisterGetScreenMatrixHandler(
        const std::function<TransformationMatrix(WebContainer*)>& cb) override;

    virtual void SetNeedsFullRepainting() override;

    WebContainerImpl(Starfish::WebView* webView);

private:
    // use Destroy function instead of using delete operator
    virtual ~WebContainerImpl()
    {
    }

    Starfish::WebView* m_webView = nullptr;
};

WebContainer* WebContainer::CreateWithBuffer(void* buffer, unsigned width,
                                             unsigned height, unsigned stride,
                                             float scaleFactor,
                                             const char* defaultFontName,
                                             const char* locale,
                                             const char* timezoneID)
{
    STARFISH_RELEASE_ASSERT(defaultFontName != nullptr);
    STARFISH_RELEASE_ASSERT(locale != nullptr);
    STARFISH_RELEASE_ASSERT(timezoneID != nullptr);

    WebContainer* newWebContainer = nullptr;
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync([&]() -> void {
        Starfish::WebView* webView = createStarfishWebViewInstance(
            width, height, scaleFactor, defaultFontName, locale, timezoneID);

        newWebContainer = new (NoGC) WebContainerImpl(webView);
        newWebContainer->UpdateBuffer(buffer, width, height, stride);
    });
    return newWebContainer;
}

WebContainer* WebContainer::Create(unsigned width, unsigned height,
                                   float scaleFactor,
                                   const char* defaultFontName,
                                   const char* locale, const char* timezoneID)
{
    STARFISH_RELEASE_ASSERT(defaultFontName != nullptr);
    STARFISH_RELEASE_ASSERT(locale != nullptr);
    STARFISH_RELEASE_ASSERT(timezoneID != nullptr);

    WebContainer* newWebContainer = nullptr;
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync([&]() -> void {
        Starfish::WebView* webView = createStarfishWebViewInstance(
            width, height, scaleFactor, defaultFontName, locale, timezoneID);

        newWebContainer = new (NoGC) WebContainerImpl(webView);
    });
    return newWebContainer;
}

void WebContainerImpl::UpdateBuffer(void* buffer, unsigned width,
                                    unsigned height, unsigned stride)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync([&]() -> void {
        ResizeTo(width, height);
        m_webView->renderer()->updateDrawingBufferAddress(buffer, stride);
    });
}

void WebContainerImpl::RegisterPreRenderingHandler(
    const std::function<WebContainer::RenderInfo(void)>& cb)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync([&]() -> void {
        m_webView->renderer()->registerRenderingPrepareCallback(
            [cb](void) -> Starfish::RenderInfo {
                WebContainer::RenderInfo tmp = cb();
                Starfish::RenderInfo result;
                result.updatedBufferAddress = tmp.updatedBufferAddress;
                result.bufferStride = tmp.bufferStride;

                return result;
            });
    });
}

void WebContainerImpl::RegisterOnRenderedHandler(
    const std::function<void(WebContainer*, const WebContainer::RenderResult&)>&
        cb)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync([&]() -> void {
        m_webView->renderer()->registerRenderingFinishedCallback(
            [this, cb](const Starfish::RenderResult& renderResult) {
                WebContainer::RenderResult result;
                result.updatedX = (int)renderResult.updateRect.x();
                result.updatedY = (int)renderResult.updateRect.y();
                result.updatedWidth = (int)renderResult.updateRect.width();
                result.updatedHeight = (int)renderResult.updateRect.height();
                result.updatedBufferAddress =
                    m_webView->renderer()->drawingBufferAddress();
                result.bufferImageWidth = m_webView->renderer()->width();
                result.bufferImageHeight = m_webView->renderer()->height();
                cb(this, result);
            });
    });
}

WebContainer* WebContainer::CreateGL(const WebContainerArguments& args,
                                     const RendererGLConfiguration& config)
{
    WebContainer* newWebContainer = nullptr;
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync([&]() -> void {
        Starfish::WebView* webView = createStarfishWebViewInstance(
            args.width, args.height, args.devicePixelRatio,
            args.defaultFontName, args.locale, args.timezoneID);

        newWebContainer = new (NoGC) WebContainerImpl(webView);

        webView->renderer()->registerGLMakeCurrentCallback(
            [config, newWebContainer](Starfish::Renderer* renderer) {
                config.onGLMakeCurrent(newWebContainer);
            });

        webView->renderer()->registerGLSwapBuffersCallback(
            [config, newWebContainer](Starfish::Renderer* renderer,
                                      bool mayNeedsSync) {
                config.onGLSwapBuffers(newWebContainer, mayNeedsSync);
            });

        if (config.onGLCreateSharedContext) {
            webView->renderer()->registerGLCreateSharedContext(
                [config,
                 newWebContainer](Starfish::Renderer* renderer) -> uintptr_t {
                    return config.onGLCreateSharedContext(newWebContainer);
                });
        }
        if (config.onGLDestroyContext) {
            webView->renderer()->registerGLDestroyContext(
                [config, newWebContainer](Starfish::Renderer* renderer,
                                          uintptr_t context) -> bool {
                    return config.onGLDestroyContext(newWebContainer, context);
                });
        }
        if (config.onGLClearCurrentContext) {
            webView->renderer()->registerGLClearCurrentContext(
                [config,
                 newWebContainer](Starfish::Renderer* renderer) -> bool {
                    return config.onGLClearCurrentContext(newWebContainer);
                });
        }
        if (config.onGLMakeCurrentWithContext) {
            webView->renderer()->registerGLMakeCurrentWithContext(
                [config, newWebContainer](Starfish::Renderer* renderer,
                                          uintptr_t context) -> bool {
                    return config.onGLMakeCurrentWithContext(newWebContainer,
                                                             context);
                });
        }
    });
    return newWebContainer;
}

#ifdef STARFISH_FLUTTER
#include <tbm_surface.h>
#endif

WebContainer* WebContainer::CreateWithPlatformImage(
    unsigned width, unsigned height, const OnPrepareImage& prepareImageCb,
    const OnFlush& flushCb, float devicePixelRatio, const char* defaultFontName,
    const char* locale, const char* timezoneID)
{
    WebContainer* newWebContainer = nullptr;
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync([&]() -> void {
        Starfish::WebView* webView =
            createStarfishWebViewInstance(width, height, devicePixelRatio,
                                          defaultFontName, locale, timezoneID);

        newWebContainer = new (NoGC) WebContainerImpl(webView);
        webView->renderer()->registerRenderingPrepareCallback(
            [prepareImageCb](void) -> Starfish::RenderInfo {
                WebContainer::ExternalImageInfo buffer = prepareImageCb();
                Starfish::RenderInfo result;
#ifdef STARFISH_FLUTTER
                tbm_surface_info_s tbmSurfaceInfo;
                if (tbm_surface_map((tbm_surface_h)buffer.imageAddress,
                                    TBM_SURF_OPTION_WRITE, &tbmSurfaceInfo) ==
                    TBM_SURFACE_ERROR_NONE) {
                    result.updatedBufferAddress = tbmSurfaceInfo.planes[0].ptr;
                    result.bufferStride = tbmSurfaceInfo.planes[0].stride;
                }
#endif
                return result;
            });

        webView->renderer()->registerRenderingFinishedCallback(
            [newWebContainer,
             flushCb](const Starfish::RenderResult& renderResult) {
                flushCb(newWebContainer, renderResult.didPaintingOrCompositing);
            });
    });
    return newWebContainer;
}

WebContainer* WebContainer::CreateGLWithPlatformImage(
    unsigned width, unsigned height, const OnGLMakeCurrent& onGLMakeCurrent,
    const OnGLSwapBuffers& onGLSwapBuffers,
    const OnPrepareImage& prepareImageCb, const OnFlush& flushCb,
    float devicePixelRatio, const char* defaultFontName, const char* locale,
    const char* timezoneID)
{
    WebContainer* newWebContainer = nullptr;
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync([&]() -> void {
        Starfish::WebView* webView =
            createStarfishWebViewInstance(width, height, devicePixelRatio,
                                          defaultFontName, locale, timezoneID);

        newWebContainer = new (NoGC) WebContainerImpl(webView);

        webView->renderer()->registerGLMakeCurrentCallback(
            [onGLMakeCurrent, newWebContainer](Starfish::Renderer* renderer) {
                onGLMakeCurrent(newWebContainer);
            });

        webView->renderer()->registerGLSwapBuffersCallback(
            [onGLSwapBuffers, newWebContainer](Starfish::Renderer* renderer,
                                               bool mayNeedsSync) {
                onGLSwapBuffers(newWebContainer, mayNeedsSync);
            });

        webView->renderer()->registerRenderingPrepareCallback(
            [prepareImageCb](void) -> Starfish::RenderInfo {
                WebContainer::ExternalImageInfo tmp = prepareImageCb();
                Starfish::RenderInfo result;
                result.updatedBufferAddress = tmp.imageAddress;
                result.bufferStride = 0;
                return result;
            });

        webView->renderer()->registerSurfaceFlushedCallback(
            [newWebContainer, flushCb](bool needsFlush) {
                flushCb(newWebContainer, needsFlush);
            });
    });
    return newWebContainer;
}

WebContainer* WebContainer::CreateHeadless(unsigned width, unsigned height,
                                           float devicePixelRatio,
                                           const char* defaultFontName,
                                           const char* locale,
                                           const char* timezoneID)
{
    Starfish::WebView* webView = createStarfishWebViewInstance(
        width, height, devicePixelRatio, defaultFontName, locale, timezoneID);
    WebContainer* newWebContainer = new (NoGC) WebContainerImpl(webView);

    return newWebContainer;
}

void WebContainerImpl::ResizeTo(size_t width, size_t height)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync([&]() -> void {
        m_webView->renderer()->resizeTo((int)width, (int)height);
    });
}

WebContainerImpl::WebContainerImpl(Starfish::WebView* webView)
    : m_webView(webView)
{
}

void WebContainerImpl::AddIdleCallback(void (*callback)(void*), void* data)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync([&]() -> void {
        struct Data : public gc {
            void (*callback)(void*);
            void* data;
        };

        Data* d = new Data();
        d->callback = callback;
        d->data = data;
        m_webView->messageLoop()->addIdler(
            nullptr,
            [](size_t, void* data) {
                Data* d = (Data*)data;
                d->callback(d->data);
            },
            d);
    });
}

size_t WebContainerImpl::AddTimeout(void (*callback)(void*), void* data,
                                    size_t timeoutInMS)
{
    size_t ret = 0;
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync([&]() -> void {
        struct Data : public gc {
            void (*callback)(void*);
            void* data;
        };

        Data* d = new Data();
        d->callback = callback;
        d->data = data;
        ret = m_webView->timer()->addTimer(
            timeoutInMS, nullptr,
            [](void* data) {
                Data* d = (Data*)data;
                d->callback(d->data);
            },
            d, false);
    });
    return ret;
}

void WebContainerImpl::ClearTimeout(size_t handle)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        m_webView->messageLoop(),
        [=]() -> void { m_webView->timer()->removeTimer(handle); });
}

void WebContainerImpl::RegisterCanRenderingHandler(
    const std::function<bool(WebContainer*)>& cb)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync([&]() -> void {
        m_webView->renderer()->registerCanRenderingCallback(
            [this, cb](Starfish::Renderer* renderer) -> bool {
                return cb(this);
            });
    });
}

Settings* WebContainerImpl::GetSettings()
{
    Settings* result = Settings::Create(USER_AGENT(STARFISH_NAME, VERSION), "");
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync([&]() -> void {
        result->SetUserAgentString(m_webView->userAgent()->toUTF8NonGCString());
#ifdef STARFISH_ENABLE_HTTPCACHE
        Nullable<::Starfish::HTTPCache*> cache =
            m_webView->starfish()->httpCache();
        if (cache.hasValue()) {
            result->SetCacheMode(cache->cacheMode());
        } else {
            result->SetCacheMode(::Starfish::HTTPCache::LOAD_NO_CACHE);
        }
#endif
        result->SetProxyURL(m_webView->proxyURL());
#ifdef STARFISH_ENABLE_TTS
        result->SetTTSMode(m_webView->tts()->mode());
        result->SetTTSLanguage(m_webView->tts()->userLanguage());
#endif
        result->SetWebSecurityMode(m_webView->getWebSecurityMode());
        result->SetIdleModeJob(m_webView->idleModeJob());
        result->SetIdleModeCheckIntervalInMS(
            m_webView->idleModeCheckIntervalInMS());
        result->SetNeedsDownloadWebFontsEarly(
            m_webView->needsDownloadWebFontsEarly());
        result->SetNeedsDownScaleImageResourceLargerThan(
            m_webView->needsDownScaleImageResourceLargerThan());
#ifndef TIZEN_COMPAT_HEADER_5_0
        result->SetScrollbarVisible(m_webView->scrollbarVisible());
#endif
        result->SetUseExternalPopup(m_webView->useExternalPopup());
        result->SetUseSpatialNavigation(m_webView->useSpatialNavigation());
    });
    return result;
}

void WebContainerImpl::LoadURL(const std::string& url)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        m_webView->messageLoop(), [=]() -> void {
            m_webView->loadHTMLDocument(
                Starfish::String::fromUTF8(url.data(), url.size()));
        });
}

std::string WebContainerImpl::GetURL()
{
    std::string ret;
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync([&]() -> void {
        ret = m_webView->mainBrowsingContext()
                  ->window()
                  ->location()
                  ->url()
                  ->urlString()
                  ->toUTF8NonGCString();
    });
    return ret;
}

void WebContainerImpl::LoadData(const std::string& data)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        m_webView->messageLoop(), [=]() -> void {
            if (data.size() > 0) {
                auto dataURI =
                    Starfish::Base64Utils::encodeBase64HTMLDataURI(data);
                m_webView->loadHTMLDocument(
                    Starfish::String::fromUTF8(dataURI.data(), dataURI.size()));
            } else {
                m_webView->loadHTMLDocument(
                    Starfish::String::fromUTF8("about:blank"));
            }
        });
}

void WebContainerImpl::Reload()
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        m_webView->messageLoop(), [=]() -> void {
            m_webView->mainBrowsingContext()->window()->location()->reload();
        });
}

void WebContainerImpl::StopLoading()
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        m_webView->messageLoop(), [=]() -> void {
            STARFISH_ASSERT(m_webView);
            m_webView->mainBrowsingContext()
                ->document()
                ->resourceLoader()
                .clear();
        });
}

void WebContainerImpl::GoBack()
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        m_webView->messageLoop(), [=]() -> void {
            m_webView->mainBrowsingContext()->window()->history()->back();
        });
}

void WebContainerImpl::GoForward()
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        m_webView->messageLoop(), [=]() -> void {
            m_webView->mainBrowsingContext()->window()->history()->forward();
        });
}

bool WebContainerImpl::CanGoBack()
{
    bool ret = false;
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync([&]() -> void {
        ret =
            m_webView->mainBrowsingContext()->window()->history()->canGoBack();
    });
    return ret;
}

bool WebContainerImpl::CanGoForward()
{
    bool ret = false;
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync([&]() -> void {
        ret = m_webView->mainBrowsingContext()
                  ->window()
                  ->history()
                  ->canGoForward();
    });
    return ret;
}

void WebContainerImpl::AddJavaScriptInterface(
    const std::string& exposedObjectName, const std::string& jsFunctionName,
    std::function<std::string(const std::string&)> cb)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        m_webView->messageLoop(), [=]() -> void {
            Starfish::String* objectName = Starfish::String::fromUTF8(
                exposedObjectName.data(), exposedObjectName.size());
            Starfish::String* functionName = Starfish::String::fromUTF8(
                jsFunctionName.data(), jsFunctionName.size());

            m_webView->addJavaScriptNativeInterface(
                objectName, functionName,
                new Starfish::JavaScriptNativeHandler(m_webView, functionName,
                                                      cb),
                nativeCallbackFunction);

            if (m_webView->mainBrowsingContext()) {
                Starfish::registerJavaScriptNativeInterface(
                    m_webView->mainBrowsingContext()
                        ->window()
                        ->scriptBindingInstance(),
                    objectName, functionName,
                    new Starfish::JavaScriptNativeHandler(m_webView,
                                                          functionName, cb),
                    nativeCallbackFunction);
            }
        });
}

std::string WebContainerImpl::EvaluateJavaScript(const std::string& script)
{
    std::string ret;
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync([&]() -> void {
        ret = m_webView
                  ->evaluateJavaScript(
                      Starfish::String::fromUTF8(script.data(), script.size()))
                  ->toUTF8NonGCString();
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
    p->webview = m_webView;
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
        m_webView->messageLoop(),
        [=]() -> void { m_webView->historyManager()->clear(); });
}

void WebContainerImpl::Destroy()
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync([&]() -> void {
        m_webView->destroy();
        m_webView = nullptr;
        GC_FREE(this);
    });
}

void WebContainerImpl::Resume()
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
        [&]() -> void { m_webView->renderer()->resume(); });
}

void WebContainerImpl::Pause()
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
        [&]() -> void { m_webView->renderer()->pause(); });
}

void WebContainerImpl::Focus()
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        m_webView->messageLoop(), [=]() -> void {
            // TODO
            STARFISH_UNIMPLEMENTED();
        });
}

void WebContainerImpl::Blur()
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        m_webView->messageLoop(),
        [=]() -> void { m_webView->renderer()->webView()->blur(); });
}

void WebContainerImpl::SetSettings(const Settings* settings)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        m_webView->messageLoop(), [=]() -> void {
            m_webView->setCustomUserAgentString(Starfish::String::fromUTF8(
                settings->GetUserAgentString().data(),
                settings->GetUserAgentString().size()));
            m_webView->setProxyURL(settings->GetProxyURL());
#ifdef STARFISH_ENABLE_TTS
            m_webView->tts()->setMode(settings->GetTTSMode());
            m_webView->tts()->setUserLanguage(settings->GetTTSLanguage());
#endif
            unsigned char r, g, b, a;
            settings->GetBaseBackgroundColor(r, g, b, a);
            m_webView->setBaseBackgroundColor(
                Starfish::Unit::Color(r, g, b, a));
            settings->GetBaseForegroundColor(r, g, b, a);
            m_webView->setBaseForegroundColor(
                Starfish::Unit::Color(r, g, b, a));
#ifdef STARFISH_ENABLE_HTTPCACHE
            Nullable<::Starfish::HTTPCache*> cache =
                m_webView->starfish()->httpCache();
            if (cache.hasValue()) {
                cache->setCacheMode(settings->GetCacheMode());
            } else {
                STARFISH_LOG_ERROR(
                    "Http Cache could not initialized. So Changing cache mode "
                    "is no "
                    "effect.. ");
            }
#endif
            m_webView->setWebSecurityMode(settings->GetWebSecurityMode());
            m_webView->setIdleModeJob(settings->GetIdleModeJob());
            m_webView->setIdleModeCheckIntervalInMS(
                settings->GetIdleModeCheckIntervalInMS());
            m_webView->setNeedsDownloadWebFontsEarly(
                settings->NeedsDownloadWebFontsEarly());
            m_webView->setUseHttp2(settings->UseHttp2());
            m_webView->setNeedsDownScaleImageResourceLargerThan(
                settings->NeedsDownScaleImageResourceLargerThan());
#ifndef TIZEN_COMPAT_HEADER_5_0
            m_webView->setScrollbarVisible(settings->ScrollbarVisible());
#endif
            m_webView->setUseExternalPopup(settings->UseExternalPopup());
            m_webView->setUseSpatialNavigation(
                settings->UseSpatialNavigation());

            delete settings;
        });
}

void WebContainerImpl::RemoveJavascriptInterface(
    const std::string& exposedObjectName, const std::string& jsFunctionName)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        m_webView->messageLoop(), [=]() -> void {
            Starfish::String* objectName = Starfish::String::fromUTF8(
                exposedObjectName.data(), exposedObjectName.size());
            Starfish::String* functionName = Starfish::String::fromUTF8(
                jsFunctionName.data(), jsFunctionName.size());

            m_webView->removeJavaScriptNativeInterface(objectName,
                                                       functionName);

            if (!jsFunctionName.empty()) {
                Starfish::unregisterJavaScriptNativeInterface(
                    m_webView->mainBrowsingContext()
                        ->window()
                        ->scriptBindingInstance(),
                    objectName, functionName);
            } else {
                Starfish::unregisterJavaScriptNativeInterface(
                    m_webView->mainBrowsingContext()
                        ->window()
                        ->scriptBindingInstance(),
                    objectName);
            }
        });
}
void WebContainerImpl::ClearCache()
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        m_webView->messageLoop(), [=]() -> void {
#ifdef STARFISH_ENABLE_HTTPCACHE
            Nullable<::Starfish::HTTPCache*> cache =
                m_webView->starfish()->httpCache();
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
        m_webView->messageLoop(), [=]() -> void {
            m_webView->registerPublicWebViewHandler(
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
        m_webView->messageLoop(), [=]() -> void {
            m_webView->registerPublicWebViewHandler(
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
        m_webView->messageLoop(), [=]() -> void {
            m_webView->registerPublicWebViewHandler(
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
        m_webView->messageLoop(), [=]() -> void {
            m_webView->registerPublicWebViewHandler(
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
        m_webView->messageLoop(), [=]() -> void {
            m_webView->registerPublicWebViewHandler(
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
        m_webView->messageLoop(), [=]() -> void {
            m_webView->registerPublicWebViewHandler(
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
                        m_webView->navigateAsync(
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
        m_webView->messageLoop(), [=]() -> void {
            m_webView->registerPublicWebViewHandler(
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
        m_webView->messageLoop(), [=]() -> void {
            m_webView->renderer()->registerCallbackHandler(
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
        m_webView->messageLoop(), [=]() -> void {
            m_webView->renderer()->registerCallbackHandler(
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
        m_webView->messageLoop(), [=]() -> void {
            m_webView->registerCustomFileResourceRequestCallbacks(
                resolveFilePathCallback, fileOpenCallback, fileReadCallback,
                fileLengthCallback, fileCloseCallback);
        });
}

void WebContainerImpl::CallHandler(const std::string& handler, void* param)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        m_webView->messageLoop(), [=]() -> void {
            if (handler.compare("onDropdownMenuItemSelected") == 0) {
                m_webView->renderer()->callHandler(
                    Starfish::WindowHandlerOnDropdownMenuItemSelected, param);
            }
        });
}

size_t WebContainerImpl::Width()
{
    size_t ret = 0;
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
        [&]() -> void { ret = m_webView->renderer()->width(); });
    return ret;
}

size_t WebContainerImpl::Height()
{
    size_t ret = 0;
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
        [&]() -> void { ret = m_webView->renderer()->height(); });
    return ret;
}

void WebContainerImpl::RegisterOnProgressChangedHandler(
    const std::function<void(WebContainer*, int)>& cb)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        m_webView->messageLoop(), [=]() -> void {
            m_webView->registerPublicWebViewHandler(
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
        m_webView->messageLoop(), [=]() -> void {
            m_webView->registerPublicWebViewHandler(
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
        m_webView->messageLoop(), [=]() -> void {
            m_webView->registerPublicWebViewHandler(
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

void WebContainerImpl::RegisterOnIdleHandler(
    const std::function<void(WebContainer*)>& cb)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        m_webView->messageLoop(), [cb, this]() -> void {
            m_webView->registerPublicWebViewHandler(
                Starfish::OnIdle,
                [cb, this](void* param) -> void { cb(this); });
        });
}

void WebContainerImpl::SetUserAgentString(const std::string& userAgent)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync([&]() -> void {
        m_webView->setCustomUserAgentString(
            Starfish::String::fromUTF8(userAgent.data(), userAgent.size()));
    });
}

std::string WebContainerImpl::GetUserAgentString()
{
    std::string ret;
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
        [&]() -> void { ret = m_webView->userAgent()->toUTF8NonGCString(); });
    return ret;
}

void WebContainerImpl::SetCacheMode(int mode)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync([&]() -> void {
#ifdef STARFISH_ENABLE_HTTPCACHE
        Nullable<::Starfish::HTTPCache*> cache =
            m_webView->starfish()->httpCache();
        if (cache.hasValue()) {
            cache->setCacheMode(mode);
        }
#endif
    });
}

int WebContainerImpl::GetCacheMode()
{
    int ret = 0;
#ifdef STARFISH_ENABLE_HTTPCACHE
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync([&]() -> void {
        Nullable<::Starfish::HTTPCache*> cache =
            m_webView->starfish()->httpCache();
        if (cache.hasValue()) {
            ret = cache->cacheMode();
        }
    });
#endif
    return ret;
}

void WebContainerImpl::SetDefaultFontSize(uint32_t size)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync([&]() -> void {
        if (LWE_MIN_FONT_SIZE <= size && size <= LWE_MAX_FONT_SIZE) {
            m_webView->setDefaultFontSize(size);
        }
    });
}

uint32_t WebContainerImpl::GetDefaultFontSize()
{
    uint32_t ret = 0;
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
        [&]() -> void { ret = m_webView->defaultFontSize(); });
    return ret;
}

void WebContainerImpl::DispatchMouseMoveEvent(MouseButtonValue button,
                                              MouseButtonsValue buttons,
                                              double x, double y)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        m_webView->messageLoop(), [=]() -> void {
            m_webView->renderer()->dispatchMouseEvent(
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
        m_webView->messageLoop(), [=]() -> void {
            m_webView->renderer()->dispatchMouseEvent(
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
        m_webView->messageLoop(), [=]() -> void {
            m_webView->renderer()->dispatchMouseEvent(
                ::Starfish::MouseEventKind::MouseEventUp,
                ::Starfish::MouseData(button, buttons, x, y, 0,
                                      Starfish::timestamp()));
        });
}

void WebContainerImpl::DispatchMouseWheelEvent(double x, double y, int delta)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        m_webView->messageLoop(), [=]() -> void {
            m_webView->renderer()->dispatchMouseWheelEvent(x, y, delta, true);
        });
}

void WebContainerImpl::DispatchKeyDownEvent(KeyValue keyCode)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        m_webView->messageLoop(), [=]() -> void {
            m_webView->renderer()->dispatchKeyEvent(
                ::Starfish::KeyEventKind::KeyEventDown,
                ::Starfish::PlatformKeyEventData(keyCode));
        });
}

void WebContainerImpl::DispatchKeyPressEvent(KeyValue keyCode)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        m_webView->messageLoop(), [=]() -> void {
            m_webView->renderer()->dispatchKeyEvent(
                ::Starfish::KeyEventKind::KeyEventPress,
                ::Starfish::PlatformKeyEventData(keyCode));
        });
}

void WebContainerImpl::DispatchKeyUpEvent(KeyValue keyCode)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        m_webView->messageLoop(), [=]() -> void {
            m_webView->renderer()->dispatchKeyEvent(
                ::Starfish::KeyEventKind::KeyEventUp,
                ::Starfish::PlatformKeyEventData(keyCode));
        });
}

void WebContainerImpl::DispatchCompositionStartEvent(
    const std::string& soFarCompositiedString)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        m_webView->messageLoop(), [=]() -> void {
            m_webView->renderer()->dispatchCompositionEvent(
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
        m_webView->messageLoop(), [=]() -> void {
            m_webView->renderer()->dispatchCompositionEvent(
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
        m_webView->messageLoop(), [=]() -> void {
            m_webView->renderer()->dispatchCompositionEvent(
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
        m_webView->messageLoop(), [=]() -> void {
            m_webView->renderer()
                ->registerShowSoftwareKeyboardIfPossibleCallback(
                    [this, cb]() { cb(this); });
        });
}

void WebContainerImpl::RegisterOnHideSoftwareKeyboardIfPossibleHandler(
    const std::function<void(WebContainer*)>& cb)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        m_webView->messageLoop(), [=]() -> void {
            m_webView->renderer()
                ->registerHideSoftwareKeyboardIfPossibleCallback(
                    [this, cb]() { cb(this); });
        });
}

void WebContainerImpl::RegisterSetNeedsRenderingCallback(
    const std::function<void(
        WebContainer*, const std::function<void()>& doRenderingFunction)>& cb)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        m_webView->messageLoop(), [=]() -> void {
            m_webView->renderer()->registerSetNeedsRenderingCallback(
                [this, cb](Starfish::Renderer* renderer) {
                    std::function<void()> fn = [renderer]() {
                        auto p = renderer;
                        p->rendering();
                    };
                    cb(this, fn);
                });
        });
}

void WebContainerImpl::SetUserData(const std::string& key, void* data)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadAsync(
        m_webView->messageLoop(),
        [=]() -> void { m_webView->publicLayerUserDataMap()[key] = data; });
}

void* WebContainerImpl::GetUserData(const std::string& key)
{
    void* ret = nullptr;
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
        [&]() -> void { ret = m_webView->publicLayerUserDataMap()[key]; });
    return ret;
}

std::string WebContainerImpl::GetTitle()
{
    Starfish::String* ret = Starfish::String::emptyString;
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync([&]() -> void {
        ret = m_webView->mainBrowsingContext()->document()->title();
    });
    return ret->toUTF8NonGCString();
}

void WebContainerImpl::ScrollTo(int x, int y)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync([&]() -> void {
        m_webView->mainBrowsingContext()->window()->scrollTo((double)x,
                                                             (double)y);
    });
}

void WebContainerImpl::ScrollBy(int x, int y)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync([&]() -> void {
        m_webView->mainBrowsingContext()->window()->scrollBy((double)x,
                                                             (double)y);
    });
}

int WebContainerImpl::GetScrollX()
{
    int x = 0;
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync([&]() -> void {
        x = (int)m_webView->mainBrowsingContext()->window()->scrollX();
    });
    return x;
}

int WebContainerImpl::GetScrollY()
{
    int y = 0;
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync([&]() -> void {
        y = (int)m_webView->mainBrowsingContext()->window()->scrollY();
    });
    return y;
}

void WebContainerImpl::SetDevicePixelRatio(float dpr)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
        [&]() -> void { m_webView->renderer()->setDevicePixelRatio(dpr); });
}

float WebContainerImpl::GetDevicePixelRatio()
{
    float dpr = 0;
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
        [&]() -> void { dpr = m_webView->renderer()->getDevicePixelRatio(); });
    return dpr;
}

void WebContainerImpl::RegisterGetScreenMatrixHandler(
    const std::function<TransformationMatrix(WebContainer*)>& cb)
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync([&]() -> void {
        m_webView->renderer()->registerGetScreenMatrix(
            [this, cb](Starfish::Renderer* renderer)
                -> Starfish::TransformationMatrix {
                TransformationMatrix m = cb(this);
                return {
                    m.scaleX,       m.skewX,        m.translateX,
                    m.skewY,        m.scaleY,       m.translateY,
                    m.perspectiveX, m.perspectiveY, m.perspectiveScale,
                };
            });
    });
}

void WebContainerImpl::SetNeedsFullRepainting()
{
    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
        [&]() -> void { m_webView->setNeedsFullRepainting(); });
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

uintptr_t EXPORT_UNMANAGED_API LWEDelegate_WebContainer_CreateGL(
    uintptr_t webContainerArguments, uintptr_t rendererGLConfiguration)
{
    auto* args = reinterpret_cast<
        const LWEDelegate::WebContainer::WebContainerArguments*>(
        webContainerArguments);
    auto* config = reinterpret_cast<
        const LWEDelegate::WebContainer::RendererGLConfiguration*>(
        rendererGLConfiguration);
    return reinterpret_cast<uintptr_t>(
        LWEDelegate::WebContainer::CreateGL(*args, *config));
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
