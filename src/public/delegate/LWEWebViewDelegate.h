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
#ifndef __LWEWebViewDelegate__
#define __LWEWebViewDelegate__

#include "LWEDelegateConfig.h"

#include <functional>
#include <vector>

namespace LWEDelegate {

class Settings;
class ResourceError;
class WebContainer;

class EXPORT_UNMANAGED_API WebView {
protected:
    virtual ~WebView()
    {
    }

public:
    static WebView* Create(void* win, unsigned x, unsigned y, unsigned width,
                           unsigned height, float devicePixelRatio,
                           const char* defaultFontName, const char* locale,
                           const char* timezoneID);

    virtual void Destroy();

    Settings* GetSettings();

    virtual void LoadURL(const std::string& url);

    std::string GetURL();

    void LoadData(const std::string& data);

    void Reload();

    void StopLoading();

    void GoBack();

    void GoForward();

    bool CanGoBack();

    bool CanGoForward();

    void Pause();

    void Resume();

    void AddJavaScriptInterface(
        const std::string& exposedObjectName, const std::string& jsFunctionName,
        std::function<std::string(const std::string&)> cb);

    std::string EvaluateJavaScript(const std::string& script);

    void EvaluateJavaScript(const std::string& script,
                            std::function<void(const std::string&)> cb);

    void ClearHistory();

    void SetSettings(const Settings* settings);

    void RemoveJavascriptInterface(const std::string& exposedObjectName,
                                   const std::string& jsFunctionName);

    void ClearCache();

    void RegisterOnReceivedErrorHandler(
        std::function<void(WebView*, ResourceError*)> cb);

    void RegisterOnPageParsedHandler(
        std::function<void(WebView*, const std::string&)> cb);

    void RegisterOnPageLoadedHandler(
        std::function<void(WebView*, const std::string&)> cb);

    void RegisterOnPageStartedHandler(
        std::function<void(WebView*, const std::string&)> cb);

    void RegisterOnLoadResourceHandler(
        std::function<void(WebView*, const std::string&)> cb);

    void RegisterCustomFileResourceRequestHandlers(
        std::function<const char*(const char* path)> resolveFilePathCallback,
        std::function<void*(const char* path)> fileOpenCallback,
        std::function<size_t(uint8_t* destBuffer, size_t size, void* handle)>
            fileReadCallback,
        std::function<long int(void* handle)> fileLengthCallback,
        std::function<void(void* handle)> fileCloseCallback);
    void RegisterDebuggerShouldInitHandler(
        const std::function<void(const std::string& url, int port,
                                 bool& shouldInit)>& cb);
    void RegisterDebuggerShouldContinueWaitingHandler(
        const std::function<void(const std::string& url, int port,
                                 bool& shouldWait)>& cb);

    void SetUserData(const std::string& key, void* data);

    void* GetUserData(const std::string& key);

    std::string GetTitle();

    void ScrollTo(int x, int y);

    void ScrollBy(int x, int y);

    int GetScrollX();

    int GetScrollY();

    virtual void* Unwrap()
    {
        return nullptr;
    }

    virtual void Focus();

    virtual void Blur();

    void SetDevicePixelRatio(float dpr);

    float GetDevicePixelRatio();

    virtual WebContainer* FetchWebContainer() = 0;

protected:
    WebView(void* impl)
        : m_impl(impl)
    {
    }

    void* m_impl;
};

} // namespace LWEDelegate

#endif
