/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
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
#ifndef __LWEWebContainerDelegate__
#define __LWEWebContainerDelegate__

#include "LWEDelegateConfig.h"

#include "PlatformIntegrationData.h"

#include <functional>
#include <vector>

namespace LWEDelegate {

class Settings;
class ResourceError;

class EXPORT_UNMANAGED_API WebContainer {
public:
    // Function set for render to buffer
    // For Tizen 5.5 and above.
    static WebContainer* Create(unsigned width, unsigned height,
                                float devicePixelRatio,
                                const char* defaultFontName, const char* locale,
                                const char* timezoneID);
    // For Tizen 5.0.
    static WebContainer* CreateWithBuffer(void* buffer, unsigned bufferWidth,
                                          unsigned bufferHeight,
                                          unsigned bufferStride,
                                          float devicePixelRatio,
                                          const char* defaultFontName,
                                          const char* locale,
                                          const char* timezoneID);
    // For Tizen 5.5 and above.
    struct RenderInfo {
        void* updatedBufferAddress;
        size_t bufferStride;
    };

    struct ExternalImageInfo {
        void* imageAddress;
    };

    struct RenderResult {
        size_t updatedX;
        size_t updatedY;
        size_t updatedWidth;
        size_t updatedHeight;

        void* updatedBufferAddress;
        size_t bufferImageWidth;
        size_t bufferImageHeight;
    };

    struct WebContainerArguments {
        unsigned width;
        unsigned height;
        float devicePixelRatio;
        const char* defaultFontName;
        const char* locale;
        const char* timezoneID;
    };

    struct TransformationMatrix {
        double scaleX;
        double skewX;
        double translateX;
        double skewY;
        double scaleY;
        double translateY;
        double perspectiveX;
        double perspectiveY;
        double perspectiveScale;
    };

    using OnMakeCurrent = std::function<void(WebContainer*)>;
    using OnSwapBuffers = std::function<void(WebContainer*, bool mayNeedsSync)>;
    using OnCreateSharedContext = std::function<uintptr_t(WebContainer*)>;
    using OnDestroyContext = std::function<bool(WebContainer*, uintptr_t)>;
    using OnClearCurrentContext = std::function<bool(WebContainer*)>;
    using OnMakeCurrentWithContext =
        std::function<bool(WebContainer*, uintptr_t)>;
    using OnGetProcAddress = std::function<void*(WebContainer*, const char*)>;
    using OnIsSupportedExtension =
        std::function<bool(WebContainer*, const char*)>;

    struct RendererGLConfiguration {
        OnMakeCurrent onMakeCurrent;
        OnSwapBuffers onSwapBuffers;
        OnCreateSharedContext onCreateSharedContext;
        OnDestroyContext onDestroyContext;
        OnClearCurrentContext onClearCurrentContext;
        OnMakeCurrentWithContext onMakeCurrentWithContext;
        OnGetProcAddress onGetProcAddress;
        OnIsSupportedExtension onIsSupportedExtension;
    };

    // For Tizen 5.5 and above.
    virtual void RegisterPreRenderingHandler(
        const std::function<RenderInfo(void)>& cb) = 0;

    virtual void RegisterOnRenderedHandler(
        const std::function<void(WebContainer*,
                                 const RenderResult& renderResult)>& cb) = 0;
    // For Tizen 5.0
    virtual void UpdateBuffer(void* buffer, unsigned width, unsigned height,
                              unsigned stride) = 0;

    using OnPrepareImage = std::function<ExternalImageInfo(void)>;
    using OnFlush = std::function<void(WebContainer*, bool needsFlush)>;
    static WebContainer* CreateWithPlatformImage(
        const WebContainerArguments& args, const OnPrepareImage& prepareImageCb,
        const OnFlush& flushCb);
    // <--- end of function set for render to buffer

    // Function set for render with OpenGL
    static WebContainer* CreateGL(const WebContainerArguments& args,
                                  const RendererGLConfiguration& config);

    static WebContainer* CreateGLWithPlatformImage(
        const WebContainerArguments& args,
        const RendererGLConfiguration& config,
        const OnPrepareImage& prepareImageCb, const OnFlush& flushCb);

    // <--- end of function set for render with OpenGL

    // Function set for headless
    static WebContainer* CreateHeadless(unsigned width, unsigned height,
                                        float devicePixelRatio,
                                        const char* defaultFontName,
                                        const char* locale,
                                        const char* timezoneID);
    // <--- end of function set for headless

    virtual void AddIdleCallback(void (*callback)(void*), void* data) = 0;
    virtual size_t AddTimeout(void (*callback)(void*), void* data,
                              size_t timeoutInMS) = 0;
    virtual void ClearTimeout(size_t handle) = 0;

