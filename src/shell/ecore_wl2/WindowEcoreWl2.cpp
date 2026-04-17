/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_SHELL_ECORE_WL2)

#include "Window.h"

#include <Ecore.h>
#include <Ecore_Wl2.h>
#include <Ecore_Input.h>
#include <EGL/egl.h>

#include <GLES2/gl2.h>

#include <memory>
#include <cstring>
#include <csignal>

namespace {

static void sigintHandler(int signum)
{
    ecore_main_loop_quit();
}

bool createEGLDisplay(EGLDisplay eglDisplay, EGLConfig& config)
{
    EGLint eglVersionMajor, eglVersionMinor;
    if (!eglInitialize(eglDisplay, &eglVersionMajor, &eglVersionMinor)) {
        printf("Unable to initialize EGL\n");
        return false;
    }

    // Set the current rendering API to OpenGL ES API
    eglBindAPI(EGL_OPENGL_ES_API);

    // Get frame buffer configuration supported
    EGLConfig eglConfig;
    {
        EGLint numConfig;
        EGLint configSize = 1;
        EGLint attributes[] = {
            EGL_SURFACE_TYPE,
            EGL_WINDOW_BIT,
            EGL_RED_SIZE,
            8,
            EGL_GREEN_SIZE,
            8,
            EGL_BLUE_SIZE,
            8,
            EGL_ALPHA_SIZE,
            8,
            EGL_DEPTH_SIZE,
            0,
            EGL_STENCIL_SIZE,
            0,
            EGL_SAMPLES,
            0,
            EGL_RENDERABLE_TYPE,
            EGL_OPENGL_ES2_BIT,
            EGL_NONE,
        };

        if (!eglChooseConfig(eglDisplay, attributes, &eglConfig, configSize,
                             &numConfig)) {
            printf("Failed to choose config (eglError: %d)\n", eglGetError());
            return false;
        }
        if (numConfig != configSize) {
            printf("Didn't get exactly one config, but %d\n", numConfig);
            return false;
        }
    }

    config = eglConfig;

    return true;
}

bool createEGLSurface(EGLSurface& surface, const EGLDisplay& eglDisplay,
                      const EGLConfig& eglConfig, Ecore_Wl2_Window* window,
                      Ecore_Wl2_Egl_Window* eglWindow)
{
    EGLSurface eglSurface;
    {
        EGLint attributes[] = { EGL_NONE };
        const auto eglNativeWindow = ecore_wl2_egl_window_native_get(eglWindow);
        eglSurface = eglCreateWindowSurface(
            eglDisplay, eglConfig, (EGLNativeWindowType)(eglNativeWindow),
            attributes);
        if (eglSurface == EGL_NO_SURFACE) {
            printf("Unable to create EGL surface (eglError: 0x%x)\n",
                   eglGetError());
            return false;
        }
    }

    surface = eglSurface;

    return true;
}

bool createGLContext(EGLContext& context, const EGLDisplay eglDisplay,
                     const EGLConfig eglConfig, const EGLContext shareContext)
{
    EGLint attributes[] = { EGL_CONTEXT_MAJOR_VERSION, 3, EGL_NONE };
    EGLContext eglContext =
        eglCreateContext(eglDisplay, eglConfig, shareContext, attributes);

    if (eglContext == EGL_NO_CONTEXT) {
        EGLint attributes[] = { EGL_CONTEXT_MAJOR_VERSION, 2, EGL_NONE };
        eglContext =
            eglCreateContext(eglDisplay, eglConfig, shareContext, attributes);

        if (eglContext == EGL_NO_CONTEXT) {
            printf("Unable to create EGL context (eglError: 0x%x)\n",
                   eglGetError());
            return false;
        }
    }

    context = eglContext;
    return true;
}

} // namespace

namespace StarfishShell {

class RendererDelegateEGL : public RendererDelegate {
public:
    RendererDelegateEGL() = default;
    virtual ~RendererDelegateEGL() = default;

