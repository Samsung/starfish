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

#if defined(STARFISH_SHELL_TCORE_WL)

#define STARFISH_ENABLE_PROFILE_TIMER

#include "StarfishConfig.h"
#include "PlatformIntegrationData.h"
#include "public/delegate/LWEWebViewDelegateImpl.h"
#include "public/delegate/LWEWebContainerDelegate.h"
#include "LWEWebView.h"

#include <tizen_core_wl.h>
#include <tizen_core_wl_internal.h>
#include <tizen_core_imf.h>
#include <EGL/egl.h>

#if defined(PORT_EVENTLOOP_BACKEND_LIBUV)
#include <uv.h>
#endif

#include <memory>
#include <cstring>
#include <locale>
#include <csignal>

namespace {

bool createEGLDisplay(EGLDisplay& display, EGLConfig& config,
                      struct wl_display* wlDisplay)
{
    EGLDisplay eglDisplay = eglGetDisplay(wlDisplay);
    if (eglDisplay == EGL_NO_DISPLAY) {
        printf("Got no EGL display.\n");
        return false;
    }

    EGLint eglVersionMajor, eglVersionMinor;
    if (!eglInitialize(eglDisplay, &eglVersionMajor, &eglVersionMinor)) {
        printf("Unable to initialize EGL\n");
        return false;
    }

    eglBindAPI(EGL_OPENGL_ES_API);

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

namespace LWEDelegate {

using namespace LWE;

class WebViewTcoreWl : public WebViewImpl {
public:
    using ResizeCallback = std::function<void(int, int)>;
    using MouseMoveCallback = std::function<void(int, int)>;
    using MouseButtonCallback = std::function<void(int, int, int)>;
    using MouseWheelCallback = std::function<void(int, int, int)>;
    using KeyCallback = std::function<void(int, int)>;
    using RenderCallback = std::function<void()>;
    using CloseCallback = std::function<void()>;

