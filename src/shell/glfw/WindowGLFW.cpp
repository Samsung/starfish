/*
 * Copyright (c) 2024-present Samsung Electronics Co., Ltd
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

#include "ShellConfig.h"

#if defined(STARFISH_SHELL_GLFW)

#include "Window.h"

#define GLFW_INCLUDE_ES3
#include <GLFW/glfw3.h>

#include <memory>
#include <vector>

namespace StarfishShell {

class RendererDelegateGLFW : public RendererDelegate {
public:
    RendererDelegateGLFW(GLFWwindow* window);
    virtual ~RendererDelegateGLFW() = default;

    virtual bool makeCurrent() override;
    virtual bool clearCurrentContext() override;
    virtual bool swapBuffers() override;
    virtual uintptr_t createSharedContext() override;
    virtual bool destroyContext(uintptr_t context) override;
    virtual bool makeCurrentWithContext(uintptr_t context) override;
    virtual void* getProcAddress(const char* name) override
    {
        return nullptr;
    }

private:
    GLFWwindow* m_window = nullptr;
};

RendererDelegateGLFW::RendererDelegateGLFW(GLFWwindow* window)
    : m_window(window)
{
}

bool RendererDelegateGLFW::makeCurrent()
{
    glfwMakeContextCurrent(m_window);
    return true;
}

bool RendererDelegateGLFW::clearCurrentContext()
{
    glfwMakeContextCurrent(nullptr);
    return true;
}

bool RendererDelegateGLFW::swapBuffers()
{
    glfwSwapBuffers(m_window);
    return true;
}

uintptr_t RendererDelegateGLFW::createSharedContext()
{
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    GLFWwindow* sharedContext = glfwCreateWindow(1, 1, "", nullptr, m_window);
    return reinterpret_cast<uintptr_t>(sharedContext);
}

bool RendererDelegateGLFW::destroyContext(uintptr_t context)
{
    glfwDestroyWindow(reinterpret_cast<GLFWwindow*>(context));
    return true;
}

bool RendererDelegateGLFW::makeCurrentWithContext(uintptr_t context)
{
    glfwMakeContextCurrent(reinterpret_cast<GLFWwindow*>(context));
    return true;
}

class WindowGLFW final : public Window {
public:
    WindowGLFW();
    bool init(const char* appName, int width, int height) override;
    void pollEvent() override;
    void terminate() override;
    void getCursorPos(double& xpos, double& ypos) override;
    void* getNativeWindowHandle() override
    {
        return nullptr;
    }

    virtual RendererDelegate* renderer()
    {
        return m_renderer.get();
    }

private:
    bool createSimpleWindow(const char* appName, int width, int height);
    void setEventHandlers();

    GLFWwindow* m_window = nullptr;
    std::unique_ptr<RendererDelegateGLFW> m_renderer;
};

WindowGLFW::WindowGLFW()
{
}

bool WindowGLFW::init(const char* appName, int width, int height)
{
    glfwSetErrorCallback([](int error, const char* description) {
        printf("%s\n", description);
    });

    if (!glfwInit()) {
        exit(-1);
    }

    printf("GLFW_VERSION: %s\n", glfwGetVersionString());

    if (!m_isVisible) {
        glfwWindowHint(GLFW_VISIBLE, 0);
    }

    if (!createSimpleWindow(appName, width, height)) {
        exit(-1);
    }

    setEventHandlers();

#if defined(STARFISH_ENABLE_TEST)
    // for screen shot
    glfwSwapInterval(0);
#endif

    m_renderer = std::unique_ptr<RendererDelegateGLFW>(
        new RendererDelegateGLFW(m_window));

    return true;
}

bool WindowGLFW::createSimpleWindow(const char* appName, int width, int height)
{
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);

    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    m_window = glfwCreateWindow(width, height, appName, nullptr, nullptr);
    if (m_window == nullptr) {
        printf(
            "Failed to create OpenGL 3.0  context. try OpenGL ES 3.0 "
            "instead");
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
        m_window = glfwCreateWindow(width, height, appName, nullptr, nullptr);
        if (m_window == nullptr) {
            printf(
                "Failed to create OpenGL ES 3.0 context. please check your "
                "environment...");
            return false;
        }
    }

    glfwSetWindowSize(m_window, width, height);

    return true;
}

void WindowGLFW::setEventHandlers()
{
    glfwSetWindowUserPointer(m_window, this);

    glfwSetCursorPosCallback(
        m_window, [](GLFWwindow* window, double xpos, double ypos) {
            WindowGLFW* winGLFW = (WindowGLFW*)glfwGetWindowUserPointer(window);
            if (winGLFW->m_motionEventHandler) {
                winGLFW->m_motionEventHandler(xpos, ypos);
            }
        });

    glfwSetMouseButtonCallback(
        m_window, [](GLFWwindow* window, int button, int action, int mods) {
            WindowGLFW* winGLFW = (WindowGLFW*)glfwGetWindowUserPointer(window);
            if (winGLFW->m_buttonEventHandler) {
                if (button == GLFW_MOUSE_BUTTON_LEFT) {
                    winGLFW->m_buttonEventHandler(
                        INPUT::MOUSE_LBUTTON,
                        action == GLFW_PRESS ? INPUT::PRESS : INPUT::RELEASE);
                }
            }
        });

    glfwSetScrollCallback(
        m_window, [](GLFWwindow* window, double xoffset, double yoffset) {
            WindowGLFW* winGLFW = (WindowGLFW*)glfwGetWindowUserPointer(window);
            if (winGLFW->m_scrollEventHandler) {
                double xpos, ypos;
                glfwGetCursorPos(window, &xpos, &ypos);
                winGLFW->m_scrollEventHandler(xpos, ypos, -yoffset);
            }
        });

    glfwSetWindowSizeCallback(
        m_window, [](GLFWwindow* window, int width, int height) {
            WindowGLFW* winGLFW = (WindowGLFW*)glfwGetWindowUserPointer(window);
            if (winGLFW->m_windowSizeEventHandler) {
                winGLFW->m_windowSizeEventHandler(width, height);
            }
        });

    glfwSetKeyCallback(m_window, [](GLFWwindow* window, int key, int scancode,
                                    int action, int mods) {
        WindowGLFW* winGLFW = (WindowGLFW*)glfwGetWindowUserPointer(window);
        INPUT type = (action == GLFW_PRESS) ? INPUT::PRESS : INPUT::RELEASE;
        if (winGLFW->m_keyEventHandler) {
            winGLFW->m_keyEventHandler(key, type, mods);
        }
    });
    glfwSetWindowCloseCallback(m_window, [](GLFWwindow* window) {
        WindowGLFW* winGLFW = (WindowGLFW*)glfwGetWindowUserPointer(window);
        if (winGLFW->m_exitEventHandler) {
            winGLFW->m_exitEventHandler();
        }
    });
}

void WindowGLFW::getCursorPos(double& xpos, double& ypos)
{
    glfwGetCursorPos(m_window, &xpos, &ypos);
}

void WindowGLFW::pollEvent()
{
    glfwPollEvents();
}

void WindowGLFW::terminate()
{
    m_renderer = nullptr;
    glfwDestroyWindow(m_window);
}

Window* Window::create()
{
    return new WindowGLFW();
}

LWE::KeyValue Window::convertKeyCode(const unsigned long key, INPUT action,
                                     unsigned mods)
{
    LWE::KeyValue keyValue = LWE::KeyValue::UnidentifiedKey;

    switch (key) {
    case GLFW_KEY_ESCAPE:
        keyValue = LWE::KeyValue::EscapeKey;
        break;
    case GLFW_KEY_ENTER:
        keyValue = LWE::KeyValue::EnterKey;
        break;
    case GLFW_KEY_SPACE:
        keyValue = LWE::KeyValue::SpaceKey;
        break;
    case GLFW_KEY_BACKSPACE:
        keyValue = LWE::KeyValue::BackspaceKey;
        break;
    case GLFW_KEY_LEFT:
        keyValue = LWE::KeyValue::ArrowLeftKey;
        break;
    case GLFW_KEY_RIGHT:
        keyValue = LWE::KeyValue::ArrowRightKey;
        break;
    case GLFW_KEY_DOWN:
        keyValue = LWE::KeyValue::ArrowDownKey;
        break;
    case GLFW_KEY_UP:
        keyValue = LWE::KeyValue::ArrowUpKey;
        break;
    case GLFW_KEY_LEFT_SHIFT:
        keyValue = LWE::KeyValue::ShiftLeftKey;
        break;
    case GLFW_KEY_RIGHT_SHIFT:
        keyValue = LWE::KeyValue::ShiftRightKey;
        break;
    default:
        if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z) {
            keyValue =
                (LWE::KeyValue)(LWE::KeyValue::LowerAKey + key - GLFW_KEY_A);
            if (mods & GLFW_MOD_SHIFT)
                keyValue =
                    (LWE::KeyValue)(keyValue + (LWE::AKey - LWE::LowerAKey));
        } else if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9) {
            keyValue =
                (LWE::KeyValue)(LWE::KeyValue::Digit0Key + key - GLFW_KEY_0);
            if (mods & GLFW_MOD_SHIFT) {
                switch (keyValue) {
                case LWE::KeyValue::Digit1Key:
                    keyValue = LWE::KeyValue::ExclamationMarkKey;
                    break;
                case LWE::KeyValue::Digit2Key:
                    keyValue = LWE::KeyValue::AtMarkKey;
                    break;
                case LWE::KeyValue::Digit3Key:
                    keyValue = LWE::KeyValue::SharpMarkKey;
                    break;
                case LWE::KeyValue::Digit4Key:
                    keyValue = LWE::KeyValue::DollarMarkKey;
                    break;
                case LWE::KeyValue::Digit5Key:
                    keyValue = LWE::KeyValue::PercentMarkKey;
                    break;
                case LWE::KeyValue::Digit6Key:
                    keyValue = LWE::KeyValue::CaretMarkKey;
                    break;
                case LWE::KeyValue::Digit7Key:
                    keyValue = LWE::KeyValue::AmpersandMarkKey;
                    break;
                case LWE::KeyValue::Digit8Key:
                    keyValue = LWE::KeyValue::AsteriskMarkKey;
                    break;
                case LWE::KeyValue::Digit9Key:
                    keyValue = LWE::KeyValue::LeftParenthesisMarkKey;
                    break;
                case LWE::KeyValue::Digit0Key:
                    keyValue = LWE::KeyValue::RightParenthesisMarkKey;
                    break;
                default:
                    break;
                }
            }
        } else
            keyValue = LWE::KeyValue::UnidentifiedKey;
        break;
    }
    return keyValue;
}
} // namespace StarfishShell

#endif
