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

#include "LWEWebView.h"

namespace LWE {

Settings WebView::GetSettings()
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

void WebView::ClearHistory()
{
    FetchWebContainer()->ClearHistory();
}

void WebView::Destroy()
{
    FetchWebContainer()->Destroy();
}

void WebView::SetSettings(const Settings& settings)
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
    std::function<void(LWE::WebView*, LWE::ResourceError)> cb)
{
    FetchWebContainer()->RegisterOnReceivedErrorHandler([this, cb](
        LWE::WebContainer*, LWE::ResourceError err) { cb(this, err); });
}

void WebView::RegisterOnPageParsedHandler(
    std::function<void(LWE::WebView*, const std::string&)> cb)
{
    FetchWebContainer()->RegisterOnPageParsedHandler(
        [this, cb](LWE::WebContainer*, const std::string& a) { cb(this, a); });
}

void WebView::RegisterOnPageLoadedHandler(
    std::function<void(LWE::WebView*, const std::string&)> cb)
{
    FetchWebContainer()->RegisterOnPageLoadedHandler(
        [this, cb](LWE::WebContainer*, const std::string& a) { cb(this, a); });
}

void WebView::RegisterOnPageStartedHandler(
    std::function<void(LWE::WebView*, const std::string&)> cb)
{
    FetchWebContainer()->RegisterOnPageStartedHandler(
        [this, cb](LWE::WebContainer*, const std::string& a) { cb(this, a); });
}

void WebView::RegisterOnLoadResourceHandler(
    std::function<void(LWE::WebView*, const std::string&)> cb)
{
    FetchWebContainer()->RegisterOnPageLoadedHandler(
        [this, cb](LWE::WebContainer*, const std::string& a) { cb(this, a); });
}

void WebView::RunMessageLoop()
{
    FetchWebContainer()->RunMessageLoop();
}

void WebView::StopMessageLoop()
{
    FetchWebContainer()->StopMessageLoop();
}
}
