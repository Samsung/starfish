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

#include <GL/glew.h>
#include <GL/wglew.h>
#pragma comment(lib, "Opengl32.lib")

namespace StarFish {

__declspec(thread) extern bool g_postLogMessageToThreadMessageQueue;

static void initError()
{
    MessageBoxA(NULL, "GLEW is not initialized!", "", 0);
    PostQuitMessage(-1);
}

HDC g_hDC;
HGLRC g_hrc;

extern "C" void __declspec(dllexport) __stdcall initGL(int* hWnd)
{
    // TODO add ReleaseDC
    HDC hDC = GetDC((HWND)hWnd);
    g_hDC = hDC;
    PIXELFORMATDESCRIPTOR pfd;
    memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 0;
    pfd.cStencilBits = 1;
    pfd.iLayerType = PFD_OVERLAY_PLANE;

    int nPixelFormat = ChoosePixelFormat((HDC)hDC, &pfd);

    if (nPixelFormat == 0)
        initError();

    BOOL bResult = SetPixelFormat((HDC)hDC, nPixelFormat, &pfd);

    if (!bResult)
        initError();

    HGLRC tempContext = wglCreateContext((HDC)hDC);
    wglMakeCurrent((HDC)hDC, tempContext);

    GLenum err = glewInit();
    if (GLEW_OK != err) {
        initError();
    }

    int attribs[] = { WGL_CONTEXT_MAJOR_VERSION_ARB,
                      3,
                      WGL_CONTEXT_MINOR_VERSION_ARB,
                      1,
                      WGL_CONTEXT_FLAGS_ARB,
                      0,
                      0 };

    if (wglewIsSupported("WGL_ARB_create_context") == 1) {
        g_hrc = wglCreateContextAttribsARB((HDC)hDC, 0, attribs);
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(tempContext);
        wglMakeCurrent((HDC)hDC, g_hrc);
    } else { // It's not possible to make a GL 3.x context. Use the old style
             // context (GL 2.1 and before)
        g_hrc = tempContext;
    }

    // Checking GL version
    const GLubyte* GLVersionString = glGetString(GL_VERSION);

    // Or better yet, use the GL3 way to get the version number
    int OpenGLVersion[2];
    glGetIntegerv(GL_MAJOR_VERSION, &OpenGLVersion[0]);
    glGetIntegerv(GL_MINOR_VERSION, &OpenGLVersion[1]);

    if (OpenGLVersion[0] < 3) {
        initError();
    }

    if (!g_hrc)
        initError();

    wglMakeCurrent(NULL, NULL);
}

extern "C" size_t STARFISH_EXPORT __stdcall createWebViewInstance(
    uint32_t initialWidth, uint32_t initialHeight)
{
    FcInitLoadConfig();
    g_postLogMessageToThreadMessageQueue = true;

    std::string localStorage = getWindowsTempDir();
    localStorage += "\\StarFishLocalStorage.txt";

    std::string cookieStorage = getWindowsTempDir();
    cookieStorage += "\\StarFishLocalCookie.txt";

    LWE::WebContainer* wv = LWE::WebContainer::CreateGL(
        initialWidth, initialHeight, 
        [](LWE::WebContainer*) { 
        wglMakeCurrent(g_hDC, g_hrc);
    },
        [](LWE::WebContainer*) { 
        SwapBuffers(g_hDC);
    }, 1, "ko-KR",
        "Asia/Seoul", localStorage.data(), cookieStorage.data(), "");
    wv->RegisterOnPageStartedHandler(
        [](LWE::WebContainer* wv, const std::string& url) {
            void* buffer = LocalAlloc(LMEM_FIXED, url.size() + 1);
            memcpy(buffer, url.data(), url.size());
            PostMessage(NULL, 0x0408, (WPARAM)buffer, url.size());
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

inline StarFish* fetchInstance(LWE::WebContainer* wv)
{
    static_assert(sizeof(LWE::WebContainer) == sizeof(void*));
    size_t* p = (size_t*)wv;
    return ((StarFish*)*p);
}

extern "C" void STARFISH_EXPORT __stdcall startMessageLoop(
    size_t webViewInstance)
{
    LWE::WebContainer* wv = (LWE::WebContainer*)webViewInstance;
    wv->RunMessageLoop();
}

extern "C" void STARFISH_EXPORT __stdcall stopMessageLoop(
    size_t webViewInstance)
{
    LWE::WebContainer* wv = (LWE::WebContainer*)webViewInstance;
    wv->StopMessageLoop();
}

extern "C" void STARFISH_EXPORT __stdcall resizeWindow(size_t webViewInstance,
                                                       uint32_t w, uint32_t h)
{
    LWE::WebContainer* wv = (LWE::WebContainer*)webViewInstance;
    wv->ResizeTo(w, h);
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
                                   : MouseButtonsValue::NoButtonDown,
                               x, y);
}

extern "C" void STARFISH_EXPORT __stdcall dispatchMouseWheelEvent(
    size_t webViewInstance, float x, float y, int delta)
{
    LWE::WebContainer* wv = (LWE::WebContainer*)webViewInstance;
    wv->DispatchMouseWheelEvent(x, y, delta);
}

KeyValue virtualKeyCodeToKeyValue(uint32_t vKeyCode,
                                  uint32_t capsLockOrShiftPressed)
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
    wv->DispatchKeyDownEvent(
        virtualKeyCodeToKeyValue(keyCode, capsLockOrShiftPressed));
}

extern "C" void STARFISH_EXPORT __stdcall dispatchKeyUpEvent(
    size_t webViewInstance, uint32_t keyCode, uint32_t capsLockOrShiftPressed)
{
    LWE::WebContainer* wv = (LWE::WebContainer*)webViewInstance;
    wv->DispatchKeyUpEvent(
        virtualKeyCodeToKeyValue(keyCode, capsLockOrShiftPressed));
}

extern "C" void STARFISH_EXPORT __stdcall dispatchCompositionStartEvent(
    size_t webViewInstance, size_t utf8Str, uint32_t strLength)
{
    LWE::WebContainer* wv = (LWE::WebContainer*)webViewInstance;
    wv->DispatchCompositionStartEvent(std::string((char*)utf8Str, strLength));
}

extern "C" void STARFISH_EXPORT __stdcall dispatchCompositionUpdateEvent(
    size_t webViewInstance, size_t utf8Str, uint32_t strLength)
{
    LWE::WebContainer* wv = (LWE::WebContainer*)webViewInstance;
    wv->DispatchCompositionUpdateEvent(std::string((char*)utf8Str, strLength));
}

extern "C" void STARFISH_EXPORT __stdcall dispatchCompositionEndEvent(
    size_t webViewInstance, size_t utf8Str, uint32_t strLength)
{
    LWE::WebContainer* wv = (LWE::WebContainer*)webViewInstance;
    wv->DispatchCompositionEndEvent(std::string((char*)utf8Str, strLength));
}

struct EvaluateJSResult {
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