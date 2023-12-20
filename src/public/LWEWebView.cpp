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

#include "LWEWebView.h"
#include "LWEDelegate.h"
#include "ResourceErrorDelegate.h"
#include "SettingsDelegate.h"

namespace LWE {

template <typename T>
T* toImpl(void* ptr)
{
    return static_cast<T*>(ptr);
}

void LWE::Initialize(const char* localStorageDataFilePath,
                     const char* cookieStoreDataFilePath,
                     const char* httpCacheDataDirectorypath)
{
    LWEDelegate::LWE::Initialize(localStorageDataFilePath,
                                 cookieStoreDataFilePath,
                                 httpCacheDataDirectorypath);
}

bool LWE::IsInitialized()
{
    return LWEDelegate::LWE::IsInitialized();
}

void LWE::Finalize()
{
    LWEDelegate::LWE::Finalize();
}

unsigned char LWE::GetGCFrequency()
{
    return LWEDelegate::LWE::GetGCFrequency();
}

void LWE::SetGCFrequency(unsigned char freq)
{
    LWEDelegate::LWE::SetGCFrequency(freq);
}

ResourceError::ResourceError(int code, const std::string& description,
                             const std::string& url)
{
    m_delegate = new LWEDelegate::ResourceError(code, description, url);
}

ResourceError::~ResourceError()
{
    delete toImpl<LWEDelegate::ResourceError>(m_delegate);
}

int ResourceError::GetErrorCode()
{
    return toImpl<LWEDelegate::ResourceError>(m_delegate)->GetErrorCode();
}

std::string ResourceError::GetDescription()
{
    return toImpl<LWEDelegate::ResourceError>(m_delegate)->GetDescription();
}

std::string ResourceError::GetUrl()
{
    return toImpl<LWEDelegate::ResourceError>(m_delegate)->GetUrl();
}

Settings::Settings(const std::string& defaultUA, const std::string& ua)
{
    m_delegate = new LWEDelegate::Settings(defaultUA, ua);
}

Settings::~Settings()
{
    delete toImpl<LWEDelegate::Settings>(m_delegate);
}

bool Settings::UpdateSetting(std::string key, std::string value)
{
    return toImpl<LWEDelegate::Settings>(m_delegate)->UpdateSetting(key, value);
}

std::string Settings::GetSetting(std::string key) const
{
    return toImpl<LWEDelegate::Settings>(m_delegate)->GetSetting(key);
}

std::string Settings::GetDefaultUserAgent() const
{
    return toImpl<LWEDelegate::Settings>(m_delegate)->GetDefaultUserAgent();
}

std::string Settings::GetUserAgentString() const
{
    return toImpl<LWEDelegate::Settings>(m_delegate)->GetUserAgentString();
}

std::string Settings::GetProxyURL() const
{
    return toImpl<LWEDelegate::Settings>(m_delegate)->GetProxyURL();
}

int Settings::GetCacheMode() const
{
    return toImpl<LWEDelegate::Settings>(m_delegate)->GetCacheMode();
}

TTSMode Settings::GetTTSMode() const
{
    return toImpl<LWEDelegate::Settings>(m_delegate)->GetTTSMode();
}

std::string Settings::GetTTSLanguage() const
{
    return toImpl<LWEDelegate::Settings>(m_delegate)->GetTTSLanguage();
}

WebSecurityMode Settings::GetWebSecurityMode() const
{
    return toImpl<LWEDelegate::Settings>(m_delegate)->GetWebSecurityMode();
}

IdleModeJob Settings::GetIdleModeJob() const
{
    return toImpl<LWEDelegate::Settings>(m_delegate)->GetIdleModeJob();
}

uint32_t Settings::GetIdleModeCheckIntervalInMS() const
{
    return toImpl<LWEDelegate::Settings>(m_delegate)
        ->GetIdleModeCheckIntervalInMS();
}

void Settings::GetBaseBackgroundColor(unsigned char& r, unsigned char& g,
                                      unsigned char& b, unsigned char& a) const
{
    return toImpl<LWEDelegate::Settings>(m_delegate)
        ->GetBaseBackgroundColor(r, g, b, a);
}

void Settings::GetBaseForegroundColor(unsigned char& r, unsigned char& g,
                                      unsigned char& b, unsigned char& a) const
{
    return toImpl<LWEDelegate::Settings>(m_delegate)
        ->GetBaseForegroundColor(r, g, b, a);
}

bool Settings::NeedsDownloadWebFontsEarly() const
{
    return toImpl<LWEDelegate::Settings>(m_delegate)
        ->NeedsDownloadWebFontsEarly();
}

bool Settings::UseHttp2() const
{
    return toImpl<LWEDelegate::Settings>(m_delegate)->UseHttp2();
}

uint32_t Settings::NeedsDownScaleImageResourceLargerThan() const
{
    return toImpl<LWEDelegate::Settings>(m_delegate)
        ->NeedsDownScaleImageResourceLargerThan();
}

bool Settings::ScrollbarVisible() const
{
    return toImpl<LWEDelegate::Settings>(m_delegate)->ScrollbarVisible();
}

bool Settings::UseExternalPopup() const
{
    return toImpl<LWEDelegate::Settings>(m_delegate)->UseExternalPopup();
}

bool Settings::UseSpatialNavigation() const
{
    return toImpl<LWEDelegate::Settings>(m_delegate)->UseSpatialNavigation();
}

void Settings::SetUserAgentString(const std::string& ua)
{
    toImpl<LWEDelegate::Settings>(m_delegate)->SetUserAgentString(ua);
}

void Settings::SetCacheMode(int mode)
{
    toImpl<LWEDelegate::Settings>(m_delegate)->SetCacheMode(mode);
}

void Settings::SetProxyURL(const std::string& proxyURL)
{
    toImpl<LWEDelegate::Settings>(m_delegate)->SetProxyURL(proxyURL);
}

void Settings::setDefaultFontSize(int size)
{
    toImpl<LWEDelegate::Settings>(m_delegate)->setDefaultFontSize(size);
}

void Settings::SetTTSMode(TTSMode value)
{
    toImpl<LWEDelegate::Settings>(m_delegate)->SetTTSMode(value);
}

void Settings::SetTTSLanguage(const std::string& language)
{
    toImpl<LWEDelegate::Settings>(m_delegate)->SetTTSLanguage(language);
}

void Settings::SetBaseBackgroundColor(unsigned char r, unsigned char g,
                                      unsigned char b, unsigned char a)
{
    toImpl<LWEDelegate::Settings>(m_delegate)
        ->SetBaseBackgroundColor(r, g, b, a);
}

void Settings::SetBaseForegroundColor(unsigned char r, unsigned char g,
                                      unsigned char b, unsigned char a)
{
    toImpl<LWEDelegate::Settings>(m_delegate)
        ->SetBaseForegroundColor(r, g, b, a);
}

void Settings::SetWebSecurityMode(WebSecurityMode value)
{
    toImpl<LWEDelegate::Settings>(m_delegate)->SetWebSecurityMode(value);
}

void Settings::SetIdleModeJob(IdleModeJob j)
{
    toImpl<LWEDelegate::Settings>(m_delegate)->SetIdleModeJob(j);
}

void Settings::SetIdleModeCheckIntervalInMS(uint32_t intervalInMS)
{
    toImpl<LWEDelegate::Settings>(m_delegate)
        ->SetIdleModeCheckIntervalInMS(intervalInMS);
}

void Settings::SetNeedsDownloadWebFontsEarly(bool b)
{
    toImpl<LWEDelegate::Settings>(m_delegate)->SetNeedsDownloadWebFontsEarly(b);
}

void Settings::SetUseHttp2(bool b)
{
    toImpl<LWEDelegate::Settings>(m_delegate)->SetUseHttp2(b);
}

void Settings::SetNeedsDownScaleImageResourceLargerThan(uint32_t demention)
{
    toImpl<LWEDelegate::Settings>(m_delegate)
        ->SetNeedsDownScaleImageResourceLargerThan(demention);
}

void Settings::SetScrollbarVisible(bool visible)
{
    toImpl<LWEDelegate::Settings>(m_delegate)->SetScrollbarVisible(visible);
}

void Settings::SetUseExternalPopup(bool useExternalPopup)
{
    toImpl<LWEDelegate::Settings>(m_delegate)
        ->SetUseExternalPopup(useExternalPopup);
}

void Settings::SetUseSpatialNavigation(bool useSpatialNavigation)
{
    toImpl<LWEDelegate::Settings>(m_delegate)
        ->SetUseSpatialNavigation(useSpatialNavigation);
}

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
    std::function<void(WebView*, ResourceError)> cb)
{
    FetchWebContainer()->RegisterOnReceivedErrorHandler(
        [this, cb](WebContainer*, ResourceError err) { cb(this, err); });
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
} // namespace LWE