    bool initialize(Ecore_Wl2_Window* window, Ecore_Wl2_Egl_Window* eglWindow,
                    Ecore_Wl2_Display* display);
    void deinitialize();
    bool resizeSurface(Ecore_Wl2_Window* window,
                       Ecore_Wl2_Egl_Window* eglWindow);

    virtual bool makeCurrent() override;
    virtual bool clearCurrentContext() override;
    virtual bool swapBuffers() override;
    virtual uintptr_t createSharedContext() override;
    virtual bool destroyContext(uintptr_t context) override;
    virtual bool makeCurrentWithContext(uintptr_t context) override;
    virtual void* getProcAddress(const char* name) override;
    virtual bool isSupportedExtension(const char* extension) override;

private:
    EGLDisplay m_eglDisplay = nullptr;
    EGLSurface m_eglSurface = nullptr;
    EGLContext m_eglContext = nullptr;
    EGLConfig m_eglConfig = nullptr;
    Ecore_Wl2_Window* m_window = nullptr;
};

bool RendererDelegateEGL::initialize(Ecore_Wl2_Window* window,
                                     Ecore_Wl2_Egl_Window* eglWindow,
                                     Ecore_Wl2_Display* display)
{
    m_window = window;
    m_eglDisplay = eglGetDisplay(ecore_wl2_display_get(display));
    if (!createEGLDisplay(m_eglDisplay, m_eglConfig) ||
        !createEGLSurface(m_eglSurface, m_eglDisplay, m_eglConfig, window,
                          eglWindow) ||
        !createGLContext(m_eglContext, m_eglDisplay, m_eglConfig, nullptr)) {
        return false;
    }
    return true;
}

void RendererDelegateEGL::deinitialize()
{
    eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE,
                   EGL_NO_CONTEXT);
    if (m_eglSurface != EGL_NO_SURFACE) {
        eglDestroySurface(m_eglDisplay, m_eglSurface);
        m_eglSurface = EGL_NO_SURFACE;
    }
    if (m_eglContext != EGL_NO_CONTEXT) {
        eglDestroyContext(m_eglDisplay, m_eglContext);
        m_eglContext = EGL_NO_CONTEXT;
    }
    if (m_eglDisplay != EGL_NO_DISPLAY) {
        eglTerminate(m_eglDisplay);
        m_eglDisplay = EGL_NO_DISPLAY;
    }
}

bool RendererDelegateEGL::resizeSurface(Ecore_Wl2_Window* window,
                                        Ecore_Wl2_Egl_Window* eglWindow)
{
    // Make context current with no surface before destroying old surface
    eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE,
                   EGL_NO_CONTEXT);

    // Destroy old surface
    if (m_eglSurface != EGL_NO_SURFACE) {
        eglDestroySurface(m_eglDisplay, m_eglSurface);
        m_eglSurface = EGL_NO_SURFACE;
    }

    // Create new surface with the new window size
    if (!createEGLSurface(m_eglSurface, m_eglDisplay, m_eglConfig, window,
                          eglWindow)) {
        return false;
    }

    // Make the context current again with the new surface
    if (!eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface,
                        m_eglContext)) {
        printf("Failed to make context current after resize (eglError: 0x%x)\n",
               eglGetError());
        return false;
    }

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);
    eglSwapBuffers(m_eglDisplay, m_eglSurface);

    return true;
}

bool RendererDelegateEGL::makeCurrent()
{
    if (!eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface,
                        m_eglContext)) {
        printf("Failed to set current context (eglError: 0x%x)\n",
               eglGetError());
        return false;
    }
    return true;
}

bool RendererDelegateEGL::swapBuffers()
{
    return eglSwapBuffers(m_eglDisplay, m_eglSurface);
}

uintptr_t RendererDelegateEGL::createSharedContext()
{
    EGLContext sharedContext;
    if (createGLContext(sharedContext, m_eglDisplay, m_eglConfig,
                        m_eglContext)) {
        return reinterpret_cast<uintptr_t>(sharedContext);
    }
    return UINTPTR_MAX;
}

