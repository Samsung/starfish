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
class WinformWebViewClient : public LWE::WebViewClient {
public:
    virtual void OnPageFinished(LWE::WebView* view, std::string url)
    {
    }
    virtual void OnPageStarted(LWE::WebView* view, std::string url)
    {
        void* buffer = LocalAlloc(LMEM_FIXED, url.size() + 1);
        memcpy(buffer, url.data(), url.size());
        PostMessage(NULL, 0x0408, (WPARAM)buffer, url.size());
    }
    virtual void OnLoadResource(LWE::WebView* view, std::string url)
    {
    }
};

extern "C" size_t STARFISH_EXPORT __stdcall createWebViewInstance(
    uint32_t initialWidth, uint32_t initialHeight, void* initialBuffer, uint32_t initialBufferStride)
{
    FcInitLoadConfig();

    g_postLogMessageToThreadMessageQueue = true;
    int flag = 0;
    float scaleFactor = 1;

    ScreenInfo info;
    info.rect.setWidth(initialWidth);
    info.rect.setHeight(initialHeight);
    info.availableRect.setWidth(initialWidth);
    info.availableRect.setHeight(initialHeight);
    info.deviceScaleFactor = 1;

    char tempPath[MAX_PATH];
    GetTempPathA(MAX_PATH, tempPath);

    std::string localStoragePath = tempPath;
    localStoragePath += "\\StarFish_localStorage.txt";

    std::string cookiePath = tempPath;
    localStoragePath += "\\StarFish_Cookies.txt";

    GC_init();
    GC_disable();
    std::string cacheDir(tempPath);
    cacheDir += "\\Starfish-cache";
    StarFish* starFish = new (NoGC) StarFish(
        (StarFishStartUpFlag)flag, "ko-KR", "Asia/Seoul", nullptr, initialWidth,
        initialHeight, 0, 0, 1, String::createASCIIString("sans-serif"), info,
        localStoragePath.data(), cookiePath.data(), cacheDir.data(),
        String::emptyString,
        String::emptyString);

    starFish->platformWindow()->updateDrawingBufferAddress(initialBuffer, initialWidth, initialHeight, initialBufferStride);

    LWE::WebView* wv = LWE::WebView::Create(starFish);
    wv->SetWebViewClient(new WinformWebViewClient());
    return (size_t)wv;
}

extern "C" void STARFISH_EXPORT __stdcall loadURL(size_t webViewInstance,
                                                  size_t utf8URL,
                                                  uint32_t urlLength)
{
    LWE::WebView* wv = (LWE::WebView*)webViewInstance;
    std::string url((char*)utf8URL, urlLength);
    wv->LoadURL(url);
}

extern "C" void STARFISH_EXPORT __stdcall startMessageLoop(
    size_t webViewInstance)
{
    LWE::WebView* wv = (LWE::WebView*)webViewInstance;
    ((StarFish*)wv->getInternalPtr())->messageLoop()->run();
}

void processMessage(MessageLoop* self, const MSG& message);
inline StarFish* fetchInstance(LWE::WebView* wv)
{
    return ((StarFish*)wv->getInternalPtr());
}

extern "C" void STARFISH_EXPORT __stdcall giveMessage(size_t webViewInstance,
                                                      MSG msg)
{
    LWE::WebView* wv = (LWE::WebView*)webViewInstance;
    processMessage(fetchInstance(wv)->messageLoop(), msg);
}

extern "C" size_t STARFISH_EXPORT __stdcall internalDrawingBufferAddress(
    size_t webViewInstance)
{
    LWE::WebView* wv = (LWE::WebView*)webViewInstance;
    return (size_t)fetchInstance(wv)->platformWindow()->drawingBufferAddress();
}

extern "C" uint32_t STARFISH_EXPORT __stdcall internalDrawingBufferWidth(
    size_t webViewInstance)
{
    LWE::WebView* wv = (LWE::WebView*)webViewInstance;
    return fetchInstance(wv)->platformWindow()->drawingBufferWidth();
}

extern "C" uint32_t STARFISH_EXPORT __stdcall internalDrawingBufferHeight(
    size_t webViewInstance)
{
    LWE::WebView* wv = (LWE::WebView*)webViewInstance;
    return fetchInstance(wv)->platformWindow()->drawingBufferHeight();
}

extern "C" uint32_t STARFISH_EXPORT __stdcall internalDrawingBufferStride(
    size_t webViewInstance)
{
    LWE::WebView* wv = (LWE::WebView*)webViewInstance;
    return fetchInstance(wv)->platformWindow()->drawingBufferStride();
}

