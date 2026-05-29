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

#if defined(STARFISH_SHELL_TCORE_WL)

#include "Window.h"

#include <tizen_core_wl.h>
#include <tizen_core_wl_internal.h>
#include <tizen_core_imf.h>
#include <EGL/egl.h>

#include <GLES2/gl2.h>

#include <memory>
#include <cstring>

namespace {

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
                      const EGLConfig& eglConfig,
                      tizen_core_wl_egl_window_h eglWindow)
{
    EGLSurface eglSurface;
    {
        EGLint attributes[] = { EGL_NONE };
        const auto eglNativeWindow =
            tizen_core_wl_egl_window_native_get(eglWindow);
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

    bool initialize(tizen_core_wl_window_h window,
                    tizen_core_wl_egl_window_h eglWindow,
                    tizen_core_wl_display_h display);
    void deinitialize();
    bool resizeSurface(tizen_core_wl_window_h window,
                       tizen_core_wl_egl_window_h eglWindow, int w, int h);

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
    tizen_core_wl_window_h m_window = nullptr;
};

bool RendererDelegateEGL::initialize(tizen_core_wl_window_h window,
                                     tizen_core_wl_egl_window_h eglWindow,
                                     tizen_core_wl_display_h display)
{
    m_window = window;

    // Get wl_display from tizen_core_wl_display
    struct wl_display* wlDisplay = nullptr;
    tizen_core_wl_display_private_get_wl_display(display, &wlDisplay);
    m_eglDisplay = eglGetDisplay(wlDisplay);

    if (!createEGLDisplay(m_eglDisplay, m_eglConfig) ||
        !createEGLSurface(m_eglSurface, m_eglDisplay, m_eglConfig, eglWindow) ||
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

bool RendererDelegateEGL::resizeSurface(tizen_core_wl_window_h window,
                                        tizen_core_wl_egl_window_h eglWindow,
                                        int w, int h)
{
    // Make context current with no surface before destroying old surface
    eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE,
                   EGL_NO_CONTEXT);

    // Destroy old surface
    if (m_eglSurface != EGL_NO_SURFACE) {
        eglDestroySurface(m_eglDisplay, m_eglSurface);
        m_eglSurface = EGL_NO_SURFACE;
    }

    // Resize the EGL window
    tizen_core_wl_egl_window_resize(eglWindow, w, h);

    // Create new surface with the new window size
    if (!createEGLSurface(m_eglSurface, m_eglDisplay, m_eglConfig, eglWindow)) {
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

class WindowTcoreWl final : public Window {
public:
    WindowTcoreWl();
    ~WindowTcoreWl();

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
    static void eventCallback(void* event,
                              tizen_core_wl_event_type_e event_type,
                              void* user_data);
    static void imfCommitCallback(tizen_core_imf_context_h ctx,
                                  void* event_info, void* user_data);
    static void imfPreeditChangedCallback(tizen_core_imf_context_h ctx,
                                          void* event_info, void* user_data);

    tizen_core_wl_display_h m_display = nullptr;
    tizen_core_wl_window_h m_window = nullptr;
    tizen_core_wl_egl_window_h m_eglWindow = nullptr;
    tizen_core_wl_seat_h m_seat = nullptr;
    tizen_core_wl_event_listener_h m_keyDownListener = nullptr;
    tizen_core_wl_event_listener_h m_keyUpListener = nullptr;
    tizen_core_wl_event_listener_h m_mouseDownListener = nullptr;
    tizen_core_wl_event_listener_h m_mouseUpListener = nullptr;
    tizen_core_wl_event_listener_h m_mouseMoveListener = nullptr;
    tizen_core_wl_event_listener_h m_mouseWheelListener = nullptr;
    tizen_core_wl_event_listener_h m_windowConfigureListener = nullptr;
    tizen_core_wl_event_listener_h m_windowDestroyListener = nullptr;
    int m_mouseX = 0, m_mouseY = 0;
    tizen_core_event_h m_eventHandle = nullptr;
    std::unique_ptr<RendererDelegateEGL> m_renderer;

    // tizen_core_imf support for IME
    tizen_core_imf_context_h m_imfContext = nullptr;
    bool m_isImfInitialized = false;
};

WindowTcoreWl::WindowTcoreWl()
{
}

WindowTcoreWl::~WindowTcoreWl()
{
}

void WindowTcoreWl::setupIMF()
{
    if (m_isImfInitialized) {
        return;
    }

    // Initialize tizen_core_imf
    if (tizen_core_imf_init() != TIZEN_CORE_IMF_ERROR_NONE) {
        printf("Warning: Failed to initialize tizen_core_imf\n");
        return;
    }

    // Create IMF context
    if (tizen_core_imf_context_create(&m_imfContext) !=
        TIZEN_CORE_IMF_ERROR_NONE) {
        printf("Warning: Failed to create IMF context\n");
        tizen_core_imf_shutdown();
        return;
    }

    // Set client window for IMF - use the tizen_core_wl window
    tizen_core_imf_context_set_client_window(m_imfContext, (void*)m_window);

    // Register commit callback
    tizen_core_imf_context_add_event_callback(
        m_imfContext, TIZEN_CORE_IMF_CALLBACK_COMMIT,
        [](tizen_core_imf_context_h ctx, void* event_info, void* user_data) {
            WindowTcoreWl* self = static_cast<WindowTcoreWl*>(user_data);
            char* commitStr = static_cast<char*>(event_info);
            self->handleImfCommit(commitStr);
        },
        this);

    // Register preedit changed callback
    tizen_core_imf_context_add_event_callback(
        m_imfContext, TIZEN_CORE_IMF_CALLBACK_PREEDIT_CHANGED,
        [](tizen_core_imf_context_h ctx, void* event_info, void* user_data) {
            WindowTcoreWl* self = static_cast<WindowTcoreWl*>(user_data);
            char* preeditStr = nullptr;
            int cursorPos = 0;
            tizen_core_imf_preedit_attr_h* attrs = nullptr;
            int attrsCount = 0;

            if (tizen_core_imf_context_get_preedit_string(
                    self->m_imfContext, &preeditStr, &attrs, &attrsCount,
                    &cursorPos) == TIZEN_CORE_IMF_ERROR_NONE) {
                self->handleImfPreeditChanged(preeditStr, cursorPos);
                if (preeditStr)
                    free(preeditStr);
                if (attrs) {
                    for (int i = 0; i < attrsCount; i++) {
                        // attrs will be freed by the framework
                    }
                    free(attrs);
                }
            }
        },
        this);

    m_isImfInitialized = true;
}

void WindowTcoreWl::cleanupIMF()
{
    if (!m_isImfInitialized || !m_imfContext) {
        return;
    }

    tizen_core_imf_context_input_panel_hide(m_imfContext);
    tizen_core_imf_context_focus_out(m_imfContext);
    tizen_core_imf_context_set_client_window(m_imfContext, nullptr);
    tizen_core_imf_context_destroy(m_imfContext);
    m_imfContext = nullptr;
    tizen_core_imf_shutdown();
    m_isImfInitialized = false;
}

void WindowTcoreWl::ShowSoftwareKeyboardIfPossible()
{
    if (m_imfContext) {
        tizen_core_imf_context_focus_in(m_imfContext);
        tizen_core_imf_context_input_panel_show(m_imfContext);
    }
}

void WindowTcoreWl::HideSoftwareKeyboardIfPossible()
{
    if (m_imfContext) {
        tizen_core_imf_context_input_panel_hide(m_imfContext);
        tizen_core_imf_context_focus_out(m_imfContext);
    }
}

void WindowTcoreWl::handleImfCommit(const char* commitStr)
{
    if (commitStr && m_compositionEventHandler) {
        m_compositionEventHandler(commitStr, true);
    }
}

void WindowTcoreWl::handleImfPreeditChanged(const char* preeditStr,
                                            int cursorPos)
{
    if (preeditStr && m_compositionEventHandler) {
        m_compositionEventHandler(preeditStr, false);
    }
}

void WindowTcoreWl::eventCallback(void* event,
                                  tizen_core_wl_event_type_e event_type,
                                  void* user_data)
{
    WindowTcoreWl* win = static_cast<WindowTcoreWl*>(user_data);
    tizen_core_wl_event_input_base_h inputEvent =
        (tizen_core_wl_event_input_base_h)event;

    switch (event_type) {
    case TIZEN_CORE_WL_EVENT_KEY_DOWN: {
        // Filter through IMF first if available
        if (win->m_imfContext) {
            char* keyname = NULL;
            char* devId = NULL;

            unsigned int keycode;
            tizen_core_wl_error_e ret =
                tizen_core_wl_event_key_get_keycode(inputEvent, &keycode);
            if (ret != TIZEN_CORE_WL_ERROR_NONE) {
                printf("Failed to get keycode: %d\n", ret);
                return;
            }

            ret = tizen_core_wl_event_key_get_keyname(inputEvent, &keyname);
            if (ret != TIZEN_CORE_WL_ERROR_NONE) {
                printf("Failed to get keyname: %d\n", ret);
                return;
            }

            tizen_core_wl_window_h window;
            ret =
                tizen_core_wl_event_input_base_get_window(inputEvent, &window);
            if (ret != TIZEN_CORE_WL_ERROR_NONE) {
                printf("Failed to get window: %d\n", ret);
                return;
            }

            ret = tizen_core_wl_event_input_base_get_device_identifier(
                inputEvent, &devId);
            if (ret != TIZEN_CORE_WL_ERROR_NONE) {
                printf("Failed to get device identifier: %d\n", ret);
                return;
            }

            tizen_core_imf_event_key_h keyEv = NULL;
            tizen_core_imf_event_key_create(&keyEv);
            tizen_core_imf_event_key_set_keyname(keyEv, keyname);
            tizen_core_imf_event_key_set_key(keyEv, keyname);
            tizen_core_imf_event_key_set_device_name(keyEv, devId);
            tizen_core_imf_event_key_set_device_class(
                keyEv, TIZEN_CORE_IMF_DEVICE_CLASS_KEYBOARD);
            tizen_core_imf_event_key_set_device_subclass(
                keyEv, TIZEN_CORE_IMF_DEVICE_SUBCLASS_NONE);
            tizen_core_imf_event_key_set_keycode(keyEv, keycode);

            bool filtered = false;
            tizen_core_imf_context_filter_event(
                win->m_imfContext, TIZEN_CORE_IMF_EVENT_TYPE_KEY_DOWN,
                (void*)keyEv, &filtered);
            tizen_core_imf_event_key_destroy(keyEv);
            if (filtered) {
                // Event was handled by IMF, don't process further
                break;
            }
        }

        if (win->m_keyEventHandler) {
            char* keyname = nullptr;
            unsigned int keycode = 0;
            unsigned int modifiers = 0;

            tizen_core_wl_event_key_get_keyname(inputEvent, &keyname);
            tizen_core_wl_event_key_get_keycode(inputEvent, &keycode);
            tizen_core_wl_event_key_get_modifiers(inputEvent, &modifiers);

            unsigned long finalKeycode = 0;
            if (keyname) {
                if (strcmp(keyname, "Left") == 0) {
                    finalKeycode = static_cast<unsigned long>(INPUT::LEFT);
                } else if (strcmp(keyname, "Up") == 0) {
                    finalKeycode = static_cast<unsigned long>(INPUT::UP);
                } else if (strcmp(keyname, "Right") == 0) {
                    finalKeycode = static_cast<unsigned long>(INPUT::RIGHT);
                } else if (strcmp(keyname, "Down") == 0) {
                    finalKeycode = static_cast<unsigned long>(INPUT::DOWN);
                } else if (strlen(keyname) == 1) {
                    finalKeycode = static_cast<unsigned long>(keyname[0]);
                } else {
                    finalKeycode = keycode;
                }
                free(keyname);
            }

            unsigned mods = 0;
            if (modifiers & TIZEN_CORE_WL_MODIFIER_SHIFT) {
                mods |= static_cast<unsigned>(MOD::SHIFT);
            }
            if (modifiers & TIZEN_CORE_WL_MODIFIER_CTRL) {
                mods |= static_cast<unsigned>(MOD::CONTROL);
            }

            INPUT action = INPUT::PRESS;
            win->m_keyEventHandler(finalKeycode, action, mods);
        }
        break;
    }

    case TIZEN_CORE_WL_EVENT_KEY_UP: {
        if (win->m_keyEventHandler) {
            char* keyname = nullptr;
            unsigned int keycode = 0;
            unsigned int modifiers = 0;

            tizen_core_wl_event_key_get_keyname(inputEvent, &keyname);
            tizen_core_wl_event_key_get_keycode(inputEvent, &keycode);
            tizen_core_wl_event_key_get_modifiers(inputEvent, &modifiers);

            unsigned long finalKeycode = 0;
            if (keyname) {
                if (strcmp(keyname, "Left") == 0) {
                    finalKeycode = static_cast<unsigned long>(INPUT::LEFT);
                } else if (strcmp(keyname, "Up") == 0) {
                    finalKeycode = static_cast<unsigned long>(INPUT::UP);
                } else if (strcmp(keyname, "Right") == 0) {
                    finalKeycode = static_cast<unsigned long>(INPUT::RIGHT);
                } else if (strcmp(keyname, "Down") == 0) {
                    finalKeycode = static_cast<unsigned long>(INPUT::DOWN);
                } else if (strlen(keyname) == 1) {
                    finalKeycode = static_cast<unsigned long>(keyname[0]);
                } else {
                    finalKeycode = keycode;
                }
                free(keyname);
            }

            unsigned mods = 0;
            if (modifiers & TIZEN_CORE_WL_MODIFIER_SHIFT) {
                mods |= static_cast<unsigned>(MOD::SHIFT);
            }
            if (modifiers & TIZEN_CORE_WL_MODIFIER_CTRL) {
                mods |= static_cast<unsigned>(MOD::CONTROL);
            }

            INPUT action = INPUT::RELEASE;
            win->m_keyEventHandler(finalKeycode, action, mods);
        }
        break;
    }

    case TIZEN_CORE_WL_EVENT_MOUSE_BUTTON_DOWN: {
        if (win->m_buttonEventHandler) {
            win->m_buttonEventHandler(INPUT::MOUSE_LBUTTON, INPUT::PRESS);
        }
        break;
    }
    case TIZEN_CORE_WL_EVENT_MOUSE_BUTTON_UP: {
        if (win->m_buttonEventHandler) {
            win->m_buttonEventHandler(INPUT::MOUSE_LBUTTON, INPUT::RELEASE);
        }
        break;
    }

    case TIZEN_CORE_WL_EVENT_MOUSE_MOVE: {
        if (win->m_motionEventHandler) {
            tizen_core_wl_event_mouse_move_get_position(
                inputEvent, &win->m_mouseX, &win->m_mouseY);
            win->m_motionEventHandler(win->m_mouseX, win->m_mouseY);
        }
        break;
    }

    case TIZEN_CORE_WL_EVENT_MOUSE_WHEEL: {
        if (win->m_scrollEventHandler) {
            int x = 0, y = 0, z = 0;
            tizen_core_wl_event_mouse_wheel_get_position(inputEvent, &x, &y);
            tizen_core_wl_event_mouse_wheel_get_z(inputEvent, &z);
            double xpos, ypos;
            win->getCursorPos(xpos, ypos);
            win->m_scrollEventHandler(xpos, ypos, z);
        }
        break;
    }

    case TIZEN_CORE_WL_EVENT_WINDOW_CONFIGURE_COMPLETE: {
        int x = 0, y = 0, w = 0, h = 0;
        tizen_core_wl_window_get_geometry(win->m_window, &x, &y, &w, &h);
        if (win->m_eglWindow && win->m_renderer) {
            win->m_renderer->resizeSurface(win->m_window, win->m_eglWindow, w,
                                           h);
        }
        if (win->m_windowSizeEventHandler) {
            win->m_windowSizeEventHandler(w, h);
        }
        break;
    }

    case TIZEN_CORE_WL_EVENT_WINDOW_DESTROY: {
        if (win->m_exitEventHandler) {
            win->m_exitEventHandler();
        }
        break;
    }

    default:
        break;
    }
}

bool WindowTcoreWl::init(const char* appName, int width, int height)
{
    // Initialize Tizen Core WL
    if (tizen_core_wl_init() != TIZEN_CORE_WL_ERROR_NONE) {
        printf("Cannot initialize tizen_core_wl\n");
        return false;
    }

    // Create display handle
    if (tizen_core_wl_display_create(&m_display) != TIZEN_CORE_WL_ERROR_NONE) {
        printf("Failed to create display handle\n");
        tizen_core_wl_shutdown();
        return false;
    }

    // Connect to Wayland display
    if (tizen_core_wl_display_connect(m_display, NULL) !=
        TIZEN_CORE_WL_ERROR_NONE) {
        printf("Failed to connect to Wayland display\n");
        tizen_core_wl_display_destroy(m_display);
        tizen_core_wl_shutdown();
        return false;
    }

    // Create window
    if (tizen_core_wl_create_window(m_display, nullptr, 0, 0, width, height,
                                    &m_window) != TIZEN_CORE_WL_ERROR_NONE) {
        printf("Failed to create window\n");
        tizen_core_wl_display_disconnect(m_display);
        tizen_core_wl_display_destroy(m_display);
        tizen_core_wl_shutdown();
        return false;
    }

    // Set window properties
    tizen_core_wl_window_set_title(m_window, "StarfishShell");
    tizen_core_wl_window_set_type(m_window,
                                  TIZEN_CORE_WL_WINDOW_TYPE_NOTIFICATION);

    // Create EGL window
    struct wl_surface* surface = nullptr;
    tizen_core_wl_window_private_get_wl_surface(m_window, &surface);
    if (tizen_core_wl_create_egl_window(m_window, width, height,
                                        &m_eglWindow) !=
            TIZEN_CORE_WL_ERROR_NONE ||
        !m_eglWindow) {
        printf("Failed to create EGL window\n");
        tizen_core_wl_window_destroy(m_window);
        tizen_core_wl_display_disconnect(m_display);
        tizen_core_wl_display_destroy(m_display);
        tizen_core_wl_shutdown();
        return false;
    }

    // Show/hide window
    if (m_isVisible) {
        tizen_core_wl_window_show(m_window);
    } else {
        tizen_core_wl_window_hide(m_window);
    }

    // Setup event handlers
    setupEventHandlers();

    // Get default seat
    tizen_core_wl_display_get_default_seat(m_display, &m_seat);

    // Initialize EGL renderer
    m_renderer =
        std::unique_ptr<RendererDelegateEGL>(new RendererDelegateEGL());
    if (!m_renderer->initialize(m_window, m_eglWindow, m_display)) {
        tizen_core_wl_egl_window_destroy(m_eglWindow);
        tizen_core_wl_window_destroy(m_window);
        tizen_core_wl_display_disconnect(m_display);
        tizen_core_wl_display_destroy(m_display);
        tizen_core_wl_shutdown();
        return false;
    }

    // Initialize IMF for IME support
    setupIMF();

    return true;
}

void WindowTcoreWl::setupEventHandlers()
{
    // Get event handle from display
    tizen_core_wl_display_get_event(m_display, &m_eventHandle);

    // Add event listeners
    tizen_core_wl_event_add_listener(m_eventHandle,
                                     TIZEN_CORE_WL_EVENT_KEY_DOWN,
                                     eventCallback, this, &m_keyDownListener);
    tizen_core_wl_event_add_listener(m_eventHandle, TIZEN_CORE_WL_EVENT_KEY_UP,
                                     eventCallback, this, &m_keyUpListener);
    tizen_core_wl_event_add_listener(m_eventHandle,
                                     TIZEN_CORE_WL_EVENT_MOUSE_BUTTON_DOWN,
                                     eventCallback, this, &m_mouseDownListener);
    tizen_core_wl_event_add_listener(m_eventHandle,
                                     TIZEN_CORE_WL_EVENT_MOUSE_BUTTON_UP,
                                     eventCallback, this, &m_mouseUpListener);
    tizen_core_wl_event_add_listener(m_eventHandle,
                                     TIZEN_CORE_WL_EVENT_MOUSE_MOVE,
                                     eventCallback, this, &m_mouseMoveListener);
    tizen_core_wl_event_add_listener(
        m_eventHandle, TIZEN_CORE_WL_EVENT_MOUSE_WHEEL, eventCallback, this,
        &m_mouseWheelListener);
    tizen_core_wl_event_add_listener(
        m_eventHandle, TIZEN_CORE_WL_EVENT_WINDOW_CONFIGURE, eventCallback,
        this, &m_windowConfigureListener);
    tizen_core_wl_event_add_listener(
        m_eventHandle, TIZEN_CORE_WL_EVENT_WINDOW_DESTROY, eventCallback, this,
        &m_windowDestroyListener);
}

void WindowTcoreWl::getCursorPos(double& xpos, double& ypos)
{
    xpos = m_mouseX;
    ypos = m_mouseY;
}

void WindowTcoreWl::terminate()
{
    // Remove event listeners
    if (m_eventHandle) {
        if (m_keyDownListener) {
            tizen_core_wl_event_remove_listener(m_eventHandle,
                                                m_keyDownListener);
        }
        if (m_keyUpListener) {
            tizen_core_wl_event_remove_listener(m_eventHandle, m_keyUpListener);
        }
        if (m_mouseDownListener) {
            tizen_core_wl_event_remove_listener(m_eventHandle,
                                                m_mouseDownListener);
        }
        if (m_mouseUpListener) {
            tizen_core_wl_event_remove_listener(m_eventHandle,
                                                m_mouseUpListener);
        }
        if (m_mouseMoveListener) {
            tizen_core_wl_event_remove_listener(m_eventHandle,
                                                m_mouseMoveListener);
        }
        if (m_mouseWheelListener) {
            tizen_core_wl_event_remove_listener(m_eventHandle,
                                                m_mouseWheelListener);
        }
        if (m_windowConfigureListener) {
            tizen_core_wl_event_remove_listener(m_eventHandle,
                                                m_windowConfigureListener);
        }
        if (m_windowDestroyListener) {
            tizen_core_wl_event_remove_listener(m_eventHandle,
                                                m_windowDestroyListener);
        }
    }

    // Clean up IMF
    cleanupIMF();

    m_renderer->deinitialize();
    m_renderer = nullptr;

    tizen_core_wl_egl_window_destroy(m_eglWindow);
    tizen_core_wl_window_destroy(m_window);
    tizen_core_wl_display_disconnect(m_display);
    tizen_core_wl_display_destroy(m_display);
    tizen_core_wl_shutdown();

    m_window = nullptr;
    m_display = nullptr;
}

Window* Window::create()
{
    return new WindowTcoreWl();
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
