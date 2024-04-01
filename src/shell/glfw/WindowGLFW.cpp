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
#include <EGL/egl.h>

#include <memory>
#include <vector>

namespace StarfishShell {

class RendererDelegateGLFW : public RendererDelegate {
public:
    RendererDelegateGLFW(GLFWwindow* window);
    virtual ~RendererDelegateGLFW() = default;

    bool initialize();

    virtual bool makeCurrent() override;
    virtual bool clearCurrentContext() override;
    virtual bool swapBuffers() override;
    virtual uintptr_t createSharedContext() override;
    virtual bool destroyContext(uintptr_t context) override;
    virtual bool makeCurrentWithContext(uintptr_t context) override;

private:
    EGLDisplay m_eglDisplay = nullptr;
    EGLSurface m_eglSurface = nullptr;
    EGLContext m_eglContext = nullptr;
    EGLConfig m_eglConfig = nullptr;

    GLFWwindow* m_window = nullptr;
};

RendererDelegateGLFW::RendererDelegateGLFW(GLFWwindow* window)
    : m_window(window)
{
}

bool RendererDelegateGLFW::initialize()
{
    makeCurrent();
    EGLContext context = eglGetCurrentContext();

    if (!context) {
        printf("No attached context found.\n");
        exit(-1);
    }

    EGLDisplay display = eglGetCurrentDisplay();
    EGLSurface draw = eglGetCurrentSurface(EGL_DRAW);

    EGLConfig config = nullptr;
    EGLint configId, numConfigs, currentConfigId;
    eglQueryContext(display, context, EGL_CONFIG_ID, &configId);
    eglGetConfigs(display, nullptr, 0, &numConfigs);

    std::vector<EGLConfig> configs(numConfigs);
    eglGetConfigs(display, configs.data(), numConfigs, &numConfigs);
    for (const auto& c : configs) {
        eglGetConfigAttrib(display, c, EGL_CONFIG_ID, &currentConfigId);
        if (currentConfigId == configId) {
            config = c;
            break;
        }
    }

    if (!display || !draw || !config || !context) {
        exit(-1);
    }

    m_eglContext = context;
    m_eglDisplay = display;
    m_eglSurface = draw;
    m_eglConfig = config;

    clearCurrentContext();
    return true;
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
    EGLint attributes[] = { EGL_CONTEXT_MAJOR_VERSION, 3, EGL_NONE };
    EGLContext sharedContext =
        eglCreateContext(m_eglDisplay, m_eglConfig, m_eglContext, attributes);

    if (sharedContext == EGL_NO_CONTEXT) {
        EGLint attributes[] = { EGL_CONTEXT_MAJOR_VERSION, 2, EGL_NONE };
        sharedContext = eglCreateContext(m_eglDisplay, m_eglConfig,
                                         m_eglContext, attributes);
        if (sharedContext == EGL_NO_CONTEXT) {
            printf("Unable to create EGL context (eglError: 0x%x)\n",
                   eglGetError());
            return UINTPTR_MAX;
        }
    }

    return reinterpret_cast<uintptr_t>(sharedContext);
}

bool RendererDelegateGLFW::destroyContext(uintptr_t context)
{
    return eglDestroyContext(m_eglDisplay,
                             reinterpret_cast<EGLContext>(context));
}

bool RendererDelegateGLFW::makeCurrentWithContext(uintptr_t context)
{
    if (!eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface,
                        reinterpret_cast<EGLContext>(context))) {
        printf("Failed to set current context (eglError: 0x%x)", eglGetError());
        return false;
    }
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
    if (!m_renderer->initialize()) {
        return false;
    }

    return true;
}

bool WindowGLFW::createSimpleWindow(const char* appName, int width, int height)
{
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);

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
