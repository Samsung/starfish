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

#ifndef __LWEWebView__
#define __LWEWebView__

#ifndef LWE_EXPORT
#ifdef _MSC_VER
#define LWE_EXPORT __declspec(dllexport)
#else
#define LWE_EXPORT __attribute__((visibility("default")))
#endif
#endif

#include "PlatformIntegrationData.h"

#include <functional>
#include <vector>
#include <string>

namespace LWE {

class LWE_EXPORT Settings {
public:
    Settings(const std::string& defaultUA, const std::string& ua);
    std::string GetDefaultUserAgent() const;
    std::string GetUserAgentString() const;
    std::string GetProxyURL() const;
    int GetCacheMode() const;
    void SetUserAgentString(const std::string& ua);
    void SetCacheMode(int mode);
    void SetProxyURL(const std::string& ua);

private:
    std::string m_defaultUserAgent;
    std::string m_userAgent;
    std::string m_proxyURL;
    int m_cacheMode;
};

class LWE_EXPORT ResourceError {
public:
    ResourceError(int code, const std::string& description);
    int GetErrorCode();
    std::string GetDescription();

private:
    int m_errorCode;
    std::string m_description;
};

class LWE_EXPORT WebContainer {
private:
    // use Destroy function instead of using delete operator
    ~WebContainer()
    {
    }

public:
    // Function set for render to buffer
    static WebContainer* Create(void* buffer, uint bufferWidth,
                                uint bufferHeight, uint bufferStride,
                                float devicePixelRatio, const char* locale,
                                const char* timezoneID,
                                const char* localStorageFilePath,
                                const char* cookieStoreFilePath,
                                const char* httpCacheDirectorypath);

    struct RenderResult {
        size_t updatedX;
        size_t updatedY;
        size_t updatedWidth;
        size_t updatedHeight;

        void* updatedBufferAddress;
        size_t bufferImageWidth;
        size_t bufferImageHeight;
    };
    void RegisterOnRenderedHandler(
        const std::function<void(LWE::WebContainer*,
                                 const RenderResult& renderResult)>& cb);
    void UpdateBuffer(void* buffer, uint width, uint height, uint stride);
    // <--- end of function set for render to buffer

    // Function set for render with OpenGL
    static WebContainer* CreateGL(
        uint width, uint height,
        const std::function<void(LWE::WebContainer*)>& onGLMakeCurrent,
        const std::function<void(LWE::WebContainer*)>& onGLSwapBuffers,
        float devicePixelRatio, const char* locale, const char* timezoneID,
        const char* localStorageFilePath, const char* cookieStoreFilePath,
        const char* httpCacheDirectorypath);
    void ResizeTo(size_t width, size_t height);
    // <--- end of function set for render with OpenGL

    // Function set for headless
    // TODO
    // <--- end of function set for headless

    void RunMessageLoop();
    void StopMessageLoop();
    void AddIdleCallback(void (*callback)(void*), void* data);
    size_t AddTimeout(void (*callback)(void*), void* data, size_t timeoutInMS);
    void ClearTimeout(size_t handle);

    Settings GetSettings();
    void LoadURL(const std::string& url);
    std::string GetURL();
    void LoadData(const std::string& data);
    void Reload();
    void StopLoading();
    void GoBack();
    void GoForward();
    bool CanGoBack();
    bool CanGoForward();
    void AddJavaScriptInterface(
        const std::string& exposedObjectName, const std::string& jsFunctionName,
        std::function<std::string(const std::string&)> cb);
    std::string EvaluateJavaScript(const std::string& script);
    void ClearHistory();
    void Destroy();
    void Pause();
    void Resume();

    void SetSettings(const Settings& setttings);
    void RemoveJavascriptInterface(const std::string& exposedObjectName,
                                   const std::string& jsFunctionName);
    void ClearCache();

    void RegisterOnReceivedErrorHandler(
        const std::function<void(LWE::WebContainer*, LWE::ResourceError)>& cb);
    void RegisterOnPageParsedHandler(
        std::function<void(LWE::WebContainer*, const std::string&)> cb);
    void RegisterOnPageLoadedHandler(
        std::function<void(LWE::WebContainer*, const std::string&)> cb);
    void RegisterOnPageStartedHandler(
        const std::function<void(LWE::WebContainer*, const std::string&)>& cb);
    void RegisterOnLoadResourceHandler(
        const std::function<void(LWE::WebContainer*, const std::string&)>& cb);
    void RegisterShouldOverrideUrlLoadingHandler(
        const std::function<bool(LWE::WebContainer*, const std::string&)>& cb);
    void RegisterOnProgressChangedHandler(
        const std::function<void(LWE::WebContainer*, int progress)>& cb);
    void RegisterOnDownloadStartHandler(
        const std::function<void(LWE::WebContainer*, const std::string&,
                                 const std::string&, const std::string&,
                                 const std::string&, long)>& cb);

