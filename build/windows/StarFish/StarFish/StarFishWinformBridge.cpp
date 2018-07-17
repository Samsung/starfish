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

#include "StarFishConfig.h"
#include "StarFish.h"
#include "LWEWebView.h"
#include "core/page/WebView.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "platform/window/PlatformWindow.h"
#include "core/dom/MouseEvent.h"
#include "core/event/KeyBoardEventData.h"
#include "platform/event/PlatformKeyEventData.h"
#include "LWEWebView.h"
#include <fontconfig/fontconfig.h>

namespace StarFish {

__declspec(thread) extern bool g_postLogMessageToThreadMessageQueue;

struct WinfomRenderResult {
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
};
__declspec(thread) WinfomRenderResult* g_renderResult;

extern "C" size_t STARFISH_EXPORT __stdcall getRenderResult(size_t webViewInstance)
{
    LWE::WebContainer* wv = (LWE::WebContainer*)webViewInstance;
    auto ret = g_renderResult;
    g_renderResult = nullptr;
    return (size_t)ret;
}

extern "C" size_t STARFISH_EXPORT __stdcall createWebViewInstance(
    uint32_t initialWidth, uint32_t initialHeight, void* initialBuffer, uint32_t initialBufferStride)
{
    FcInitLoadConfig();
    g_postLogMessageToThreadMessageQueue = true;

    std::string localStorage = getWindowsTempDir();
    localStorage += "\\StarFishLocalStorage.txt";

    std::string cookieStorage = getWindowsTempDir();
    cookieStorage += "\\StarFishLocalCookie.txt";

    LWE::WebContainer* wv = LWE::WebContainer::Create(initialBuffer, initialWidth, initialHeight, initialBufferStride, 1, "ko-KR", "Asia/Seoul", localStorage.data(), cookieStorage.data(), "");
    wv->RegisterOnPageStartedHandler([](LWE::WebContainer* wv, const std::string& url) {
        void* buffer = LocalAlloc(LMEM_FIXED, url.size() + 1);
        memcpy(buffer, url.data(), url.size());
        PostMessage(NULL, 0x0408, (WPARAM)buffer, url.size());
    });
    wv->RegisterOnRenderedHandler([](LWE::WebContainer* wv, const LWE::WebContainer::RenderResult& result) {
        WinfomRenderResult* r = new (LocalAlloc(LMEM_FIXED, sizeof (WinfomRenderResult))) WinfomRenderResult();
        r->x = result.updatedX;
        r->y = result.updatedY;
        r->width = result.updatedWidth;
        r->height = result.updatedHeight;
        g_renderResult = r;
    });
    return (size_t)wv;
}

extern "C" void STARFISH_EXPORT __stdcall loadURL(size_t webViewInstance,
                                                  size_t utf8URL,
                                                  uint32_t urlLength)
{
    LWE::WebContainer* wv = (LWE::WebContainer*)webViewInstance;
    std::string url((char*)utf8URL, urlLength);
    wv->LoadURL(url);
}

extern "C" void STARFISH_EXPORT __stdcall setProxyURL(size_t webViewInstance,
    size_t utf8URL,
    uint32_t urlLength)
{
    LWE::WebContainer* wv = (LWE::WebContainer*)webViewInstance;
    auto s = wv->GetSettings();
    std::string url((char*)utf8URL, urlLength);
    s.SetProxyURL(url);
    wv->SetSettings(s);
}

void processMessage(MessageLoop* self, const MSG& message);
inline StarFish* fetchInstance(LWE::WebContainer* wv)
{
    static_assert(sizeof(LWE::WebContainer) == sizeof(void*));
    size_t* p = (size_t*)wv;
    return ((StarFish*)*p);
}

extern "C" void STARFISH_EXPORT __stdcall giveMessage(size_t webViewInstance,
                                                      MSG msg)
{
    LWE::WebContainer* wv = (LWE::WebContainer*)webViewInstance;
    processMessage(fetchInstance(wv)->messageLoop(), msg);
}

extern "C" void STARFISH_EXPORT __stdcall updateDrawingBufferAddress(
    size_t webViewInstance, uint32_t w, uint32_t h, void* buffer, uint32_t stride)
{
    LWE::WebContainer* wv = (LWE::WebContainer*)webViewInstance;
    wv->UpdateBuffer(buffer, w, h, stride);
}

extern "C" void STARFISH_EXPORT __stdcall resizeWindow(size_t webViewInstance,
                                                       uint32_t w, uint32_t h, void* buffer, uint32_t stride)
{
    LWE::WebContainer* wv = (LWE::WebContainer*)webViewInstance;
    wv->UpdateBuffer(buffer, w, h, stride);
}

extern "C" void STARFISH_EXPORT __stdcall dispatchMouseDownEvent(
    size_t webViewInstance, float x, float y)
{
    LWE::WebContainer* wv = (LWE::WebContainer*)webViewInstance;
    wv->DispatchMouseDownEvent(MouseButtonValue::LeftButton,
                  MouseButtonsValue::LeftButtonDown, x, y);
}

extern "C" void STARFISH_EXPORT __stdcall dispatchMouseUpEvent(
    size_t webViewInstance, float x, float y)
{
    LWE::WebContainer* wv = (LWE::WebContainer*)webViewInstance;
    wv->DispatchMouseUpEvent(MouseButtonValue::NoButton,
        MouseButtonsValue::NoButtonDown, x, y);
}

extern "C" void STARFISH_EXPORT __stdcall dispatchMouseMoveEvent(
    size_t webViewInstance, float x, float y, bool isLButtonPressed,
    bool isRButtonPressed)
{
    LWE::WebContainer* wv = (LWE::WebContainer*)webViewInstance;
    wv->DispatchMouseMoveEvent(isLButtonPressed ? MouseButtonValue::LeftButton
        : MouseButtonValue::NoButton,
        isLButtonPressed
        ? MouseButtonsValue::LeftButtonDown
        : MouseButtonsValue::NoButtonDown, x, y);
}

extern "C" void STARFISH_EXPORT __stdcall dispatchMouseWheelEvent(
    size_t webViewInstance, float x, float y, int delta)
{
    LWE::WebContainer* wv = (LWE::WebContainer*)webViewInstance;
    wv->DispatchMouseWheelEvent(x, y, delta);
}

KeyValue virtualKeyCodeToKeyValue(uint32_t vKeyCode, uint32_t capsLockOrShiftPressed)
{
    switch (vKeyCode) {
    case VK_LEFT:
        return KeyValue::ArrowLeftKey;
    case VK_RIGHT:
        return KeyValue::ArrowRightKey;
    case VK_UP:
        return KeyValue::ArrowUpKey;
    case VK_DOWN:
        return KeyValue::ArrowDownKey;
    case VK_RETURN:
        return KeyValue::EnterKey;
    case VK_BACK:
        return KeyValue::BackspaceKey;
    default:
        UINT ch = MapVirtualKey(vKeyCode, MAPVK_VK_TO_CHAR);
        if (String::isASCIIPrintableKey(ch)) {
            if (isalpha(ch)) {
                if (!capsLockOrShiftPressed) {
                    ch = tolower(ch);
                }
            }
            return (KeyValue)ch;
        }
        return KeyValue::UnidentifiedKey;
    }
}

extern "C" void STARFISH_EXPORT __stdcall dispatchKeyDownEvent(
    size_t webViewInstance, uint32_t keyCode, uint32_t capsLockOrShiftPressed)
{
    LWE::WebContainer* wv = (LWE::WebContainer*)webViewInstance;
    wv->DispatchKeyDownEvent(virtualKeyCodeToKeyValue(keyCode, capsLockOrShiftPressed));
}

extern "C" void STARFISH_EXPORT __stdcall dispatchKeyUpEvent(
    size_t webViewInstance, uint32_t keyCode, uint32_t capsLockOrShiftPressed)
{
    LWE::WebContainer* wv = (LWE::WebContainer*)webViewInstance;
    wv->DispatchKeyUpEvent(virtualKeyCodeToKeyValue(keyCode, capsLockOrShiftPressed));
}

extern "C" void STARFISH_EXPORT __stdcall dispatchCompositionStartEvent(size_t webViewInstance,
    size_t utf8Str,
    uint32_t strLength)
{
    LWE::WebContainer* wv = (LWE::WebContainer*)webViewInstance;
    wv->DispatchCompositionStartEvent(std::string((char*)utf8Str, strLength));
}

extern "C" void STARFISH_EXPORT __stdcall dispatchCompositionUpdateEvent(size_t webViewInstance,
    size_t utf8Str,
    uint32_t strLength)
{
    LWE::WebContainer* wv = (LWE::WebContainer*)webViewInstance;
    wv->DispatchCompositionUpdateEvent(std::string((char*)utf8Str, strLength));
}

extern "C" void STARFISH_EXPORT __stdcall dispatchCompositionEndEvent(size_t webViewInstance,
    size_t utf8Str,
    uint32_t strLength)
{
    LWE::WebContainer* wv = (LWE::WebContainer*)webViewInstance;
    wv->DispatchCompositionEndEvent(std::string((char*)utf8Str, strLength));
}

struct EvaluateJSResult
{
    void* buf;
    size_t len;
};

extern "C" EvaluateJSResult STARFISH_EXPORT __stdcall evaluateJS(
    size_t webViewInstance, void* buffer, uint32_t len)
{
    LWE::WebContainer* wv = (LWE::WebContainer*)webViewInstance;
    std::string code((const char*)buffer, len);
    std::string result = wv->EvaluateJavaScript(code);
    EvaluateJSResult r;
    r.buf = LocalAlloc(LMEM_FIXED, result.length());
    memcpy(r.buf, result.data(), result.length());
    r.len = result.length();
    return r;
}

} // namespace StarFish