extern "C" void STARFISH_EXPORT __stdcall updateDrawingBufferAddress(
    size_t webViewInstance, uint32_t w, uint32_t h, void* buffer, uint32_t stride)
{
    LWE::WebView* wv = (LWE::WebView*)webViewInstance;
    fetchInstance(wv)->platformWindow()->updateDrawingBufferAddress(buffer, w, h, stride);
}

extern "C" uint32_t STARFISH_EXPORT __stdcall drawingBufferFrameNumber(
    size_t webViewInstance)
{
    LWE::WebView* wv = (LWE::WebView*)webViewInstance;
    return fetchInstance(wv)->platformWindow()->drawingBufferFrameNumber();
}


extern "C" void STARFISH_EXPORT __stdcall resizeWindow(size_t webViewInstance,
                                                       uint32_t w, uint32_t h, void* buffer, uint32_t stride)
{
    LWE::WebView* wv = (LWE::WebView*)webViewInstance;
    fetchInstance(wv)->platformWindow()->updateDrawingBufferAddress(buffer, w, h, stride);
}

extern "C" void STARFISH_EXPORT __stdcall dispatchMouseDownEvent(
    size_t webViewInstance, float x, float y)
{
    LWE::WebView* wv = (LWE::WebView*)webViewInstance;
    fetchInstance(wv)->platformWindow()->dispatchMouseEvent(
        MouseEventKind::MouseEventDown,
        MouseData(MouseData::MouseButtonValue::LeftButton,
                  MouseData::MouseButtonsValue::LeftButtonDown, x, y, 0));
}

extern "C" void STARFISH_EXPORT __stdcall dispatchMouseUpEvent(
    size_t webViewInstance, float x, float y)
{
    LWE::WebView* wv = (LWE::WebView*)webViewInstance;
    fetchInstance(wv)->platformWindow()->dispatchMouseEvent(
        MouseEventKind::MouseEventUp,
        MouseData(MouseData::MouseButtonValue::NoButton,
                  MouseData::MouseButtonsValue::NoButtonDown, x, y, 0));
}

extern "C" void STARFISH_EXPORT __stdcall dispatchMouseMoveEvent(
    size_t webViewInstance, float x, float y, bool isLButtonPressed,
    bool isRButtonPressed)
{
    LWE::WebView* wv = (LWE::WebView*)webViewInstance;
    fetchInstance(wv)->platformWindow()->dispatchMouseEvent(
        MouseEventKind::MouseEventMove,
        MouseData(isLButtonPressed ? MouseData::MouseButtonValue::LeftButton
                                   : MouseData::MouseButtonValue::NoButton,
                  isLButtonPressed
                      ? MouseData::MouseButtonsValue::LeftButtonDown
                      : MouseData::MouseButtonsValue::NoButtonDown,
                  x, y, 0));
}

extern "C" void STARFISH_EXPORT __stdcall dispatchMouseWheelEvent(
    size_t webViewInstance, float x, float y, int delta)
{
    LWE::WebView* wv = (LWE::WebView*)webViewInstance;
    fetchInstance(wv)->platformWindow()->dispatchMouseWheelEvent(x, y, delta, true);
}

KeyValue virtualKeyCodeToKeyValue(uint32_t vKeyCode)
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
        return KeyValue::UnidentifiedKey;
    }
}

extern "C" void STARFISH_EXPORT __stdcall dispatchKeyDownEvent(
    size_t webViewInstance, uint32_t keyCode)
{
    LWE::WebView* wv = (LWE::WebView*)webViewInstance;
    fetchInstance(wv)->platformWindow()->dispatchKeyEvent(KeyEventKind::KeyEventDown, PlatformKeyEventData(virtualKeyCodeToKeyValue(keyCode)));
}

extern "C" void STARFISH_EXPORT __stdcall dispatchKeyUpEvent(
    size_t webViewInstance, uint32_t keyCode)
{
    LWE::WebView* wv = (LWE::WebView*)webViewInstance;
    fetchInstance(wv)->platformWindow()->dispatchKeyEvent(KeyEventKind::KeyEventUp, PlatformKeyEventData(virtualKeyCodeToKeyValue(keyCode)));
}

struct EvaluateJSResult
{
    void* buf;
    size_t len;
};

extern "C" EvaluateJSResult STARFISH_EXPORT __stdcall evaluateJS(
    size_t webViewInstance, void* buffer, uint32_t len)
{
    LWE::WebView* wv = (LWE::WebView*)webViewInstance;
    std::string code((const char*)buffer, len);
    std::string result = wv->EvaluateJavaScript(code);
    EvaluateJSResult r;
    r.buf = LocalAlloc(LMEM_FIXED, result.length());
    memcpy(r.buf, result.data(), result.length());
    r.len = result.length();
    return r;
}

} // namespace StarFish