    virtual void RegisterCanRenderingHandler(
        const std::function<bool(WebContainer*)>& cb) = 0;

    virtual Settings* GetSettings() = 0;
    virtual void LoadURL(const std::string& url) = 0;
    virtual std::string GetURL() = 0;
    virtual void LoadData(const std::string& data) = 0;
    virtual void Reload() = 0;
    virtual void StopLoading() = 0;
    virtual void GoBack() = 0;
    virtual void GoForward() = 0;
    virtual bool CanGoBack() = 0;
    virtual bool CanGoForward() = 0;
    virtual void AddJavaScriptInterface(
        const std::string& exposedObjectName, const std::string& jsFunctionName,
        std::function<std::string(const std::string&)> cb) = 0;
    virtual std::string EvaluateJavaScript(const std::string& script) = 0;
    virtual void EvaluateJavaScript(
        const std::string& script,
        std::function<void(const std::string&)> cb) = 0;
    virtual void ClearHistory() = 0;
    virtual void Destroy() = 0;
    virtual void Pause() = 0;
    virtual void Resume() = 0;

    virtual void ResizeTo(size_t width, size_t height) = 0;

    virtual void Focus() = 0;
    virtual void Blur() = 0;

    virtual void SetSettings(const Settings* settings) = 0;
    virtual void RemoveJavascriptInterface(
        const std::string& exposedObjectName,
        const std::string& jsFunctionName) = 0;
    virtual void ClearCache() = 0;

    virtual void RegisterOnReceivedErrorHandler(
        const std::function<void(WebContainer*, ResourceError*)>& cb) = 0;
    virtual void RegisterOnPageParsedHandler(
        std::function<void(WebContainer*, const std::string&)> cb) = 0;
    virtual void RegisterOnPageLoadedHandler(
        std::function<void(WebContainer*, const std::string&)> cb) = 0;
    virtual void RegisterOnPageStartedHandler(
        const std::function<void(WebContainer*, const std::string&)>& cb) = 0;
    virtual void RegisterOnLoadResourceHandler(
        const std::function<void(WebContainer*, const std::string&)>& cb) = 0;
    virtual void RegisterShouldOverrideUrlLoadingHandler(
        const std::function<bool(WebContainer*, const std::string&)>& cb) = 0;
    virtual void RegisterOnProgressChangedHandler(
        const std::function<void(WebContainer*, int progress)>& cb) = 0;
    virtual void RegisterOnDownloadStartHandler(
        const std::function<void(WebContainer*, const std::string&,
                                 const std::string&, const std::string&,
                                 const std::string&, long)>& cb) = 0;

    virtual void RegisterShowDropdownMenuHandler(
        const std::function<void(WebContainer*, const std::vector<std::string>*,
                                 int)>& cb) = 0;
    virtual void RegisterShowAlertHandler(
        const std::function<void(WebContainer*, const std::string&,
                                 const std::string&)>& cb) = 0;

    virtual void RegisterCustomFileResourceRequestHandlers(
        std::function<const char*(const char* path)> resolveFilePathCallback,
        std::function<void*(const char* path)> fileOpenCallback,
        std::function<size_t(uint8_t* destBuffer, size_t size, void* handle)>
            fileReadCallback,
        std::function<long int(void* handle)> fileLengthCallback,
        std::function<void(void* handle)> fileCloseCallback) = 0;

    virtual void RegisterDebuggerShouldInitHandler(
        const std::function<void(const std::string& url, int port,
                                 bool& shouldInit)>& cb) = 0;
    virtual void RegisterDebuggerShouldContinueWaitingHandler(
        const std::function<void(const std::string& url, int port,
                                 bool& shouldWait)>& cb) = 0;

    virtual void RegisterOnIdleHandler(
        const std::function<void(WebContainer*)>& cb) = 0;

    virtual void CallHandler(const std::string& handler, void* param) = 0;

    virtual void SetUserAgentString(const std::string& userAgent) = 0;
    virtual std::string GetUserAgentString() = 0;
    virtual void SetCacheMode(int mode) = 0;
    virtual int GetCacheMode() = 0;
    virtual void SetDefaultFontSize(uint32_t size) = 0;
    virtual uint32_t GetDefaultFontSize() = 0;