bool RendererDelegateEGL::destroyContext(uintptr_t context)
{
    return eglDestroyContext(m_eglDisplay,
                             reinterpret_cast<EGLContext>(context));
}

bool RendererDelegateEGL::clearCurrentContext()
{
    return eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE,
                          EGL_NO_CONTEXT);
}

bool RendererDelegateEGL::makeCurrentWithContext(uintptr_t context)
{
    if (!eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface,
                        reinterpret_cast<EGLContext>(context))) {
        printf("Failed to set current context (eglError: 0x%x)\n",
               eglGetError());
        return false;
    }
    return true;
}

void* RendererDelegateEGL::getProcAddress(const char* name)
{
    return reinterpret_cast<void*>(eglGetProcAddress(name));
}

bool RendererDelegateEGL::isSupportedExtension(const char* extension)
{
    const char* extensions = eglQueryString(m_eglDisplay, EGL_EXTENSIONS);
    return strstr(extensions, extension) != nullptr;
}

class WindowEcoreWl2 final : public Window {
public:
    WindowEcoreWl2();
    ~WindowEcoreWl2();

    bool init(const char* appName, int width, int height) override;
    void terminate() override;
    void getCursorPos(double& xpos, double& ypos) override;

    void* getNativeWindowHandle() override
    {
        return nullptr;
    }

    virtual RendererDelegate* renderer() override
    {
        return m_renderer.get();
    }

private:
    void setupEventHandlers();

    Ecore_Wl2_Display* m_display = nullptr;
    Ecore_Wl2_Window* m_window = nullptr;
    Ecore_Wl2_Egl_Window* m_eglWindow = nullptr;
    std::unique_ptr<RendererDelegateEGL> m_renderer;
    Ecore_Event_Handler* m_keyDownHandler = nullptr;
    Ecore_Event_Handler* m_keyUpHandler = nullptr;
    Ecore_Event_Handler* m_mouseDownHandler = nullptr;
    Ecore_Event_Handler* m_mouseUpHandler = nullptr;
    Ecore_Event_Handler* m_mouseMoveHandler = nullptr;
    Ecore_Event_Handler* m_mouseWheelHandler = nullptr;
    Ecore_Event_Handler* m_windowResizeHandler = nullptr;
    Ecore_Event_Handler* m_windowDeleteHandler = nullptr;
};

WindowEcoreWl2::WindowEcoreWl2()
{
}

WindowEcoreWl2::~WindowEcoreWl2()
{
}

bool WindowEcoreWl2::init(const char* appName, int width, int height)
{
    // Setup SIGINT signal handler
    std::signal(SIGINT, sigintHandler);

    // Initialize Ecore_Wl2
    if (!ecore_wl2_init()) {
        printf("Cannot initialize ecore_wl2\n");
        return false;
    }
    ecore_main_loop_iterate();

    // Connect to Wayland display
    m_display = ecore_wl2_display_connect(NULL);
    if (!m_display) {
        printf("Failed to connect to Wayland display\n");
        ecore_wl2_shutdown();
        return false;
    }
    ecore_main_loop_iterate();

    // Create window
    m_window = ecore_wl2_window_new(m_display, NULL, 0, 0, width, height);
    if (!m_window) {
        fprintf(stderr, "Failed to create window\n");
        ecore_wl2_display_disconnect(m_display);
        ecore_wl2_shutdown();
        return false;
    }
    ecore_main_loop_iterate();

    // Set window properties
    ecore_wl2_window_title_set(m_window, "StarfishShell");
    ecore_main_loop_iterate();
    ecore_wl2_window_type_set(m_window, ECORE_WL2_WINDOW_TYPE_NOTIFICATION);
    ecore_main_loop_iterate();

    m_eglWindow = ecore_wl2_egl_window_create(m_window, width, height);
    ecore_main_loop_iterate();

    if (m_isVisible) {
        ecore_wl2_window_show(m_window);
    } else {
        ecore_wl2_window_hide(m_window);
    }
    ecore_main_loop_iterate();

    // Setup event handlers
    setupEventHandlers();

    // Initialize EGL renderer
    m_renderer =
        std::unique_ptr<RendererDelegateEGL>(new RendererDelegateEGL());
    if (!m_renderer->initialize(m_window, m_eglWindow, m_display)) {
        ecore_wl2_window_free(m_window);
        ecore_wl2_display_disconnect(m_display);
        ecore_wl2_shutdown();
        return false;
    }

    return true;
}

