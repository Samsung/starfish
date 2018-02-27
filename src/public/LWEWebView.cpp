/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "StarFishConfig.h"
#include "StarFish.h"

#include "LWEWebView.h"
#include "platform/window/PlatformWindow.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "core/page/WebView.h"
#include "core/page/History.h"
#include "core/page/Location.h"
#include "core/dom/Document.h"

#define TO_STARFISH(ptr) ((StarFish::StarFish*)ptr)
#define TO_HISTORY(ptr)         \
    ((StarFish::StarFish*)ptr)  \
        ->platformWindow()      \
        ->webView()             \
        ->mainBrowsingContext() \
        ->window()              \
        ->history()

#define TO_LOCATION(ptr)        \
    ((StarFish::StarFish*)ptr)  \
        ->platformWindow()      \
        ->webView()             \
        ->mainBrowsingContext() \
        ->window()              \
        ->location()

#define TO_RESOURCE_LOADER(ptr) \
    ((StarFish::StarFish*)ptr)  \
        ->platformWindow()      \
        ->webView()             \
        ->mainBrowsingContext() \
        ->document()            \
        ->resourceLoader()

namespace LWE {

Settings::Settings(std::string default_ua, std::string ua)
    : m_defaultUserAgent(default_ua)
    , m_UserAgent(ua)
{
}

std::string Settings::GetDefaultUserAgent()
{
    return m_defaultUserAgent;
}

std::string Settings::GetUserAgentString()
{
    return m_UserAgent;
}

void Settings::SetUserAgentString(std::string ua)
{
    m_UserAgent = ua;
}

ResourceError::ResourceError(int code, std::string description)
    : m_errorCode(code)
    , m_description(description)
{
}

int ResourceError::GetErrorCode()
{
    return m_errorCode;
}

std::string ResourceError::GetDescription()
{
    return m_description;
}

WebView* WebView::Create()
{
    // elm_init(0, 0);
    // elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);

    std::string screenShot;
    std::string customUserAgentString;
    std::string builtinPolyfillPathString;
    int width = 1280, height = 720;
    int x = 0, y = 0;
    int flag = 0;
    float scaleFactor = 1;

    StarFish::ScreenInfo info;
    info.rect.setWidth(width);
    info.rect.setHeight(height);
    info.availableRect.setWidth(width);
    info.availableRect.setHeight(height);
    info.deviceScaleFactor = scaleFactor;

    std::string cacheDir(getenv("HOME"));
    cacheDir += "/Starfish-cache";
    StarFish::StarFish* starfish = new StarFish::StarFish(
        (StarFish::StarFishStartUpFlag)flag, "ko-KR", "Asia/Seoul", nullptr,
        width, height, x, y, 1,
        StarFish::String::createASCIIString("sans-serif"), info,
        "/tmp/StarFish_localStorage.txt", "/tmp/StarFish_Cookies.txt",
        cacheDir.data(),
        StarFish::String::fromUTF8(customUserAgentString.data()),
        StarFish::String::fromUTF8(builtinPolyfillPathString.data()));

    return new WebView(starfish);
}

WebView* WebView::Create(void* starFish)
{
    return new WebView(starFish);
}

WebView::WebView(void* starFish)
    : m_starfish(starFish)
{
}

Settings WebView::GetSettings()
{
    STARFISH_ASSERT(m_starfish);
    std::string ua = TO_STARFISH(m_starfish)->userAgent()->toUTF8NonGCString();
    return Settings(USER_AGENT(STARFISH_NAME, VERSION), ua);
}

void WebView::LoadURL(std::string url)
{
    STARFISH_ASSERT(m_starfish);
    TO_STARFISH(m_starfish)
        ->loadHTMLDocument(StarFish::String::fromUTF8(url.data()));
}

std::string WebView::GetURL()
{
    STARFISH_ASSERT(m_starfish);
    return TO_LOCATION(m_starfish)->url()->urlString()->toUTF8NonGCString();
}

void WebView::LoadData(std::string data)
{
    STARFISH_ASSERT(m_starfish);
    unsigned int dataLength = data.size();
    // base64 encode
    if (data.size() > 0) {
        const char* originData = data.c_str();
        std::string dataURI = "data:text/html;charset=utf-8;base64,";
        std::string base64Chars =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        int i = 0, j = 0;
        unsigned char charArray3[3];
        unsigned char charArray4[4];

        while (dataLength--) {
            charArray3[i++] = *(originData++);
            if (i == 3) {
                charArray4[0] = (charArray3[0] & 0xfc) >> 2;
                charArray4[1] = ((charArray3[0] & 0x03) << 4) +
                                ((charArray3[1] & 0xf0) >> 4);
                charArray4[2] = ((charArray3[1] & 0x0f) << 2) +
                                ((charArray3[2] & 0xc0) >> 6);
                charArray4[3] = charArray3[2] & 0x3f;

                for (i = 0; (i < 4); i++) {
                    dataURI += base64Chars[charArray4[i]];
                }
                i = 0;
            }
        }

        if (i) {
            for (j = i; j < 3; j++) {
                charArray3[j] = '\0';
            }

            charArray4[0] = (charArray3[0] & 0xfc) >> 2;
            charArray4[1] =
                ((charArray3[0] & 0x03) << 4) + ((charArray3[1] & 0xf0) >> 4);
            charArray4[2] =
                ((charArray3[1] & 0x0f) << 2) + ((charArray3[2] & 0xc0) >> 6);
            charArray4[3] = charArray3[2] & 0x3f;

            for (j = 0; (j < i + 1); j++) {
                dataURI += base64Chars[charArray4[j]];
            }

            while ((i++ < 3)) {
                dataURI += '=';
            }
        }
        TO_STARFISH(m_starfish)
            ->loadHTMLDocument(StarFish::String::fromUTF8(dataURI.data()));
    }
}

void WebView::Reload()
{
    STARFISH_ASSERT(m_starfish);
    TO_LOCATION(m_starfish)->reload();
}

void WebView::StopLoading()
{
    STARFISH_ASSERT(m_starfish);
    TO_RESOURCE_LOADER(m_starfish).clear();
}

void WebView::GoBack()
{
    STARFISH_ASSERT(m_starfish);
    TO_HISTORY(m_starfish)->back();
}

void WebView::GoForward()
{
    STARFISH_ASSERT(m_starfish);
    TO_HISTORY(m_starfish)->forward();
}

bool WebView::CanGoBack()
{
    STARFISH_ASSERT(m_starfish);
    return TO_HISTORY(m_starfish)->canGoBack();
}

bool WebView::CanGoForward()
{
    STARFISH_ASSERT(m_starfish);
    return TO_HISTORY(m_starfish)->canGoForward();
}

void WebView::AddJavaScriptInterface(std::string exposedObjectName,
                                     std::string jsFunctionName,
                                     std::string (*cb)(std::string))
{
    STARFISH_ASSERT(m_starfish);
}

std::string WebView::EvaluateJavaScript(std::string script)
{
    STARFISH_ASSERT(m_starfish);
    return TO_STARFISH(m_starfish)
        ->evaluate(StarFish::String::createASCIIString(script.c_str()))
        ->toUTF8NonGCString();
}

void WebView::ClearHistory()
{
    STARFISH_ASSERT(m_starfish);
    TO_STARFISH(m_starfish)
        ->platformWindow()
        ->webView()
        ->historyManager()
        ->clear();
}

void WebView::Destroy()
{
    STARFISH_ASSERT(m_starfish);
    delete TO_STARFISH(m_starfish);
}

void WebView::SetSettings(LWE::Settings setttings)
{
    STARFISH_ASSERT(m_starfish);
    TO_STARFISH(m_starfish)
        ->setCustomUserAgentString(
            StarFish::String::fromUTF8(setttings.GetUserAgentString().c_str()));
}

void WebView::RemoveJavascriptInterface(std::string exposedObjectName,
                                        std::string jsFunctionName)
{
    STARFISH_ASSERT(m_starfish);
}

void WebView::SetWebViewClient(LWE::WebViewClient* client)
{
    m_webViewClient = client;

    TO_STARFISH(m_starfish)
        ->registerWebViewHandler(
            std::string("OnReceivedError"),
            [this](StarFish::String* url, int errorCode) -> void {
                // make error description
                this->m_webViewClient->OnReceivedError(
                    this, ResourceError(errorCode, std::string()));
            });

    TO_STARFISH(m_starfish)
        ->registerWebViewHandler(
            std::string("OnPageFinished"),
            [this](StarFish::String* url, int errorCode) -> void {
                this->m_webViewClient->OnPageFinished(this,
                                                      url->toUTF8NonGCString());
            });

    TO_STARFISH(m_starfish)
        ->registerWebViewHandler(
            std::string("OnPageStarted"),
            [this](StarFish::String* url, int errorCode) -> void {
                this->m_webViewClient->OnPageStarted(this,
                                                     url->toUTF8NonGCString());
            });

    TO_STARFISH(m_starfish)
        ->registerWebViewHandler(
            std::string("OnLoadResource"),
            [this](StarFish::String* url, int errorCode) -> void {
                this->m_webViewClient->OnLoadResource(this,
                                                      url->toUTF8NonGCString());
            });
}
}