    virtual void DispatchMouseMoveEvent(::LWE::MouseButtonValue button,
                                        ::LWE::MouseButtonsValue buttons,
                                        double x, double y) = 0;
    virtual void DispatchMouseDownEvent(::LWE::MouseButtonValue button,
                                        ::LWE::MouseButtonsValue buttons,
                                        double x, double y) = 0;
    virtual void DispatchMouseUpEvent(::LWE::MouseButtonValue button,
                                      ::LWE::MouseButtonsValue buttons,
                                      double x, double y) = 0;
    virtual void DispatchMouseWheelEvent(double x, double y, int delta) = 0;
    virtual void DispatchKeyDownEvent(::LWE::KeyValue keyCode) = 0;
    virtual void DispatchKeyPressEvent(::LWE::KeyValue keyCode) = 0;
    virtual void DispatchKeyUpEvent(::LWE::KeyValue keyCode) = 0;

    virtual void DispatchCompositionStartEvent(
        const std::string& soFarCompositiedString) = 0;
    virtual void DispatchCompositionUpdateEvent(
        const std::string& soFarCompositiedString) = 0;
    virtual void DispatchCompositionEndEvent(
        const std::string& soFarCompositiedString) = 0;
    virtual void RegisterOnShowSoftwareKeyboardIfPossibleHandler(
        const std::function<void(WebContainer*)>& cb) = 0;
    virtual void RegisterOnHideSoftwareKeyboardIfPossibleHandler(
        const std::function<void(WebContainer*)>& cb) = 0;

    virtual void SetUserData(const std::string& key, void* data) = 0;
    virtual void* GetUserData(const std::string& key) = 0;

    virtual std::string GetTitle() = 0;
    virtual void ScrollTo(int x, int y) = 0;
    virtual void ScrollBy(int x, int y) = 0;
    virtual int GetScrollX() = 0;
    virtual int GetScrollY() = 0;

    virtual size_t Width() = 0;
    virtual size_t Height() = 0;

    // You can control rendering flow through this function
    // If you got callback, you must call `doRenderingFunction` after
    virtual void RegisterSetNeedsRenderingCallback(
        const std::function<void(
            WebContainer*, const std::function<void()>& doRenderingFunction)>&
            cb) = 0;
    virtual void SetDevicePixelRatio(float dpr) = 0;
    virtual float GetDevicePixelRatio() = 0;

    virtual void RegisterGetScreenMatrixHandler(
        const std::function<TransformationMatrix(WebContainer*)>& cb) = 0;

    virtual void SetNeedsFullRepainting() = 0;

protected:
    WebContainer() = default;
    // use Destroy function instead of using delete operator
    virtual ~WebContainer() = default;
};

} // namespace LWEDelegate

// C wrappers used for dlopen/dlsym.
extern "C" {

uintptr_t EXPORT_UNMANAGED_API LWEDelegate_WebContainer_Create(
    unsigned width, unsigned height, float devicePixelRatio,
    const char* defaultFontName, const char* locale, const char* timezoneID);

uintptr_t EXPORT_UNMANAGED_API LWEDelegate_WebContainer_CreateWithBuffer(
    void* buffer, unsigned bufferWidth, unsigned bufferHeight,
    unsigned bufferStride, float devicePixelRatio, const char* defaultFontName,
    const char* locale, const char* timezoneID);

uintptr_t EXPORT_UNMANAGED_API
LWEDelegate_WebContainer_Create_With_PlatformImage(
    uintptr_t webContainerArguments, uintptr_t prepareImageCb,
    uintptr_t flushCb);

uintptr_t EXPORT_UNMANAGED_API LWEDelegate_WebContainer_CreateGL(
    uintptr_t webContainerArguments, uintptr_t rendererGLConfiguration);

uintptr_t EXPORT_UNMANAGED_API
LWEDelegate_WebContainer_CreateGLWithPlatformImage(
    uintptr_t webContainerArguments, uintptr_t rendererGLConfiguration,
    uintptr_t prepareImageCb, uintptr_t flushCb);

uintptr_t EXPORT_UNMANAGED_API LWEDelegate_WebContainer_CreateHeadless(
    unsigned width, unsigned height, float devicePixelRatio,
    const char* defaultFontName, const char* locale, const char* timezoneID);

typedef struct {
    uintptr_t (*Create)(unsigned, unsigned, float, const char*, const char*,
                        const char*);
    uintptr_t (*CreateWithBuffer)(void*, unsigned, unsigned, unsigned, float,
                                  const char*, const char*, const char*);
    uintptr_t (*CreateWithPlatformImage)(uintptr_t, uintptr_t, uintptr_t);
    uintptr_t (*CreateGL)(uintptr_t, uintptr_t);
    uintptr_t (*CreateGLWithPlatformImage)(uintptr_t, uintptr_t, uintptr_t,
                                           uintptr_t);
    uintptr_t (*CreateHeadless)(unsigned, unsigned, float, const char*,
                                const char*, const char*);

} WebContainerProcTable;
}

#endif
