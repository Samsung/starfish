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

#include "LWEWebViewDelegateImpl.h"

#include "SettingsDelegate.h"
#include "ResourceErrorDelegate.h"
#include "LWEWebContainerDelegate.h"

namespace LWEDelegate {

Settings* WebViewImpl::GetSettings()
{
    return FetchWebContainer()->GetSettings();
}

void WebViewImpl::LoadURL(const std::string& url)
{
    FetchWebContainer()->LoadURL(url);
}

std::string WebViewImpl::GetURL()
{
    return FetchWebContainer()->GetURL();
}

void WebViewImpl::LoadData(const std::string& data)
{
    FetchWebContainer()->LoadData(data);
}

void WebViewImpl::Reload()
{
    FetchWebContainer()->Reload();
}

void WebViewImpl::StopLoading()
{
    FetchWebContainer()->StopLoading();
}

void WebViewImpl::GoBack()
{
    FetchWebContainer()->GoBack();
}

void WebViewImpl::GoForward()
{
    FetchWebContainer()->GoForward();
}

bool WebViewImpl::CanGoBack()
{
    return FetchWebContainer()->CanGoBack();
}

bool WebViewImpl::CanGoForward()
{
    return FetchWebContainer()->CanGoForward();
}

void WebViewImpl::AddJavaScriptInterface(
    const std::string& exposedObjectName, const std::string& jsFunctionName,
    std::function<std::string(const std::string&)> cb)
{
    FetchWebContainer()->AddJavaScriptInterface(exposedObjectName,
                                                jsFunctionName, cb);
}

std::string WebViewImpl::EvaluateJavaScript(const std::string& script)
{
    return FetchWebContainer()->EvaluateJavaScript(script);
}

void WebViewImpl::EvaluateJavaScript(const std::string& script,
                                     std::function<void(const std::string&)> cb)
{
    return FetchWebContainer()->EvaluateJavaScript(script, cb);
}

void WebViewImpl::ClearHistory()
{
    FetchWebContainer()->ClearHistory();
}

void WebViewImpl::Destroy()
{
    FetchWebContainer()->Destroy();
    delete this;
}

void WebViewImpl::SetSettings(const Settings* settings)
{
    FetchWebContainer()->SetSettings(settings);
}

void WebViewImpl::RemoveJavascriptInterface(
    const std::string& exposedObjectName, const std::string& jsFunctionName)
{
    FetchWebContainer()->RemoveJavascriptInterface(exposedObjectName,
                                                   jsFunctionName);
}

void WebViewImpl::ClearCache()
{
    FetchWebContainer()->ClearCache();
}

void WebViewImpl::RegisterOnReceivedErrorHandler(
    std::function<void(WebView*, ResourceError*)> cb)
{
    FetchWebContainer()->RegisterOnReceivedErrorHandler(
        [this, cb](WebContainer*, ResourceError* err) { cb(this, err); });
}

void WebViewImpl::RegisterOnPageParsedHandler(
    std::function<void(WebView*, const std::string&)> cb)
{
    FetchWebContainer()->RegisterOnPageParsedHandler(
        [this, cb](WebContainer*, const std::string& a) { cb(this, a); });
}

void WebViewImpl::RegisterOnPageLoadedHandler(
    std::function<void(WebView*, const std::string&)> cb)
{
    FetchWebContainer()->RegisterOnPageLoadedHandler(
        [this, cb](WebContainer*, const std::string& a) { cb(this, a); });
}

void WebViewImpl::RegisterOnPageStartedHandler(
    std::function<void(WebView*, const std::string&)> cb)
{
    FetchWebContainer()->RegisterOnPageStartedHandler(
        [this, cb](WebContainer*, const std::string& a) { cb(this, a); });
}

void WebViewImpl::RegisterOnLoadResourceHandler(
    std::function<void(WebView*, const std::string&)> cb)
{
    FetchWebContainer()->RegisterOnLoadResourceHandler(
        [this, cb](WebContainer*, const std::string& a) { cb(this, a); });
}

void WebViewImpl::Pause()
{
    FetchWebContainer()->Pause();
}

void WebViewImpl::Resume()
{
    FetchWebContainer()->Resume();
}

void WebViewImpl::RegisterCustomFileResourceRequestHandlers(
    std::function<const char*(const char* path)> resolveFilePathCallback,
    std::function<void*(const char* path)> fileOpenCallback,
    std::function<size_t(uint8_t* destBuffer, size_t size, void* handle)>
        fileReadCallback,
    std::function<long int(void* handle)> fileLengthCallback,
    std::function<void(void* handle)> fileCloseCallback)
{
    FetchWebContainer()->RegisterCustomFileResourceRequestHandlers(
        resolveFilePathCallback, fileOpenCallback, fileReadCallback,
        fileLengthCallback, fileCloseCallback);
}

void WebViewImpl::RegisterDebuggerShouldInitHandler(
    const std::function<void(const std::string& url, int port, bool& ret)>& cb)
{
    FetchWebContainer()->RegisterDebuggerShouldInitHandler(cb);
}

void WebViewImpl::RegisterDebuggerShouldContinueWaitingHandler(
    const std::function<void(const std::string& url, int port, bool& ret)>& cb)
{
    FetchWebContainer()->RegisterDebuggerShouldContinueWaitingHandler(cb);
}

void WebViewImpl::SetUserData(const std::string& key, void* data)
{
    FetchWebContainer()->SetUserData(key, data);
}

void* WebViewImpl::GetUserData(const std::string& key)
{
    return FetchWebContainer()->GetUserData(key);
}

std::string WebViewImpl::GetTitle()
{
    return FetchWebContainer()->GetTitle();
}

void WebViewImpl::ScrollTo(int x, int y)
{
    FetchWebContainer()->ScrollTo(x, y);
}

void WebViewImpl::ScrollBy(int x, int y)
{
    FetchWebContainer()->ScrollBy(x, y);
}

int WebViewImpl::GetScrollX()
{
    return FetchWebContainer()->GetScrollX();
}

int WebViewImpl::GetScrollY()
{
    return FetchWebContainer()->GetScrollY();
}

void WebViewImpl::Focus()
{
    FetchWebContainer()->Focus();
}

void WebViewImpl::Blur()
{
    FetchWebContainer()->Blur();
}

void WebViewImpl::SetDevicePixelRatio(float dpr)
{
    FetchWebContainer()->SetDevicePixelRatio(dpr);
}

float WebViewImpl::GetDevicePixelRatio()
{
    return FetchWebContainer()->GetDevicePixelRatio();
}
} // namespace LWEDelegate
