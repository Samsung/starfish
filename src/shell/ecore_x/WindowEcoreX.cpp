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

#if defined(STARFISH_SHELL_ECORE_X)

#include "Window.h"

#include <Ecore.h>
#include <Ecore_X.h>
#include <Ecore_Input.h>
#include <Ecore_IMF.h>
#include <Evas.h>
#include <EGL/egl.h>

#include <memory>
#include <cstring>
#include <csignal>

namespace {

static void sigintHandler(int signum)
{
    ecore_main_loop_quit();
}

bool createEGLDisplay(EGLDisplay& display, EGLConfig& config)
{
    // Connecting to the display
    EGLDisplay eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (eglDisplay == EGL_NO_DISPLAY) {
        printf("Got no EGL display.\n");
        return false;
    }

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

    display = eglDisplay;
    config = eglConfig;

    return true;
}

bool createEGLSurface(EGLSurface& surface, const EGLDisplay& eglDisplay,
                      const EGLConfig& eglConfig, const unsigned long window)
{
    EGLSurface eglSurface;
    {
        EGLint attributes[] = { EGL_NONE };
        eglSurface =
            eglCreateWindowSurface(eglDisplay, eglConfig, window, attributes);
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

    bool initialize(Ecore_X_Window window);
    void deinitialize();
    bool resizeSurface(Ecore_X_Window window);

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
    Ecore_X_Window m_window = 0;
};

bool RendererDelegateEGL::initialize(Ecore_X_Window window)
{
    m_window = window;
    if (!createEGLDisplay(m_eglDisplay, m_eglConfig) ||
        !createEGLSurface(m_eglSurface, m_eglDisplay, m_eglConfig, window) ||
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

bool RendererDelegateEGL::resizeSurface(Ecore_X_Window window)
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
    if (!createEGLSurface(m_eglSurface, m_eglDisplay, m_eglConfig, window)) {
        return false;
    }

    // Make the context current again with the new surface
    if (!eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface,
                        m_eglContext)) {
        printf("Failed to make context current after resize (eglError: 0x%x)\n",
               eglGetError());
        return false;
    }

    m_window = window;
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

class WindowEcoreX final : public Window {
public:
    WindowEcoreX();
    ~WindowEcoreX();

    bool init(const char* appName, int width, int height) override;
    void terminate() override;
    void getCursorPos(double& xpos, double& ypos) override;
    void ShowSoftwareKeyboardIfPossible() override;
    void HideSoftwareKeyboardIfPossible() override;

    void* getNativeWindowHandle() override
    {
        return nullptr;
    }

    virtual RendererDelegate* renderer() override
    {
        return m_renderer.get();
    }

    void handleImfCommit(const char* commitStr);
    void handleImfPreeditChanged(const char* preeditStr, int cursorPos);

private:
    void setupEventHandlers();
    void setupIMF();
    void cleanupIMF();

    Ecore_X_Window m_window = 0;
    std::unique_ptr<RendererDelegateEGL> m_renderer;
    Ecore_Event_Handler* m_keyDownHandler = nullptr;
    Ecore_Event_Handler* m_keyUpHandler = nullptr;
    Ecore_Event_Handler* m_mouseDownHandler = nullptr;
    Ecore_Event_Handler* m_mouseUpHandler = nullptr;
    Ecore_Event_Handler* m_mouseMoveHandler = nullptr;
    Ecore_Event_Handler* m_mouseWheelHandler = nullptr;
    Ecore_Event_Handler* m_windowResizeHandler = nullptr;
    Ecore_Event_Handler* m_windowDeleteHandler = nullptr;

    // Ecore_IMF support for IME
    Ecore_IMF_Context* m_imfContext = nullptr;
    bool m_isImfInitialized = false;
};

WindowEcoreX::WindowEcoreX()
{
}

WindowEcoreX::~WindowEcoreX()
{
}

bool WindowEcoreX::init(const char* appName, int width, int height)
{
    // Setup SIGINT signal handler
    std::signal(SIGINT, sigintHandler);

    // Initialize Ecore_X
    if (!ecore_x_init(nullptr)) {
        printf("Cannot initialize ecore_x\n");
        return false;
    }

    // Create window
    m_window = ecore_x_window_new(0, 0, 0, width, height);
    if (!m_window) {
        fprintf(stderr, "Failed to create window\n");
        ecore_x_shutdown();
        return 1;
    }

    // Set window properties
    ecore_x_icccm_title_set(m_window, "StarfishShell");
    ecore_x_netwm_window_type_set(m_window, ECORE_X_WINDOW_TYPE_NORMAL);

    // Set input mask to receive keyboard and mouse events
    ecore_x_event_mask_set(
        m_window,
        Ecore_X_Event_Mask(
            ECORE_X_EVENT_MASK_KEY_DOWN | ECORE_X_EVENT_MASK_KEY_UP |
            ECORE_X_EVENT_MASK_MOUSE_DOWN | ECORE_X_EVENT_MASK_MOUSE_UP |
            ECORE_X_EVENT_MASK_MOUSE_MOVE | ECORE_X_EVENT_MASK_MOUSE_WHEEL));

    // Set window to receive delete request
    ecore_x_icccm_protocol_set(m_window, ECORE_X_WM_PROTOCOL_DELETE_REQUEST,
                               EINA_TRUE);

    if (m_isVisible) {
        ecore_x_window_show(m_window);
    } else {
        ecore_x_window_hide(m_window);
    }

    // Setup event handlers
    setupEventHandlers();

    // Initialize EGL renderer
    m_renderer =
        std::unique_ptr<RendererDelegateEGL>(new RendererDelegateEGL());
    if (!m_renderer->initialize(m_window)) {
        ecore_x_shutdown();
        return false;
    }

    // Initialize IMF for IME support
    setupIMF();

    return true;
}

static Ecore_IMF_Keyboard_Modifiers ecore_modifier_to_imf_modifier(
    unsigned int ecore_modifiers)
{
    unsigned int imf_modifiers = ECORE_IMF_KEYBOARD_MODIFIER_NONE;

    if (ecore_modifiers & ECORE_EVENT_MODIFIER_SHIFT) {
        imf_modifiers |= ECORE_IMF_KEYBOARD_MODIFIER_SHIFT;
    }

    if (ecore_modifiers & ECORE_EVENT_MODIFIER_CTRL) {
        imf_modifiers |= ECORE_IMF_KEYBOARD_MODIFIER_CTRL;
    }

    if (ecore_modifiers & ECORE_EVENT_MODIFIER_ALT) {
        imf_modifiers |= ECORE_IMF_KEYBOARD_MODIFIER_ALT;
    }

    return (Ecore_IMF_Keyboard_Modifiers)imf_modifiers;
}

void WindowEcoreX::setupEventHandlers()
{
    // Keyboard key down handler with IMF filtering
    m_keyDownHandler = ecore_event_handler_add(
        ECORE_EVENT_KEY_DOWN,
        [](void* data, int type, void* event) -> Eina_Bool {
            WindowEcoreX* win = static_cast<WindowEcoreX*>(data);
            Ecore_Event_Key* keyEvent = static_cast<Ecore_Event_Key*>(event);

            // Filter through IMF first if available
            if (win->m_imfContext) {
                Ecore_IMF_Event_Key_Down imfEvent;
                memset(&imfEvent, 0, sizeof(Ecore_IMF_Event_Key_Down));
                imfEvent.keyname = (char*)keyEvent->keyname;
                imfEvent.key = keyEvent->key;
                imfEvent.string = keyEvent->string;
                imfEvent.compose = keyEvent->compose;
                imfEvent.timestamp = keyEvent->timestamp;
                imfEvent.modifiers =
                    ecore_modifier_to_imf_modifier(keyEvent->modifiers);
                if (ecore_imf_context_filter_event(
                        win->m_imfContext, ECORE_IMF_EVENT_KEY_DOWN,
                        (Ecore_IMF_Event*)&imfEvent)) {
                    return ECORE_CALLBACK_DONE;
                }
            }

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
            WindowEcoreX* win = static_cast<WindowEcoreX*>(data);
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
            WindowEcoreX* win = static_cast<WindowEcoreX*>(data);
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
            WindowEcoreX* win = static_cast<WindowEcoreX*>(data);
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
            WindowEcoreX* win = static_cast<WindowEcoreX*>(data);
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
            WindowEcoreX* win = static_cast<WindowEcoreX*>(data);
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

    // Window resize handler - using X window property change
    m_windowResizeHandler = ecore_event_handler_add(
        ECORE_X_EVENT_WINDOW_CONFIGURE,
        [](void* data, int type, void* event) -> Eina_Bool {
            WindowEcoreX* win = static_cast<WindowEcoreX*>(data);
            Ecore_X_Event_Window_Configure* configureEvent =
                static_cast<Ecore_X_Event_Window_Configure*>(event);

            if (configureEvent->win == win->m_window) {
                // Resize EGL surface to match new window size
                if (win->m_renderer) {
                    win->m_renderer->resizeSurface(win->m_window);
                }
                if (win->m_windowSizeEventHandler) {
                    win->m_windowSizeEventHandler(configureEvent->w,
                                                  configureEvent->h);
                }
            }
            return ECORE_CALLBACK_PASS_ON;
        },
        this);

    // Window delete handler
    m_windowDeleteHandler = ecore_event_handler_add(
        ECORE_X_EVENT_WINDOW_DELETE_REQUEST,
        [](void* data, int type, void* event) -> Eina_Bool {
            WindowEcoreX* win = static_cast<WindowEcoreX*>(data);
            Ecore_X_Event_Window_Delete_Request* deleteEvent =
                static_cast<Ecore_X_Event_Window_Delete_Request*>(event);

            if (deleteEvent->win == win->m_window) {
                if (win->m_exitEventHandler) {
                    win->m_exitEventHandler();
                }
            }
            return ECORE_CALLBACK_PASS_ON;
        },
        this);
}

void WindowEcoreX::setupIMF()
{
    if (m_isImfInitialized) {
        return;
    }

    // Initialize Ecore_IMF
    ecore_imf_init();

    // Get default IMF context ID
    const char* imfMethod = ecore_imf_context_default_id_get();
    if (!imfMethod) {
        printf("Warning: No default IMF method available\n");
        return;
    }

    // Create IMF context
    m_imfContext = ecore_imf_context_add(imfMethod);
    if (!m_imfContext) {
        printf("Warning: Failed to create IMF context\n");
        return;
    }

    // Set client window for IMF
    ecore_imf_context_client_window_set(m_imfContext,
                                        (void*)(uintptr_t)m_window);

    // Register commit callback
    ecore_imf_context_event_callback_add(
        m_imfContext, ECORE_IMF_CALLBACK_COMMIT,
        [](void* data, Ecore_IMF_Context* ctx, void* event_info) {
            WindowEcoreX* self = static_cast<WindowEcoreX*>(data);
            char* commitStr = static_cast<char*>(event_info);
            if (commitStr && self->m_compositionEventHandler) {
                self->m_compositionEventHandler(commitStr, true);
            }
        },
        this);

    // Register preedit changed callback
    ecore_imf_context_event_callback_add(
        m_imfContext, ECORE_IMF_CALLBACK_PREEDIT_CHANGED,
        [](void* data, Ecore_IMF_Context* ctx, void* event_info) {
            WindowEcoreX* self = static_cast<WindowEcoreX*>(data);
            char* preeditStr = nullptr;
            int cursorPos = 0;
            ecore_imf_context_preedit_string_get(self->m_imfContext,
                                                 &preeditStr, &cursorPos);
            if (preeditStr) {
                if (self->m_compositionEventHandler) {
                    self->m_compositionEventHandler(preeditStr, false);
                }
                free(preeditStr);
            }
        },
        this);

    m_isImfInitialized = true;
}

void WindowEcoreX::cleanupIMF()
{
    if (!m_isImfInitialized || !m_imfContext) {
        return;
    }

    ecore_imf_context_hide(m_imfContext);
    ecore_imf_context_focus_out(m_imfContext);
    ecore_imf_context_reset(m_imfContext);
    ecore_imf_context_client_window_set(m_imfContext, nullptr);
    ecore_imf_context_del(m_imfContext);
    m_imfContext = nullptr;
    ecore_imf_shutdown();
    m_isImfInitialized = false;
}

void WindowEcoreX::ShowSoftwareKeyboardIfPossible()
{
    if (m_imfContext) {
        ecore_imf_context_focus_in(m_imfContext);
        ecore_imf_context_show(m_imfContext);
        printf("[IMF] Show software keyboard\n");
    }
}

void WindowEcoreX::HideSoftwareKeyboardIfPossible()
{
    if (m_imfContext) {
        ecore_imf_context_hide(m_imfContext);
        ecore_imf_context_focus_out(m_imfContext);
        printf("[IMF] Hide software keyboard\n");
    }
}

void WindowEcoreX::handleImfCommit(const char* commitStr)
{
    if (commitStr && m_compositionEventHandler) {
        m_compositionEventHandler(commitStr, true);
    }
}

void WindowEcoreX::handleImfPreeditChanged(const char* preeditStr,
                                           int cursorPos)
{
    if (preeditStr && m_compositionEventHandler) {
        m_compositionEventHandler(preeditStr, false);
    }
}

void WindowEcoreX::getCursorPos(double& xpos, double& ypos)
{
    int win_x = 0, win_y = 0;

    ecore_x_pointer_xy_get(m_window, &win_x, &win_y);

    xpos = win_x;
    ypos = win_y;
}

void WindowEcoreX::terminate()
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

    // Clean up IMF
    cleanupIMF();

    m_renderer->deinitialize();
    m_renderer = nullptr;

    ecore_x_window_free(m_window);

    ecore_x_shutdown();

    m_window = 0;
}

Window* Window::create()
{
    return new WindowEcoreX();
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