    WebViewTcoreWl(void* winArg, unsigned x, unsigned y, unsigned width,
                   unsigned height, float devicePixelRatio,
                   const char* defaultFontName, const char* locale,
                   const char* timezoneID)
        : m_ownsWindow(false)
        , m_window(nullptr)
        , m_eglWindow(nullptr)
        , m_eglDisplay(EGL_NO_DISPLAY)
        , m_eglSurface(EGL_NO_SURFACE)
        , m_eglContext(EGL_NO_CONTEXT)
        , m_lastWidth(width)
        , m_lastHeight(height)
        , m_webContainer(nullptr)
        , m_imfContext(nullptr)
        , m_isImfInitialized(false)
        , m_keyDownListener(nullptr)
        , m_keyUpListener(nullptr)
        , m_mouseDownListener(nullptr)
        , m_mouseUpListener(nullptr)
        , m_mouseMoveListener(nullptr)
        , m_mouseWheelListener(nullptr)
        , m_windowConfigureListener(nullptr)
        , m_windowDestroyListener(nullptr)
    {
        STARFISH_LOG_INFO("WebViewTcoreWl::WebViewTcoreWl");

        if (tizen_core_wl_init() != TIZEN_CORE_WL_ERROR_NONE) {
            STARFISH_LOG_ERROR("Cannot initialize tizen_core_wl");
            return;
        }

        if (tizen_core_wl_display_create(&m_display) !=
            TIZEN_CORE_WL_ERROR_NONE) {
            STARFISH_LOG_ERROR("Failed to create display");
            tizen_core_wl_shutdown();
            return;
        }

        if (tizen_core_wl_display_connect(m_display, NULL) !=
            TIZEN_CORE_WL_ERROR_NONE) {
            STARFISH_LOG_ERROR("Failed to connect to Wayland display");
            tizen_core_wl_display_destroy(m_display);
            tizen_core_wl_shutdown();
            return;
        }

        if (winArg == nullptr) {
            if (tizen_core_wl_create_window(m_display, nullptr, 0, 0, width,
                                            height, &m_window) !=
                TIZEN_CORE_WL_ERROR_NONE) {
                STARFISH_LOG_ERROR("Failed to create window");
                tizen_core_wl_display_disconnect(m_display);
                tizen_core_wl_display_destroy(m_display);
                tizen_core_wl_shutdown();
                return;
            }

            tizen_core_wl_window_set_title(m_window, "Starfish");
            tizen_core_wl_window_set_type(
                m_window, TIZEN_CORE_WL_WINDOW_TYPE_NOTIFICATION);

            struct wl_surface* surface = nullptr;
            tizen_core_wl_window_private_get_wl_surface(m_window, &surface);
            if (tizen_core_wl_create_egl_window(m_window, width, height,
                                                &m_eglWindow) !=
                    TIZEN_CORE_WL_ERROR_NONE ||
                !m_eglWindow) {
                STARFISH_LOG_ERROR("Failed to create EGL window");
                tizen_core_wl_window_destroy(m_window);
                tizen_core_wl_display_disconnect(m_display);
                tizen_core_wl_display_destroy(m_display);
                tizen_core_wl_shutdown();
                return;
            }

            tizen_core_wl_window_show(m_window);
            m_ownsWindow = true;
        } else {
            m_window = reinterpret_cast<tizen_core_wl_window_h>(winArg);
            m_ownsWindow = false;
        }

        struct wl_display* wlDisplay = nullptr;
        tizen_core_wl_display_private_get_wl_display(m_display, &wlDisplay);
        if (!createEGLDisplay(m_eglDisplay, m_eglConfig, wlDisplay) ||
            !createEGLSurface(m_eglSurface, m_eglDisplay, m_eglConfig,
                              m_eglWindow) ||
            !createGLContext(m_eglContext, m_eglDisplay, m_eglConfig,
                             nullptr)) {
            STARFISH_LOG_ERROR("Failed to initialize EGL");
            tizen_core_wl_egl_window_destroy(m_eglWindow);
            tizen_core_wl_window_destroy(m_window);
            tizen_core_wl_display_disconnect(m_display);
            tizen_core_wl_display_destroy(m_display);
            tizen_core_wl_shutdown();
            return;
        }

        if (!eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface,
                            m_eglContext)) {
            STARFISH_LOG_ERROR("Failed to make context current");
        }

        setupEventHandlers();
        setupIMF();

        WebContainer::WebContainerArguments args{
            width,           height, devicePixelRatio,
            defaultFontName, locale, timezoneID,
        };

        WebContainer::RendererGLConfiguration config;
        config.onMakeCurrent = [this](WebContainer* wc) { makeCurrent(); };
        config.onSwapBuffers = [this](WebContainer* wc, bool mayNeedsSync) {
            swapBuffers();
        };
        config.onCreateSharedContext = [this](WebContainer* wc) -> uintptr_t {
            EGLContext sharedContext;
            if (createGLContext(sharedContext, m_eglDisplay, m_eglConfig,
                                m_eglContext)) {
                return reinterpret_cast<uintptr_t>(sharedContext);
            }
            return UINTPTR_MAX;
        };
        config.onDestroyContext = [this](WebContainer* wc,
                                         uintptr_t context) -> bool {
            return eglDestroyContext(m_eglDisplay,
                                     reinterpret_cast<EGLContext>(context));
        };
        config.onClearCurrentContext = [this](WebContainer* wc) -> bool {
            return eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE,
                                  EGL_NO_CONTEXT);
        };
        config.onMakeCurrentWithContext = [this](WebContainer* wc,
                                                 uintptr_t context) -> bool {
            if (!eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface,
                                reinterpret_cast<EGLContext>(context))) {
                printf("Failed to set current context (eglError: 0x%x)\n",
                       eglGetError());
                return false;
            }
            return true;
        };
        config.onGetProcAddress = [this](WebContainer* wc,
                                         const char* name) -> void* {
            return reinterpret_cast<void*>(eglGetProcAddress(name));
        };
        config.onIsSupportedExtension = [this](WebContainer* wc,
                                               const char* extension) -> bool {
            const char* extensions =
                eglQueryString(m_eglDisplay, EGL_EXTENSIONS);
            return extensions && strstr(extensions, extension) != nullptr;
        };

