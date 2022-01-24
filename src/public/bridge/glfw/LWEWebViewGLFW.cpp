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

#include "StarfishConfig.h"
#include "LWEWebView.h"

#include "Starfish.h"

#if defined(PORT_WEBVIEW_BRIDGE_GLFW)

#define GLFW_INCLUDE_ES3
#include <GLFW/glfw3.h>

namespace LWE {

static void error_callback(int error, const char* description)
{
    STARFISH_LOG_ERROR("%s", description);
}

static void cursor_position_callback(GLFWwindow* window, double xpos,
                                     double ypos);
static void mouse_button_callback(GLFWwindow* window, int button, int action,
                                  int mods);
static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
static void window_size_callback(GLFWwindow* window, int width, int height);
static void key_callback(GLFWwindow* window, int key, int scancode, int action,
                         int mods);
static void poller(void* data);

class WebViewGLFW : public WebView {
public:
    WebViewGLFW(void* winArg, unsigned x, unsigned y, unsigned width,
                unsigned height, float devicePixelRatio,
                const char* defaultFontName, const char* locale,
                const char* timezoneID)
        : WebView(nullptr)
    {
        glfwSetErrorCallback(error_callback);
        if (!glfwInit())
            exit(-1);

        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#if defined(STARFISH_ENABLE_TEST)
        {
            const char* path = getenv("SCREEN_SHOT");
            const char* hide = getenv("HIDE_WINDOW");
            if ((path && strlen(path)) || (hide && strlen(hide))) {
                glfwWindowHint(GLFW_VISIBLE, 0);
            }
        }
#endif
        m_glWindow = glfwCreateWindow(width, height, "Starfish", NULL, NULL);
        if (m_glWindow == nullptr) {
            STARFISH_LOG_ERROR(
                "failed to create OpenGL 3.0  context. try OpenGL ES 3.0 "
                "instead");
            glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
            m_glWindow =
                glfwCreateWindow(width, height, "Starfish", NULL, NULL);
            if (m_glWindow == nullptr) {
                STARFISH_LOG_ERROR(
                    "failed to create OpenGL ES 3.0 context. please check your "
                    "environment...");
                STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
            }
        }

        glfwSetWindowPos(m_glWindow, x, y);
        glfwSetWindowSize(m_glWindow, width, height);

        glfwMakeContextCurrent(m_glWindow);

        glfwSetWindowUserPointer(m_glWindow, this);
        glfwSetCursorPosCallback(m_glWindow, cursor_position_callback);
        glfwSetMouseButtonCallback(m_glWindow, mouse_button_callback);
        glfwSetScrollCallback(m_glWindow, scroll_callback);
        glfwSetWindowSizeCallback(m_glWindow, window_size_callback);
        glfwSetKeyCallback(m_glWindow, key_callback);

#if defined(STARFISH_ENABLE_TEST)
        // for screen shot
        glfwSwapInterval(0);
#endif

        m_isMouseLbuttonDown = false;

        glfwMakeContextCurrent(nullptr);

        ::LWE::WebContainer* webContainer = ::LWE::WebContainer::CreateGL(
            width, height,
            [this](WebContainer* wc) { glfwMakeContextCurrent(m_glWindow); },
            [this](WebContainer* wc, bool mayNeedsSync) {
                glfwSwapBuffers(m_glWindow);
            },
            devicePixelRatio, defaultFontName, locale, timezoneID);

        m_impl = webContainer;

        m_pollTimer = webContainer->AddTimeout(poller, this, 10);
    }

    virtual void Destroy() override
    {
        FetchWebContainer()->Blur();

        FetchWebContainer()->ClearTimeout(m_pollTimer);

        FetchWebContainer()->Destroy();

        glfwDestroyWindow(m_glWindow);

        // FIXME memory of <this> pointer is leaking now
        this->~WebView();
    }

    bool m_isMouseLbuttonDown;
    size_t m_pollTimer;
    GLFWwindow* m_glWindow;

