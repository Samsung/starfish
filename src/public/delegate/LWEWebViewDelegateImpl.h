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
#ifndef __LWEWebViewDelegateImpl__
#define __LWEWebViewDelegateImpl__

#include "LWEWebViewDelegate.h"

namespace LWEDelegate {

class WebViewImpl : public WebView {
public:
    virtual void Destroy() override;

    virtual Settings* GetSettings() override;

    virtual void LoadURL(const std::string& url) override;

    virtual std::string GetURL() override;

    virtual void LoadData(const std::string& data) override;

    virtual void Reload() override;

    virtual void StopLoading() override;

    virtual void GoBack() override;

    virtual void GoForward() override;

    virtual bool CanGoBack() override;

    virtual bool CanGoForward() override;

    virtual void Pause() override;

    virtual void Resume() override;

    virtual void AddJavaScriptInterface(
        const std::string& exposedObjectName, const std::string& jsFunctionName,
        std::function<std::string(const std::string&)> cb) override;

    virtual std::string EvaluateJavaScript(const std::string& script) override;

    virtual void EvaluateJavaScript(
        const std::string& script,
        std::function<void(const std::string&)> cb) override;

    virtual void ClearHistory() override;

    virtual void SetSettings(const Settings* settings) override;

    virtual void RemoveJavascriptInterface(
        const std::string& exposedObjectName,
        const std::string& jsFunctionName) override;

    virtual void ClearCache() override;

    virtual void RegisterOnReceivedErrorHandler(
        std::function<void(WebView*, ResourceError*)> cb) override;

    virtual void RegisterOnPageParsedHandler(
        std::function<void(WebView*, const std::string&)> cb) override;

    virtual void RegisterOnPageLoadedHandler(
        std::function<void(WebView*, const std::string&)> cb) override;

    virtual void RegisterOnPageStartedHandler(
        std::function<void(WebView*, const std::string&)> cb) override;

    virtual void RegisterOnLoadResourceHandler(
        std::function<void(WebView*, const std::string&)> cb) override;

    virtual void RegisterCustomFileResourceRequestHandlers(
        std::function<const char*(const char* path)> resolveFilePathCallback,
        std::function<void*(const char* path)> fileOpenCallback,
        std::function<size_t(uint8_t* destBuffer, size_t size, void* handle)>
            fileReadCallback,
        std::function<long int(void* handle)> fileLengthCallback,
        std::function<void(void* handle)> fileCloseCallback) override;
    virtual void RegisterDebuggerShouldInitHandler(
        const std::function<void(const std::string& url, int port,
                                 bool& shouldInit)>& cb) override;
    virtual void RegisterDebuggerShouldContinueWaitingHandler(
        const std::function<void(const std::string& url, int port,
                                 bool& shouldWait)>& cb) override;

    virtual void SetUserData(const std::string& key, void* data) override;

    virtual void* GetUserData(const std::string& key) override;

    virtual std::string GetTitle() override;

    virtual void ScrollTo(int x, int y) override;

    virtual void ScrollBy(int x, int y) override;

    virtual int GetScrollX() override;

    virtual int GetScrollY() override;

    virtual void* Unwrap() override
    {
        return nullptr;
    }

    virtual void Focus() override;

    virtual void Blur() override;

    virtual void SetDevicePixelRatio(float dpr) override;

    virtual float GetDevicePixelRatio() override;

    virtual WebContainer* FetchWebContainer() override;

protected:
    WebViewImpl();

    virtual ~WebViewImpl();

    void SetWebContainer(WebContainer* webContainer);

    WebContainer* m_webContainer = nullptr;
};

} // namespace LWEDelegate

#endif