        m_webContainer = WebContainer::CreateGL(args, config);
        SetWebContainer(m_webContainer);

        setResizeCallback(
            [this](int w, int h) { m_webContainer->ResizeTo(w, h); });

        setMouseMoveCallback([this](int x, int y) {
            m_webContainer->DispatchMouseMoveEvent(
                MouseButtonValue::NoButton, MouseButtonsValue::NoButtonDown, x,
                y);
        });

        setMouseButtonCallback([this](int button, int x, int y) {
            if (button == 1) {
                m_webContainer->DispatchMouseDownEvent(
                    MouseButtonValue::LeftButton,
                    MouseButtonsValue::LeftButtonDown, x, y);
            } else if (button == 0) {
                m_webContainer->DispatchMouseUpEvent(
                    MouseButtonValue::LeftButton,
                    MouseButtonsValue::NoButtonDown, x, y);
            }
        });

        setMouseWheelCallback([this](int delta, int x, int y) {
            m_webContainer->DispatchMouseWheelEvent(x, y, delta);
        });

        setKeyCallback([this](int key, int isKeyDown) {
            LWE::KeyValue keyValue = static_cast<LWE::KeyValue>(key);
            if (isKeyDown) {
                m_webContainer->DispatchKeyDownEvent(keyValue);
                m_webContainer->DispatchKeyPressEvent(keyValue);
            } else {
                m_webContainer->DispatchKeyUpEvent(keyValue);
            }
        });

        setCloseCallback([this]() {
            STARFISH_LOG_INFO("Window close requested");
            raise(SIGINT);
        });

        m_webContainer->RegisterOnShowSoftwareKeyboardIfPossibleHandler(
            [this](WebContainer*) {
                if (m_imfContext) {
                    tizen_core_imf_context_focus_in(m_imfContext);
                    tizen_core_imf_context_input_panel_show(m_imfContext);
                }
            });

        m_webContainer->RegisterOnHideSoftwareKeyboardIfPossibleHandler(
            [this](WebContainer* t) {
                if (m_imfContext) {
                    tizen_core_imf_context_input_panel_hide(m_imfContext);
                    tizen_core_imf_context_focus_out(m_imfContext);
                }
            });
    }

    ~WebViewTcoreWl()
    {
        STARFISH_LOG_INFO("WebViewTcoreWl::~WebViewTcoreWl");

        if (m_webContainer) {
            m_webContainer->Destroy();
            m_webContainer = nullptr;
        }

        cleanupIMF();

        if (m_eventHandle) {
            if (m_keyDownListener) {
                tizen_core_wl_event_remove_listener(m_eventHandle,
                                                    m_keyDownListener);
            }
            if (m_keyUpListener) {
                tizen_core_wl_event_remove_listener(m_eventHandle,
                                                    m_keyUpListener);
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

        if (m_eglSurface != EGL_NO_SURFACE) {
            eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE,
                           EGL_NO_CONTEXT);
            eglDestroySurface(m_eglDisplay, m_eglSurface);
        }
        if (m_eglContext != EGL_NO_CONTEXT) {
            eglDestroyContext(m_eglDisplay, m_eglContext);
        }
        if (m_eglDisplay != EGL_NO_DISPLAY) {
            eglTerminate(m_eglDisplay);
        }

        if (m_eglWindow) {
            tizen_core_wl_egl_window_destroy(m_eglWindow);
        }

        if (m_window && m_ownsWindow) {
            tizen_core_wl_window_destroy(m_window);
        }

        tizen_core_wl_display_disconnect(m_display);
        tizen_core_wl_display_destroy(m_display);

        tizen_core_wl_shutdown();
    }

    virtual void Destroy() override
    {
        delete this;
    }

    bool makeCurrent()
    {
        if (!eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface,
                            m_eglContext)) {
            printf("Failed to set current context (eglError: 0x%x)\n",
                   eglGetError());
            return false;
        }
        return true;
    }

