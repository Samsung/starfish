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
#include <string>
#include <cstdint>

namespace LWEDelegate {

class Settings;
class ResourceError;
class WebContainer;

class EXPORT_UNMANAGED_API WebView {
public:
    static WebView* Create(void* win, unsigned x, unsigned y, unsigned width,
                           unsigned height, float devicePixelRatio,
                           const char* defaultFontName, const char* locale,
                           const char* timezoneID);

    virtual void Destroy() = 0;

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

    virtual void Pause() = 0;

    virtual void Resume() = 0;

    virtual void AddJavaScriptInterface(
        const std::string& exposedObjectName, const std::string& jsFunctionName,
        std::function<std::string(const std::string&)> cb) = 0;

    virtual std::string EvaluateJavaScript(const std::string& script) = 0;

    virtual void EvaluateJavaScript(
        const std::string& script,
        std::function<void(const std::string&)> cb) = 0;

    virtual void ClearHistory() = 0;

    virtual void SetSettings(const Settings* settings) = 0;

    virtual void RemoveJavascriptInterface(
        const std::string& exposedObjectName,
        const std::string& jsFunctionName) = 0;

    virtual void ClearCache() = 0;

    virtual void RegisterOnReceivedErrorHandler(
        std::function<void(WebView*, ResourceError*)> cb) = 0;

    virtual void RegisterOnPageParsedHandler(
        std::function<void(WebView*, const std::string&)> cb) = 0;

    virtual void RegisterOnPageLoadedHandler(
        std::function<void(WebView*, const std::string&)> cb) = 0;

    virtual void RegisterOnPageStartedHandler(
        std::function<void(WebView*, const std::string&)> cb) = 0;

    virtual void RegisterOnLoadResourceHandler(
        std::function<void(WebView*, const std::string&)> cb) = 0;

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

    virtual void SetUserData(const std::string& key, void* data) = 0;

    virtual void* GetUserData(const std::string& key) = 0;

    virtual std::string GetTitle() = 0;

    virtual void ScrollTo(int x, int y) = 0;

    virtual void ScrollBy(int x, int y) = 0;

    virtual int GetScrollX() = 0;

    virtual int GetScrollY() = 0;

    virtual void* Unwrap() = 0;

    virtual void Focus() = 0;

    virtual void Blur() = 0;

    virtual void SetDevicePixelRatio(float dpr) = 0;

    virtual float GetDevicePixelRatio() = 0;

    virtual WebContainer* FetchWebContainer() = 0;

protected:
    WebView() = default;
    virtual ~WebView() = default;
};

} // namespace LWEDelegate

// C wrappers used for dlopen/dlsym.
extern "C" {

uintptr_t EXPORT_UNMANAGED_API LWEDelegate_WebView_Create(
    void* win, unsigned x, unsigned y, unsigned width, unsigned height,
    float devicePixelRatio, const char* defaultFontName, const char* locale,
    const char* timezoneID);

typedef struct {
    uintptr_t (*Create)(void*, unsigned, unsigned, unsigned, unsigned, float,
                        const char*, const char*, const char*);
} WebViewProcTable;
}

#endif
