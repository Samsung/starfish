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

#include "LWEWebViewDelegate.h"

#include "SettingsDelegate.h"
#include "ResourceErrorDelegate.h"
#include "LWEWebContainerDelegate.h"

namespace LWEDelegate {

Settings* WebView::GetSettings()
{
    return FetchWebContainer()->GetSettings();
}

void WebView::LoadURL(const std::string& url)
{
    FetchWebContainer()->LoadURL(url);
}

std::string WebView::GetURL()
{
    return FetchWebContainer()->GetURL();
}

void WebView::LoadData(const std::string& data)
{
    FetchWebContainer()->LoadData(data);
}

void WebView::Reload()
{
    FetchWebContainer()->Reload();
}

void WebView::StopLoading()
{
    FetchWebContainer()->StopLoading();
}

void WebView::GoBack()
{
    FetchWebContainer()->GoBack();
}

void WebView::GoForward()
{
    FetchWebContainer()->GoForward();
}

bool WebView::CanGoBack()
{
    return FetchWebContainer()->CanGoBack();
}

bool WebView::CanGoForward()
{
    return FetchWebContainer()->CanGoForward();
}

void WebView::AddJavaScriptInterface(
    const std::string& exposedObjectName, const std::string& jsFunctionName,
    std::function<std::string(const std::string&)> cb)
{
    FetchWebContainer()->AddJavaScriptInterface(exposedObjectName,
                                                jsFunctionName, cb);
}

std::string WebView::EvaluateJavaScript(const std::string& script)
{
    return FetchWebContainer()->EvaluateJavaScript(script);
}

void WebView::EvaluateJavaScript(const std::string& script,
                                 std::function<void(const std::string&)> cb)
{
    return FetchWebContainer()->EvaluateJavaScript(script, cb);
}

void WebView::ClearHistory()
{
    FetchWebContainer()->ClearHistory();
}

void WebView::Destroy()
{
    FetchWebContainer()->Destroy();
    delete this;
}

void WebView::SetSettings(const Settings* settings)
{
    FetchWebContainer()->SetSettings(settings);
}

void WebView::RemoveJavascriptInterface(const std::string& exposedObjectName,
                                        const std::string& jsFunctionName)
{
    FetchWebContainer()->RemoveJavascriptInterface(exposedObjectName,
                                                   jsFunctionName);
}

void WebView::ClearCache()
{
    FetchWebContainer()->ClearCache();
}

void WebView::RegisterOnReceivedErrorHandler(
    std::function<void(WebView*, ResourceError*)> cb)
{
    FetchWebContainer()->RegisterOnReceivedErrorHandler(
        [this, cb](WebContainer*, ResourceError* err) { cb(this, err); });
}

void WebView::RegisterOnPageParsedHandler(
    std::function<void(WebView*, const std::string&)> cb)
{
    FetchWebContainer()->RegisterOnPageParsedHandler(
        [this, cb](WebContainer*, const std::string& a) { cb(this, a); });
}

void WebView::RegisterOnPageLoadedHandler(
    std::function<void(WebView*, const std::string&)> cb)
{
    FetchWebContainer()->RegisterOnPageLoadedHandler(
        [this, cb](WebContainer*, const std::string& a) { cb(this, a); });
}

void WebView::RegisterOnPageStartedHandler(
    std::function<void(WebView*, const std::string&)> cb)
{
    FetchWebContainer()->RegisterOnPageStartedHandler(
        [this, cb](WebContainer*, const std::string& a) { cb(this, a); });
}

void WebView::RegisterOnLoadResourceHandler(
    std::function<void(WebView*, const std::string&)> cb)
{
    FetchWebContainer()->RegisterOnLoadResourceHandler(
        [this, cb](WebContainer*, const std::string& a) { cb(this, a); });
}

void WebView::Pause()
{
    FetchWebContainer()->Pause();
}

void WebView::Resume()
{
    FetchWebContainer()->Resume();
}

void WebView::RegisterCustomFileResourceRequestHandlers(
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

void WebView::RegisterDebuggerShouldInitHandler(
    const std::function<void(const std::string& url, int port, bool& ret)>& cb)
{
    FetchWebContainer()->RegisterDebuggerShouldInitHandler(cb);
}

void WebView::RegisterDebuggerShouldContinueWaitingHandler(
    const std::function<void(const std::string& url, int port, bool& ret)>& cb)
{
    FetchWebContainer()->RegisterDebuggerShouldContinueWaitingHandler(cb);
}

void WebView::SetUserData(const std::string& key, void* data)
{
    FetchWebContainer()->SetUserData(key, data);
}

void* WebView::GetUserData(const std::string& key)
{
    return FetchWebContainer()->GetUserData(key);
}

std::string WebView::GetTitle()
{
    return FetchWebContainer()->GetTitle();
}

void WebView::ScrollTo(int x, int y)
{
    FetchWebContainer()->ScrollTo(x, y);
}

void WebView::ScrollBy(int x, int y)
{
    FetchWebContainer()->ScrollBy(x, y);
}

int WebView::GetScrollX()
{
    return FetchWebContainer()->GetScrollX();
}

int WebView::GetScrollY()
{
    return FetchWebContainer()->GetScrollY();
}

void WebView::Focus()
{
    FetchWebContainer()->Focus();
}

void WebView::Blur()
{
    FetchWebContainer()->Blur();
}

void WebView::SetDevicePixelRatio(float dpr)
{
    FetchWebContainer()->SetDevicePixelRatio(dpr);
}

float WebView::GetDevicePixelRatio()
{
    return FetchWebContainer()->GetDevicePixelRatio();
}
} // namespace LWEDelegate