    void RegisterShowDropdownMenuHandler(
        const std::function<void(LWE::WebContainer*,
                                 const std::vector<std::string>*, int)>& cb);
    void RegisterShowAlertHandler(
        const std::function<void(LWE::WebContainer*, const std::string&,
                                 const std::string&)>& cb);

    void CallHandler(const std::string& handler, void* param);

    void SetUserAgentString(const std::string& userAgent);
    void SetCacheMode(int mode);
    void DispatchMouseMoveEvent(MouseButtonValue button,
                                MouseButtonsValue buttons, double x, double y);
    void DispatchMouseDownEvent(MouseButtonValue button,
                                MouseButtonsValue buttons, double x, double y);
    void DispatchMouseUpEvent(MouseButtonValue button,
                              MouseButtonsValue buttons, double x, double y);
    void DispatchMouseWheelEvent(double x, double y, int delta);
    void DispatchKeyDownEvent(KeyValue keyCode);
    void DispatchKeyPressEvent(KeyValue keyCode);
    void DispatchKeyUpEvent(KeyValue keyCode);

    void DispatchCompositionStartEvent(
        const std::string& soFarCompositiedString);
    void DispatchCompositionUpdateEvent(
        const std::string& soFarCompositiedString);
    void DispatchCompositionEndEvent(const std::string& soFarCompositiedString);
    void RegisterOnShowSoftwareKeyboardIfPossibleHandler(
        const std::function<void(LWE::WebContainer*)>& cb);
    void RegisterOnHideSoftwareKeyboardIfPossibleHandler(
        const std::function<void(LWE::WebContainer*)>& cb);

    size_t Width();
    size_t Height();

    // You can control rendering flow through this function
    // If you got callback, you must call `doRenderingFunction` after
    void RegisterSetNeedsRenderingCallback(
        const std::function<
            void(LWE::WebContainer*,
                 const std::function<void()>& doRenderingFunction)>& cb);

protected:
    WebContainer(void* starFish);

private:
    void* m_starfish;
};

class LWE_EXPORT WebView {
protected:
    // use Destroy function instead of using delete operator
    virtual ~WebView()
    {
    }

public:
    static WebView* Create(void* win, int x, int y, int width, int height,
                           float devicePixelRatio, const char* locale,
                           const char* timezoneID,
                           const char* localStorageFilePath,
                           const char* cookieStoreFilePath,
                           const char* httpCacheDirectorypath);

    virtual void Destroy();

    Settings GetSettings();
    void LoadURL(const std::string& url);
    std::string GetURL();
    void LoadData(const std::string& data);
    void Reload();
    void StopLoading();
    void GoBack();
    void GoForward();
    bool CanGoBack();
    bool CanGoForward();
    void AddJavaScriptInterface(
        const std::string& exposedObjectName, const std::string& jsFunctionName,
        std::function<std::string(const std::string&)> cb);
    std::string EvaluateJavaScript(const std::string& script);
    void ClearHistory();
    void SetSettings(const Settings& setttings);
    void RemoveJavascriptInterface(const std::string& exposedObjectName,
                                   const std::string& jsFunctionName);
    void ClearCache();
    void RegisterOnReceivedErrorHandler(
        std::function<void(LWE::WebView*, LWE::ResourceError)> cb);
    void RegisterOnPageParsedHandler(
        std::function<void(LWE::WebView*, const std::string&)> cb);
    void RegisterOnPageLoadedHandler(
        std::function<void(LWE::WebView*, const std::string&)> cb);
    void RegisterOnPageStartedHandler(
        std::function<void(LWE::WebView*, const std::string&)> cb);
    void RegisterOnLoadResourceHandler(
        std::function<void(LWE::WebView*, const std::string&)> cb);

    virtual void* Unwrap()
    {
        // Some platform returns associated native handle ex) Evas_Object*
        return nullptr;
    }

    void RunMessageLoop();
    void StopMessageLoop();

protected:
    WebView(void* impl)
        : m_impl(impl)
    {
    }

    virtual LWE::WebContainer* FetchWebContainer() = 0;

    void* m_impl;
};

} // namespace LWE

#endif