    bool swapBuffers()
    {
        return eglSwapBuffers(m_eglDisplay, m_eglSurface);
    }

    void setResizeCallback(ResizeCallback callback)
    {
        m_resizeCallback = callback;
    }

    void setMouseMoveCallback(MouseMoveCallback callback)
    {
        m_mouseMoveCallback = callback;
    }

    void setMouseButtonCallback(MouseButtonCallback callback)
    {
        m_mouseButtonCallback = callback;
    }

    void setMouseWheelCallback(MouseWheelCallback callback)
    {
        m_mouseWheelCallback = callback;
    }

    void setKeyCallback(KeyCallback callback)
    {
        m_keyCallback = callback;
    }

    void setRenderCallback(RenderCallback callback)
    {
        m_renderCallback = callback;
    }

    void setCloseCallback(CloseCallback callback)
    {
        m_closeCallback = callback;
    }

    void dispatchImfCommit(const char* commitStr)
    {
        if (commitStr && m_webContainer) {
            m_webContainer->DispatchCompositionEndEvent(commitStr);
        }
    }

    void dispatchImfPreeditChanged(const char* preeditStr)
    {
        if (preeditStr && m_webContainer) {
            m_webContainer->DispatchCompositionUpdateEvent(preeditStr);
        }
    }

    static void eventCallback(void* event,
                              tizen_core_wl_event_type_e event_type,
                              void* user_data)
    {
        WebViewTcoreWl* win = static_cast<WebViewTcoreWl*>(user_data);
        tizen_core_wl_event_input_base_h inputEvent =
            (tizen_core_wl_event_input_base_h)event;

        switch (event_type) {
        case TIZEN_CORE_WL_EVENT_KEY_DOWN: {
            if (win->m_imfContext) {
                char* keyname = NULL;
                char* devId = NULL;
                unsigned int keycode;

                if (tizen_core_wl_event_key_get_keycode(inputEvent, &keycode) !=
                        TIZEN_CORE_WL_ERROR_NONE ||
                    tizen_core_wl_event_key_get_keyname(inputEvent, &keyname) !=
                        TIZEN_CORE_WL_ERROR_NONE ||
                    tizen_core_wl_event_input_base_get_device_identifier(
                        inputEvent, &devId) != TIZEN_CORE_WL_ERROR_NONE) {
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
                    if (keyname)
                        free(keyname);
                    if (devId)
                        free(devId);
                    return;
                }
                if (keyname)
                    free(keyname);
                if (devId)
                    free(devId);
            }

            if (win->m_keyCallback) {
                char* keyname = nullptr;
                unsigned int keycode = 0;
                unsigned int modifiers = 0;

                tizen_core_wl_event_key_get_keyname(inputEvent, &keyname);
                tizen_core_wl_event_key_get_keycode(inputEvent, &keycode);
                tizen_core_wl_event_key_get_modifiers(inputEvent, &modifiers);

                LWE::KeyValue keyValue = LWE::UnidentifiedKey;
                if (keyname) {
                    if (strcmp(keyname, "Left") == 0) {
                        keyValue = LWE::ArrowLeftKey;
                    } else if (strcmp(keyname, "Up") == 0) {
                        keyValue = LWE::ArrowUpKey;
                    } else if (strcmp(keyname, "Right") == 0) {
                        keyValue = LWE::ArrowRightKey;
                    } else if (strcmp(keyname, "Down") == 0) {
                        keyValue = LWE::ArrowDownKey;
                    } else if (strcmp(keyname, "Return") == 0) {
                        keyValue = LWE::EnterKey;
                    } else if (strcmp(keyname, "Escape") == 0) {
                        keyValue = LWE::EscapeKey;
                    } else if (strcmp(keyname, "BackSpace") == 0) {
                        keyValue = LWE::BackspaceKey;
                    } else if (strcmp(keyname, "Tab") == 0) {
                        keyValue = LWE::TabKey;
                    } else if (strcmp(keyname, "Delete") == 0) {
                        keyValue = LWE::DeleteKey;
                    } else if (strcmp(keyname, "Home") == 0) {
                        keyValue = LWE::HomeKey;
                    } else if (strcmp(keyname, "End") == 0) {
                        keyValue = LWE::EndKey;
                    } else if (strcmp(keyname, "Page_Up") == 0) {
                        keyValue = LWE::PageUpKey;
                    } else if (strcmp(keyname, "Page_Down") == 0) {
                        keyValue = LWE::PageDownKey;
                    } else if (strcmp(keyname, "Insert") == 0) {
                        keyValue = LWE::InsertKey;
                    } else if (strlen(keyname) == 1) {
                        unsigned char c = keyname[0];
                        if (c >= 'A' && c <= 'Z') {
                            keyValue = static_cast<LWE::KeyValue>(
                                static_cast<int>(LWE::AKey) + (c - 'A'));
                        } else if (c >= 'a' && c <= 'z') {
                            keyValue = static_cast<LWE::KeyValue>(
                                static_cast<int>(LWE::LowerAKey) + (c - 'a'));
                        } else if (c >= '0' && c <= '9') {
                            keyValue = static_cast<LWE::KeyValue>(
                                static_cast<int>(LWE::Digit0Key) + (c - '0'));
                        } else if (c == ' ') {
                            keyValue = LWE::SpaceKey;
                        } else {
                            keyValue = static_cast<LWE::KeyValue>(c);
                        }
                    } else if (strncmp(keyname, "F", 1) == 0 &&
                               strlen(keyname) <= 3) {
                        int fnum = atoi(keyname + 1);
                        if (fnum >= 1 && fnum <= 12) {
                            keyValue = static_cast<LWE::KeyValue>(
                                static_cast<int>(LWE::F1Key) + (fnum - 1));
                        }
                    } else {
                        keyValue = LWE::UnidentifiedKey;
                    }
                    free(keyname);
                }

                win->m_keyCallback(static_cast<int>(keyValue), 1);
            }
            break;
        }

        case TIZEN_CORE_WL_EVENT_KEY_UP: {
            if (win->m_keyCallback) {
                char* keyname = nullptr;
                unsigned int keycode = 0;

                tizen_core_wl_event_key_get_keyname(inputEvent, &keyname);
                tizen_core_wl_event_key_get_keycode(inputEvent, &keycode);

                LWE::KeyValue keyValue = LWE::UnidentifiedKey;
                if (keyname) {
                    if (strcmp(keyname, "Left") == 0) {
                        keyValue = LWE::ArrowLeftKey;
                    } else if (strcmp(keyname, "Up") == 0) {
                        keyValue = LWE::ArrowUpKey;
                    } else if (strcmp(keyname, "Right") == 0) {
                        keyValue = LWE::ArrowRightKey;
                    } else if (strcmp(keyname, "Down") == 0) {
                        keyValue = LWE::ArrowDownKey;
                    } else if (strcmp(keyname, "Return") == 0) {
                        keyValue = LWE::EnterKey;
                    } else if (strcmp(keyname, "Escape") == 0) {
                        keyValue = LWE::EscapeKey;
                    } else if (strcmp(keyname, "BackSpace") == 0) {
                        keyValue = LWE::BackspaceKey;
                    } else if (strcmp(keyname, "Tab") == 0) {
                        keyValue = LWE::TabKey;
                    } else if (strcmp(keyname, "Delete") == 0) {
                        keyValue = LWE::DeleteKey;
                    } else if (strcmp(keyname, "Home") == 0) {
                        keyValue = LWE::HomeKey;
                    } else if (strcmp(keyname, "End") == 0) {
                        keyValue = LWE::EndKey;
                    } else if (strcmp(keyname, "Page_Up") == 0) {
                        keyValue = LWE::PageUpKey;
                    } else if (strcmp(keyname, "Page_Down") == 0) {
                        keyValue = LWE::PageDownKey;
                    } else if (strcmp(keyname, "Insert") == 0) {
                        keyValue = LWE::InsertKey;
                    } else if (strlen(keyname) == 1) {
                        unsigned char c = keyname[0];
                        if (c >= 'A' && c <= 'Z') {
                            keyValue = static_cast<LWE::KeyValue>(
                                static_cast<int>(LWE::AKey) + (c - 'A'));
                        } else if (c >= 'a' && c <= 'z') {
                            keyValue = static_cast<LWE::KeyValue>(
                                static_cast<int>(LWE::LowerAKey) + (c - 'a'));
                        } else if (c >= '0' && c <= '9') {
                            keyValue = static_cast<LWE::KeyValue>(
                                static_cast<int>(LWE::Digit0Key) + (c - '0'));
                        } else if (c == ' ') {
                            keyValue = LWE::SpaceKey;
                        } else {
                            keyValue = static_cast<LWE::KeyValue>(c);
                        }
                    } else if (strncmp(keyname, "F", 1) == 0 &&
                               strlen(keyname) <= 3) {
                        int fnum = atoi(keyname + 1);
                        if (fnum >= 1 && fnum <= 12) {
                            keyValue = static_cast<LWE::KeyValue>(
                                static_cast<int>(LWE::F1Key) + (fnum - 1));
                        }
                    } else {
                        keyValue = LWE::UnidentifiedKey;
                    }
                    free(keyname);
                }

                win->m_keyCallback(static_cast<int>(keyValue), 0);
            }
            break;
        }

        case TIZEN_CORE_WL_EVENT_MOUSE_BUTTON_DOWN: {
            if (win->m_mouseButtonCallback) {
                tizen_core_wl_event_input_base_h inputEvent =
                    (tizen_core_wl_event_input_base_h)event;
                int x = 0, y = 0;
                tizen_core_wl_event_mouse_button_get_position(inputEvent, &x,
                                                              &y);
                win->m_mouseX = x;
                win->m_mouseY = y;
                win->m_mouseButtonCallback(1, x, y);
            }
            break;
        }

        case TIZEN_CORE_WL_EVENT_MOUSE_BUTTON_UP: {
            if (win->m_mouseButtonCallback) {
                tizen_core_wl_event_input_base_h inputEvent =
                    (tizen_core_wl_event_input_base_h)event;
                int x = 0, y = 0;
                tizen_core_wl_event_mouse_button_get_position(inputEvent, &x,
                                                              &y);
                win->m_mouseX = x;
                win->m_mouseY = y;
                win->m_mouseButtonCallback(0, x, y);
            }
            break;
        }

        case TIZEN_CORE_WL_EVENT_MOUSE_MOVE: {
            if (win->m_mouseMoveCallback) {
                tizen_core_wl_event_input_base_h inputEvent =
                    (tizen_core_wl_event_input_base_h)event;
                int x = 0, y = 0;
                tizen_core_wl_event_mouse_move_get_position(inputEvent, &x, &y);
                win->m_mouseMoveCallback(x, y);
            }
            break;
        }

        case TIZEN_CORE_WL_EVENT_MOUSE_WHEEL: {
            if (win->m_mouseWheelCallback) {
                tizen_core_wl_event_input_base_h inputEvent =
                    (tizen_core_wl_event_input_base_h)event;
                int x = 0, y = 0, z = 0;
                tizen_core_wl_event_mouse_wheel_get_position(inputEvent, &x,
                                                             &y);
                tizen_core_wl_event_mouse_wheel_get_z(inputEvent, &z);
                win->m_mouseWheelCallback(z, x, y);
            }
            break;
        }

        case TIZEN_CORE_WL_EVENT_WINDOW_CONFIGURE: {
            int x = 0, y = 0, w = 0, h = 0;
            tizen_core_wl_window_get_geometry(win->m_window, &x, &y, &w, &h);
            if (win->m_lastWidth == w && win->m_lastHeight == h) {
                break;
            }

            if (!win->resizeSurface(w, h)) {
                STARFISH_LOG_ERROR(
                    "Failed to create new EGL surface after resize");
            } else {
                if (!eglMakeCurrent(win->m_eglDisplay, win->m_eglSurface,
                                    win->m_eglSurface, win->m_eglContext)) {
                    STARFISH_LOG_ERROR(
                        "Failed to make context current after resize");
                }
            }

            win->m_lastWidth = w;
            win->m_lastHeight = h;

            if (win->m_resizeCallback) {
                win->m_resizeCallback(w, h);
            }
            break;
        }

        case TIZEN_CORE_WL_EVENT_WINDOW_DESTROY: {
            if (win->m_closeCallback) {
                win->m_closeCallback();
            }
            break;
        }

        default:
            break;
        }
    }