void WindowEcoreWl2::setupEventHandlers()
{
    // Keyboard key down handler
    m_keyDownHandler = ecore_event_handler_add(
        ECORE_EVENT_KEY_DOWN,
        [](void* data, int type, void* event) -> Eina_Bool {
            WindowEcoreWl2* win = static_cast<WindowEcoreWl2*>(data);
            Ecore_Event_Key* keyEvent = static_cast<Ecore_Event_Key*>(event);

            if (win->m_keyEventHandler && keyEvent->keyname) {
                unsigned long keycode = 0;
                const char* keyname = keyEvent->keyname;

                // Convert key name to keycode
                if (strcmp(keyname, "Left") == 0) {
                    keycode = static_cast<unsigned long>(INPUT::LEFT);
                } else if (strcmp(keyname, "Up") == 0) {
                    keycode = static_cast<unsigned long>(INPUT::UP);
                } else if (strcmp(keyname, "Right") == 0) {
                    keycode = static_cast<unsigned long>(INPUT::RIGHT);
                } else if (strcmp(keyname, "Down") == 0) {
                    keycode = static_cast<unsigned long>(INPUT::DOWN);
                } else if (strlen(keyname) == 1) {
                    keycode = static_cast<unsigned long>(keyname[0]);
                } else {
                    keycode = keyEvent->keycode;
                }

                unsigned mods = 0;
                if (keyEvent->modifiers & ECORE_EVENT_MODIFIER_SHIFT) {
                    mods |= static_cast<unsigned>(MOD::SHIFT);
                }
                if (keyEvent->modifiers & ECORE_EVENT_MODIFIER_CTRL) {
                    mods |= static_cast<unsigned>(MOD::CONTROL);
                }

                win->m_keyEventHandler(keycode, INPUT::PRESS, mods);
            }
            return ECORE_CALLBACK_PASS_ON;
        },
        this);

    // Keyboard key up handler
    m_keyUpHandler = ecore_event_handler_add(
        ECORE_EVENT_KEY_UP,
        [](void* data, int type, void* event) -> Eina_Bool {
            WindowEcoreWl2* win = static_cast<WindowEcoreWl2*>(data);
            Ecore_Event_Key* keyEvent = static_cast<Ecore_Event_Key*>(event);

            if (win->m_keyEventHandler && keyEvent->keyname) {
                unsigned long keycode = 0;
                const char* keyname = keyEvent->keyname;

                // Convert key name to keycode
                if (strcmp(keyname, "Left") == 0) {
                    keycode = static_cast<unsigned long>(INPUT::LEFT);
                } else if (strcmp(keyname, "Up") == 0) {
                    keycode = static_cast<unsigned long>(INPUT::UP);
                } else if (strcmp(keyname, "Right") == 0) {
                    keycode = static_cast<unsigned long>(INPUT::RIGHT);
                } else if (strcmp(keyname, "Down") == 0) {
                    keycode = static_cast<unsigned long>(INPUT::DOWN);
                } else if (strlen(keyname) == 1) {
                    keycode = static_cast<unsigned long>(keyname[0]);
                } else {
                    keycode = keyEvent->keycode;
                }

                unsigned mods = 0;
                if (keyEvent->modifiers & ECORE_EVENT_MODIFIER_SHIFT) {
                    mods |= static_cast<unsigned>(MOD::SHIFT);
                }
                if (keyEvent->modifiers & ECORE_EVENT_MODIFIER_CTRL) {
                    mods |= static_cast<unsigned>(MOD::CONTROL);
                }

                win->m_keyEventHandler(keycode, INPUT::RELEASE, mods);
            }
            return ECORE_CALLBACK_PASS_ON;
        },
        this);

    // Mouse button down handler
    m_mouseDownHandler = ecore_event_handler_add(
        ECORE_EVENT_MOUSE_BUTTON_DOWN,
        [](void* data, int type, void* event) -> Eina_Bool {
            WindowEcoreWl2* win = static_cast<WindowEcoreWl2*>(data);
            Ecore_Event_Mouse_Button* buttonEvent =
                static_cast<Ecore_Event_Mouse_Button*>(event);

            if (win->m_buttonEventHandler) {
                if (buttonEvent->buttons == 1) {
                    win->m_buttonEventHandler(INPUT::MOUSE_LBUTTON,
                                              INPUT::PRESS);
                }
            }
            return ECORE_CALLBACK_PASS_ON;
        },
        this);

    // Mouse button up handler
    m_mouseUpHandler = ecore_event_handler_add(
        ECORE_EVENT_MOUSE_BUTTON_UP,
        [](void* data, int type, void* event) -> Eina_Bool {
            WindowEcoreWl2* win = static_cast<WindowEcoreWl2*>(data);
            Ecore_Event_Mouse_Button* buttonEvent =
                static_cast<Ecore_Event_Mouse_Button*>(event);

            if (win->m_buttonEventHandler) {
                if (buttonEvent->buttons == 1) {
                    win->m_buttonEventHandler(INPUT::MOUSE_LBUTTON,
                                              INPUT::RELEASE);
                }
            }
            return ECORE_CALLBACK_PASS_ON;
        },
        this);

    // Mouse move handler
    m_mouseMoveHandler = ecore_event_handler_add(
        ECORE_EVENT_MOUSE_MOVE,
        [](void* data, int type, void* event) -> Eina_Bool {
            WindowEcoreWl2* win = static_cast<WindowEcoreWl2*>(data);
            Ecore_Event_Mouse_Move* moveEvent =
                static_cast<Ecore_Event_Mouse_Move*>(event);

            if (win->m_motionEventHandler) {
                win->m_motionEventHandler(moveEvent->x, moveEvent->y);
            }
            return ECORE_CALLBACK_PASS_ON;
        },
        this);

    // Mouse wheel handler
    m_mouseWheelHandler = ecore_event_handler_add(
        ECORE_EVENT_MOUSE_WHEEL,
        [](void* data, int type, void* event) -> Eina_Bool {
            WindowEcoreWl2* win = static_cast<WindowEcoreWl2*>(data);
            Ecore_Event_Mouse_Wheel* wheelEvent =
                static_cast<Ecore_Event_Mouse_Wheel*>(event);

            if (win->m_scrollEventHandler) {
                double xpos, ypos;
                win->getCursorPos(xpos, ypos);
                win->m_scrollEventHandler(xpos, ypos, wheelEvent->z);
            }
            return ECORE_CALLBACK_PASS_ON;
        },
        this);

    // Window resize handler
    m_windowResizeHandler = ecore_event_handler_add(
        ECORE_WL2_EVENT_WINDOW_CONFIGURE_COMPLETE,
        [](void* data, int type, void* event) -> Eina_Bool {
            WindowEcoreWl2* win = static_cast<WindowEcoreWl2*>(data);
            Ecore_Wl2_Event_Window_Configure* configureEvent =
                static_cast<Ecore_Wl2_Event_Window_Configure*>(event);

            if (configureEvent->win ==
                (unsigned int)ecore_wl2_window_id_get(win->m_window)) {
                // Resize EGL surface to match new window size
                int x = 0, y = 0, w = 0, h = 0;
                ecore_wl2_window_geometry_get(win->m_window, &x, &y, &w, &h);
                ecore_wl2_egl_window_resize_with_rotation(win->m_eglWindow, x,
                                                          y, w, h, 0);
                if (win->m_renderer) {
                    win->m_renderer->resizeSurface(win->m_window,
                                                   win->m_eglWindow);
                }
                if (win->m_windowSizeEventHandler) {
                    win->m_windowSizeEventHandler(w, h);
                }
            }
            return ECORE_CALLBACK_PASS_ON;
        },
        this);

    // Window delete handler
    m_windowDeleteHandler = ecore_event_handler_add(
        ECORE_WL2_EVENT_WINDOW_DESTROY,
        [](void* data, int type, void* event) -> Eina_Bool {
            WindowEcoreWl2* win = static_cast<WindowEcoreWl2*>(data);
            Ecore_Wl2_Event_Window_Deactivate* deactivateEvent =
                static_cast<Ecore_Wl2_Event_Window_Deactivate*>(event);

            if (deactivateEvent->win ==
                (unsigned int)ecore_wl2_window_id_get(win->m_window)) {
                if (win->m_exitEventHandler) {
                    win->m_exitEventHandler();
                }
            }
            return ECORE_CALLBACK_PASS_ON;
        },
        this);
}