    virtual ::LWE::WebContainer* FetchWebContainer() override
    {
        return (::LWE::WebContainer*)m_impl;
    }
};

static void poller(void* data)
{
    WebViewGLFW* self = (WebViewGLFW*)data;
    glfwPollEvents();
    self->m_pollTimer = self->FetchWebContainer()->AddTimeout(poller, self, 25);
}

static void cursor_position_callback(GLFWwindow* window, double xpos,
                                     double ypos)
{
    WebViewGLFW* wnd = (WebViewGLFW*)glfwGetWindowUserPointer(window);
    MouseButtonsValue buttons = wnd->m_isMouseLbuttonDown
                                    ? MouseButtonsValue::LeftButtonDown
                                    : MouseButtonsValue::NoButtonDown;
    wnd->FetchWebContainer()->DispatchMouseMoveEvent(MouseButtonValue::NoButton,
                                                     buttons, xpos, ypos);
}

static void mouse_button_callback(GLFWwindow* window, int button, int action,
                                  int mods)
{
    WebViewGLFW* wnd = (WebViewGLFW*)glfwGetWindowUserPointer(window);
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        MouseButtonsValue buttons = wnd->m_isMouseLbuttonDown
                                        ? MouseButtonsValue::LeftButtonDown
                                        : MouseButtonsValue::NoButtonDown;
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        if (action == GLFW_PRESS) {
            wnd->m_isMouseLbuttonDown = true;
            wnd->FetchWebContainer()->DispatchMouseDownEvent(
                MouseButtonValue::NoButton, buttons, xpos, ypos);
        } else {
            wnd->m_isMouseLbuttonDown = false;
            wnd->FetchWebContainer()->DispatchMouseUpEvent(
                MouseButtonValue::NoButton, buttons, xpos, ypos);
        }
    }
}

static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    WebViewGLFW* wnd = (WebViewGLFW*)glfwGetWindowUserPointer(window);
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    wnd->FetchWebContainer()->DispatchMouseWheelEvent(xpos, ypos, -yoffset);
}

static void window_size_callback(GLFWwindow* window, int width, int height)
{
    WebViewGLFW* wnd = (WebViewGLFW*)glfwGetWindowUserPointer(window);
    wnd->FetchWebContainer()->ResizeTo(width, height);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action,
                         int mods)
{
    KeyValue keyValue = KeyValue::UnidentifiedKey;

    switch (key) {
    case GLFW_KEY_ESCAPE:
        keyValue = KeyValue::EscapeKey;
        break;
    case GLFW_KEY_ENTER:
        keyValue = KeyValue::EnterKey;
        break;
    case GLFW_KEY_SPACE:
        keyValue = KeyValue::SpaceKey;
        break;
    case GLFW_KEY_BACKSPACE:
        keyValue = KeyValue::BackspaceKey;
        break;
    case GLFW_KEY_LEFT:
        keyValue = KeyValue::ArrowLeftKey;
        break;
    case GLFW_KEY_RIGHT:
        keyValue = KeyValue::ArrowRightKey;
        break;
    case GLFW_KEY_DOWN:
        keyValue = KeyValue::ArrowDownKey;
        break;
    case GLFW_KEY_UP:
        keyValue = KeyValue::ArrowUpKey;
        break;
    case GLFW_KEY_LEFT_SHIFT:
        keyValue = KeyValue::ShiftLeftKey;
        break;
    case GLFW_KEY_RIGHT_SHIFT:
        keyValue = KeyValue::ShiftRightKey;
        break;
    default:
        if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z) {
            keyValue = (KeyValue)(KeyValue::LowerAKey + key - GLFW_KEY_A);
            if (mods & GLFW_MOD_SHIFT)
                keyValue = (KeyValue)(keyValue + (AKey - LowerAKey));
        } else if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9) {
            keyValue = (KeyValue)(KeyValue::Digit0Key + key - GLFW_KEY_0);
        } else
            keyValue = KeyValue::UnidentifiedKey;
        break;
    }

    WebViewGLFW* wnd = (WebViewGLFW*)glfwGetWindowUserPointer(window);
    if (action == GLFW_PRESS) {
        wnd->FetchWebContainer()->DispatchKeyDownEvent(keyValue);
        wnd->FetchWebContainer()->DispatchKeyPressEvent(keyValue);
    } else {
        wnd->FetchWebContainer()->DispatchKeyUpEvent(keyValue);
    }
}

WebView* WebView::Create(void* win, unsigned x, unsigned y, unsigned width,
                         unsigned height, float devicePixelRatio,
                         const char* defaultFontName, const char* locale,
                         const char* timezoneID)
{
    return new WebViewGLFW(win, x, y, width, height, devicePixelRatio,
                           defaultFontName, locale, timezoneID);
}
} // namespace LWE

#endif
