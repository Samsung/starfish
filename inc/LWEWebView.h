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
#include <string>

namespace LWE {
class LWE_EXPORT Settings {
public:
    Settings(const std::string& defaultUA, const std::string& ua);
    std::string GetDefaultUserAgent() const;
    std::string GetUserAgentString() const;
    int GetCacheMode() const;
    void SetUserAgentString(const std::string& ua);
    void SetCacheMode(int mode);

private:
    std::string m_defaultUserAgent;
    std::string m_userAgent;
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

class LWE_EXPORT WebView {
public:
    static WebView* Create(void* starFish);
    static WebView* Create(void* win, int x, int y, int width, int height);

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
    void SetSettings(const Settings& setttings);
    void RemoveJavascriptInterface(const std::string& exposedObjectName,
                                   const std::string& jsFunctionName);
    void ClearCache();
    void RegisterOnReceivedErrorHandler(
        std::function<void(LWE::WebView*, LWE::ResourceError)> cb);
    void RegisterOnPageFinishedHandler(
        std::function<void(LWE::WebView*, const std::string&)> cb);
    void RegisterOnPageStartedHandler(
        std::function<void(LWE::WebView*, const std::string&)> cb);
    void RegisterOnLoadResourceHandler(
        std::function<void(LWE::WebView*, const std::string&)> cb);

    void* unwrap();

protected:
    WebView(void* starFish);

private:
    void* m_starfish;
};

class LWE_EXPORT WebContainer {
public:
    static WebContainer* Create(void* buffer, uint width, uint height,
                                uint stride, float scaleFactor);

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
    void SetSettings(const Settings& setttings);
    void RemoveJavascriptInterface(const std::string& exposedObjectName,
                                   const std::string& jsFunctionName);
    void ClearCache();

    void RegisterOnReceivedErrorHandler(
        const std::function<void(LWE::WebContainer*, LWE::ResourceError)>& cb);
    void RegisterOnPageFinishedHandler(
        const std::function<void(LWE::WebContainer*, const std::string&)>& cb);
    void RegisterOnPageStartedHandler(
        const std::function<void(LWE::WebContainer*, const std::string&)>& cb);
    void RegisterOnLoadResourceHandler(
        const std::function<void(LWE::WebContainer*, const std::string&)>& cb);

    void UpdateBuffer(void* buffer, uint width, uint height, uint stride);
    void RenderingDirectly();
    void RegisterOnRenderedHandler(
        const std::function<void(LWE::WebContainer*, void*)>& cb);

    void SetUserAgentString(const std::string& userAgent);
    void SetCacheMode(int mode);
    void DispatchMouseMoveEvent(MouseButtonValue button,
                                MouseButtonsValue buttons, double x, double y);
    void DispatchMouseDownEvent(MouseButtonValue button,
                                MouseButtonsValue buttons, double x, double y);
    void DispatchMouseUpEvent(MouseButtonValue button,
                              MouseButtonsValue buttons, double x, double y);
    void DispatchMouseWheelEvent(double x, double y, int delta);
    void DispatchKeyDownEvent(KeyValue keyCode,
                              int modifier = 0); // modifiers not defined yet.
    void DispatchKeyPressEvent(KeyValue keyCode, int modifier = 0);
    void DispatchKeyUpEvent(KeyValue keyCode, int modifier = 0);

protected:
    WebContainer(void* starFish);

private:
    void* m_starfish;
};
}

#ifdef PORT_WINDOW_BACKEND_ANDROID
void requestRender(void* view);
#endif

#endif