void WindowEcoreWl2::getCursorPos(double& xpos, double& ypos)
{
    int win_x = 0, win_y = 0;

    ecore_wl2_input_pointer_xy_get(ecore_wl2_input_default_input_get(m_display),
                                   &win_x, &win_y);

    xpos = win_x;
    ypos = win_y;
}

void WindowEcoreWl2::terminate()
{
    // Reset SIGINT signal handler to default
    std::signal(SIGINT, SIG_DFL);

    // Clean up event handlers
    if (m_keyDownHandler) {
        ecore_event_handler_del(m_keyDownHandler);
        m_keyDownHandler = nullptr;
    }
    if (m_keyUpHandler) {
        ecore_event_handler_del(m_keyUpHandler);
        m_keyUpHandler = nullptr;
    }
    if (m_mouseDownHandler) {
        ecore_event_handler_del(m_mouseDownHandler);
        m_mouseDownHandler = nullptr;
    }
    if (m_mouseUpHandler) {
        ecore_event_handler_del(m_mouseUpHandler);
        m_mouseUpHandler = nullptr;
    }
    if (m_mouseMoveHandler) {
        ecore_event_handler_del(m_mouseMoveHandler);
        m_mouseMoveHandler = nullptr;
    }
    if (m_mouseWheelHandler) {
        ecore_event_handler_del(m_mouseWheelHandler);
        m_mouseWheelHandler = nullptr;
    }
    if (m_windowResizeHandler) {
        ecore_event_handler_del(m_windowResizeHandler);
        m_windowResizeHandler = nullptr;
    }
    if (m_windowDeleteHandler) {
        ecore_event_handler_del(m_windowDeleteHandler);
        m_windowDeleteHandler = nullptr;
    }

    m_renderer->deinitialize();
    m_renderer = nullptr;

    ecore_wl2_egl_window_destroy(m_eglWindow);
    ecore_wl2_window_free(m_window);
    ecore_wl2_display_disconnect(m_display);
    ecore_wl2_shutdown();

    m_window = nullptr;
    m_display = nullptr;
}

Window* Window::create()
{
    return new WindowEcoreWl2();
}

LWE::KeyValue Window::convertKeyCode(const unsigned long key, INPUT action,
                                     unsigned mods)
{
    switch (static_cast<ASCII>(key)) {
    case ASCII::HT:
        return LWE::KeyValue::TabKey;
    case ASCII::BS:
        return LWE::KeyValue::BackspaceKey;
    case ASCII::CR:
        return LWE::KeyValue::EnterKey;
    case ASCII::ESC:
        return LWE::KeyValue::EscapeKey;
    case ASCII::DEL:
        return LWE::KeyValue::DeleteKey;
    default:
        break;
    }
    switch (static_cast<INPUT>(key)) {
    case INPUT::LEFT:
        return LWE::KeyValue::ArrowLeftKey;
    case INPUT::UP:
        return LWE::KeyValue::ArrowUpKey;
    case INPUT::RIGHT:
        return LWE::KeyValue::ArrowRightKey;
    case INPUT::DOWN:
        return LWE::KeyValue::ArrowDownKey;
    default:
        break;
    }
    return static_cast<LWE::KeyValue>(key);
}

} // namespace StarfishShell

#endif