    bool resizeSurface(int w, int h)
    {
        eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE,
                       EGL_NO_CONTEXT);

        // Destroy old surface
        if (m_eglSurface != EGL_NO_SURFACE) {
            eglDestroySurface(m_eglDisplay, m_eglSurface);
            m_eglSurface = EGL_NO_SURFACE;
        }

        // Resize the EGL window
        tizen_core_wl_egl_window_resize(m_eglWindow, w, h);

        // Create new surface with the new window size
        if (!createEGLSurface(m_eglSurface, m_eglDisplay, m_eglConfig,
                              m_eglWindow)) {
            return false;
        }

        // Make the context current again with the new surface
        if (!eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface,
                            m_eglContext)) {
            printf(
                "Failed to make context current after resize (eglError: "
                "0x%x)\n",
                eglGetError());
            return false;
        }

        return true;
    }

    void setupEventHandlers()
    {
        if (!m_display) {
            return;
        }

        tizen_core_wl_display_get_event(m_display, &m_eventHandle);

        tizen_core_wl_event_add_listener(
            m_eventHandle, TIZEN_CORE_WL_EVENT_KEY_DOWN, eventCallback, this,
            &m_keyDownListener);
        tizen_core_wl_event_add_listener(m_eventHandle,
                                         TIZEN_CORE_WL_EVENT_KEY_UP,
                                         eventCallback, this, &m_keyUpListener);
        tizen_core_wl_event_add_listener(
            m_eventHandle, TIZEN_CORE_WL_EVENT_MOUSE_BUTTON_DOWN, eventCallback,
            this, &m_mouseDownListener);
        tizen_core_wl_event_add_listener(
            m_eventHandle, TIZEN_CORE_WL_EVENT_MOUSE_BUTTON_UP, eventCallback,
            this, &m_mouseUpListener);
        tizen_core_wl_event_add_listener(
            m_eventHandle, TIZEN_CORE_WL_EVENT_MOUSE_MOVE, eventCallback, this,
            &m_mouseMoveListener);
        tizen_core_wl_event_add_listener(
            m_eventHandle, TIZEN_CORE_WL_EVENT_MOUSE_WHEEL, eventCallback, this,
            &m_mouseWheelListener);
        tizen_core_wl_event_add_listener(
            m_eventHandle, TIZEN_CORE_WL_EVENT_WINDOW_CONFIGURE, eventCallback,
            this, &m_windowConfigureListener);
        tizen_core_wl_event_add_listener(
            m_eventHandle, TIZEN_CORE_WL_EVENT_WINDOW_DESTROY, eventCallback,
            this, &m_windowDestroyListener);
    }

    void setupIMF()
    {
        if (m_isImfInitialized) {
            return;
        }

        if (tizen_core_imf_init() != TIZEN_CORE_IMF_ERROR_NONE) {
            printf("Warning: Failed to initialize tizen_core_imf\n");
            return;
        }

        if (tizen_core_imf_context_create(&m_imfContext) !=
            TIZEN_CORE_IMF_ERROR_NONE) {
            printf("Warning: Failed to create IMF context\n");
            tizen_core_imf_shutdown();
            return;
        }

        tizen_core_imf_context_set_client_window(m_imfContext, (void*)m_window);

        tizen_core_imf_context_add_event_callback(
            m_imfContext, TIZEN_CORE_IMF_CALLBACK_COMMIT,
            [](tizen_core_imf_context_h ctx, void* event_info,
               void* user_data) {
                WebViewTcoreWl* self = static_cast<WebViewTcoreWl*>(user_data);
                char* commitStr = static_cast<char*>(event_info);
                self->dispatchImfCommit(commitStr);
            },
            this);

        tizen_core_imf_context_add_event_callback(
            m_imfContext, TIZEN_CORE_IMF_CALLBACK_PREEDIT_CHANGED,
            [](tizen_core_imf_context_h ctx, void* event_info,
               void* user_data) {
                WebViewTcoreWl* self = static_cast<WebViewTcoreWl*>(user_data);
                char* preeditStr = nullptr;
                int cursorPos = 0;
                tizen_core_imf_preedit_attr_h* attrs = nullptr;
                int attrsCount = 0;

                if (tizen_core_imf_context_get_preedit_string(
                        self->m_imfContext, &preeditStr, &attrs, &attrsCount,
                        &cursorPos) == TIZEN_CORE_IMF_ERROR_NONE) {
                    self->dispatchImfPreeditChanged(preeditStr);
                    if (preeditStr)
                        free(preeditStr);
                    if (attrs) {
                        for (int i = 0; i < attrsCount; i++) {
                        }
                        free(attrs);
                    }
                }
            },
            this);

        m_isImfInitialized = true;
    }

    void cleanupIMF()
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

private:
    bool m_ownsWindow;
    tizen_core_wl_display_h m_display = nullptr;
    tizen_core_wl_window_h m_window;
    tizen_core_wl_egl_window_h m_eglWindow;
    EGLDisplay m_eglDisplay;
    EGLSurface m_eglSurface;
    EGLContext m_eglContext;
    EGLConfig m_eglConfig;
    int m_lastWidth;
    int m_lastHeight;

    WebContainer* m_webContainer;

    ResizeCallback m_resizeCallback;
    MouseMoveCallback m_mouseMoveCallback;
    MouseButtonCallback m_mouseButtonCallback;
    MouseWheelCallback m_mouseWheelCallback;
    KeyCallback m_keyCallback;
    RenderCallback m_renderCallback;
    CloseCallback m_closeCallback;

    tizen_core_imf_context_h m_imfContext;
    bool m_isImfInitialized;

    tizen_core_wl_event_listener_h m_keyDownListener;
    tizen_core_wl_event_listener_h m_keyUpListener;
    tizen_core_wl_event_listener_h m_mouseDownListener;
    tizen_core_wl_event_listener_h m_mouseUpListener;
    tizen_core_wl_event_listener_h m_mouseMoveListener;
    tizen_core_wl_event_listener_h m_mouseWheelListener;
    tizen_core_wl_event_listener_h m_windowConfigureListener;
    tizen_core_wl_event_listener_h m_windowDestroyListener;
    tizen_core_event_h m_eventHandle;

    int m_mouseX;
    int m_mouseY;
};

WebView* WebView::Create(void* win, unsigned x, unsigned y, unsigned width,
                         unsigned height, float devicePixelRatio,
                         const char* defaultFontName, const char* locale,
                         const char* timezoneID)
{
    return new WebViewTcoreWl(win, x, y, width, height, devicePixelRatio,
                              defaultFontName, locale, timezoneID);
}

} // namespace LWEDelegate

#endif
