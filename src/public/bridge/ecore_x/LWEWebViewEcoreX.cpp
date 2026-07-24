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

#if defined(STARFISH_SHELL_ECORE_X)

#define STARFISH_ENABLE_PROFILE_TIMER

#include "StarfishConfig.h"
#include "PlatformIntegrationData.h"
#include "public/delegate/LWEWebViewDelegateImpl.h"
#include "public/delegate/LWEWebContainerDelegate.h"
#include "LWEWebView.h"

#include <Ecore.h>
#include <Ecore_X.h>
#include <Ecore_Input.h>
#include <Ecore_IMF.h>
#include <EGL/egl.h>

#if defined(PORT_EVENTLOOP_BACKEND_LIBUV)
#include <uv.h>
#endif

#include <memory>
#include <cstring>
#include <locale>
#include <csignal>

namespace {

bool createEGLDisplay(EGLDisplay& display, EGLConfig& config)
{
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

} // namespace

namespace LWEDelegate {

using namespace LWE;

class WebViewEcoreX : public WebViewImpl {
public:
    WebViewEcoreX(void* winArg, unsigned x, unsigned y, unsigned width,
                  unsigned height, float devicePixelRatio,
                  const char* defaultFontName, const char* locale,
                  const char* timezoneID)
        : m_ownsWindow(false)
        , m_window(0)
        , m_eglDisplay(EGL_NO_DISPLAY)
        , m_eglSurface(EGL_NO_SURFACE)
        , m_eglContext(EGL_NO_CONTEXT)
        , m_lastWidth(width)
        , m_lastHeight(height)
        , m_webContainer(nullptr)
        , m_imfContext(nullptr)
        , m_isImfInitialized(false)
        , m_keyDownHandler(nullptr)
        , m_keyUpHandler(nullptr)
        , m_mouseDownHandler(nullptr)
        , m_mouseUpHandler(nullptr)
        , m_mouseMoveHandler(nullptr)
        , m_mouseWheelHandler(nullptr)
        , m_windowResizeHandler(nullptr)
        , m_windowDeleteHandler(nullptr)
    {
        STARFISH_LOG_INFO("WebViewEcoreX::WebViewEcoreX");

        if (!ecore_x_init(nullptr)) {
            STARFISH_LOG_ERROR("Cannot initialize ecore_x");
            return;
        }

        if (winArg == nullptr) {
            m_window = ecore_x_window_new(0, 0, 0, width, height);
            if (!m_window) {
                STARFISH_LOG_ERROR("Failed to create window");
                ecore_x_shutdown();
                return;
            }

            ecore_x_icccm_title_set(m_window, "Starfish");
            ecore_x_netwm_window_type_set(m_window, ECORE_X_WINDOW_TYPE_NORMAL);

            ecore_x_event_mask_set(
                m_window, Ecore_X_Event_Mask(ECORE_X_EVENT_MASK_KEY_DOWN |
                                             ECORE_X_EVENT_MASK_KEY_UP |
                                             ECORE_X_EVENT_MASK_MOUSE_DOWN |
                                             ECORE_X_EVENT_MASK_MOUSE_UP |
                                             ECORE_X_EVENT_MASK_MOUSE_MOVE |
                                             ECORE_X_EVENT_MASK_MOUSE_WHEEL));

            ecore_x_icccm_protocol_set(
                m_window, ECORE_X_WM_PROTOCOL_DELETE_REQUEST, EINA_TRUE);

            ecore_x_window_show(m_window);
            m_ownsWindow = true;
        } else {
            unsigned int val =
                static_cast<unsigned int>(reinterpret_cast<uintptr_t>(winArg));
            m_window = (Ecore_X_Window)val;
            m_ownsWindow = false;
        }

        if (!initializeEGL()) {
            STARFISH_LOG_ERROR("Failed to initialize EGL");
            return;
        }

        ecore_main_loop_glib_integrate();

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
            MouseButtonsValue buttons = m_isMouseLbuttonDown
                                            ? MouseButtonsValue::LeftButtonDown
                                            : MouseButtonsValue::NoButtonDown;
            m_webContainer->DispatchMouseMoveEvent(MouseButtonValue::NoButton,
                                                   buttons, x, y);
        });

        setMouseButtonCallback([this](int button, int x, int y) {
            if (button == 1) {
                m_isMouseLbuttonDown = true;
                m_webContainer->DispatchMouseDownEvent(
                    MouseButtonValue::LeftButton,
                    MouseButtonsValue::LeftButtonDown, x, y);
            } else if (button == 0) {
                m_isMouseLbuttonDown = false;
                m_webContainer->DispatchMouseUpEvent(
                    MouseButtonValue::LeftButton,
                    MouseButtonsValue::NoButtonDown, x, y);
            }
        });

        setMouseWheelCallback([this](int delta, int x, int y) {
            m_webContainer->DispatchMouseWheelEvent(x, y, delta);
        });

        setKeyCallback([this](int key, int isKeyDown) {
            ::LWE::KeyValue keyValue = static_cast<::LWE::KeyValue>(key);
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
                    ecore_imf_context_focus_in(m_imfContext);
                    ecore_imf_context_show(m_imfContext);
                }
            });

        m_webContainer->RegisterOnHideSoftwareKeyboardIfPossibleHandler(
            [this](WebContainer* t) {
                if (m_imfContext) {
                    ecore_imf_context_hide(m_imfContext);
                    ecore_imf_context_focus_out(m_imfContext);
                }
            });
    }

    ~WebViewEcoreX()
    {
        if (m_webContainer) {
            m_webContainer->Destroy();
            m_webContainer = nullptr;
        }

        cleanupIMF();

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

        cleanupEGL();

        if (m_ownsWindow && m_window) {
            ecore_x_window_free(m_window);
            ecore_x_shutdown();
        }
    }

    virtual void Destroy() override
    {
        delete this;
    }

    void render()
    {
        makeCurrent();
        if (m_renderCallback) {
            m_renderCallback();
        }
        swapBuffers();
    }

    using ResizeCallback = std::function<void(int, int)>;
    using MouseMoveCallback = std::function<void(int, int)>;
    using MouseButtonCallback = std::function<void(int, int, int)>;
    using MouseWheelCallback = std::function<void(int, int, int)>;
    using KeyCallback = std::function<void(int, int)>;
    using RenderCallback = std::function<void()>;
    using CloseCallback = std::function<void()>;

    void setResizeCallback(ResizeCallback cb)
    {
        m_resizeCallback = cb;
    }
    void setMouseMoveCallback(MouseMoveCallback cb)
    {
        m_mouseMoveCallback = cb;
    }
    void setMouseButtonCallback(MouseButtonCallback cb)
    {
        m_mouseButtonCallback = cb;
    }
    void setMouseWheelCallback(MouseWheelCallback cb)
    {
        m_mouseWheelCallback = cb;
    }
    void setKeyCallback(KeyCallback cb)
    {
        m_keyCallback = cb;
    }
    void setRenderCallback(RenderCallback cb)
    {
        m_renderCallback = cb;
    }
    void setCloseCallback(CloseCallback cb)
    {
        m_closeCallback = cb;
    }

    Ecore_X_Window getEcoreXWindow()
    {
        return m_window;
    }

    bool initializeEGL()
    {
        if (!createEGLDisplay(m_eglDisplay, m_eglConfig) ||
            !createEGLSurface(m_eglSurface, m_eglDisplay, m_eglConfig,
                              m_window) ||
            !createGLContext(m_eglContext, m_eglDisplay, m_eglConfig,
                             nullptr)) {
            return false;
        }
        return true;
    }

    void cleanupEGL()
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

    void setupEventHandlers()
    {
        m_keyDownHandler = ecore_event_handler_add(
            ECORE_EVENT_KEY_DOWN,
            [](void* data, int type, void* event) -> Eina_Bool {
                WebViewEcoreX* win = static_cast<WebViewEcoreX*>(data);
                Ecore_Event_Key* keyEvent =
                    static_cast<Ecore_Event_Key*>(event);

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

                if (win->m_keyCallback && keyEvent->keyname) {
                    ::LWE::KeyValue keyValue = ::LWE::UnidentifiedKey;
                    const char* keyname = keyEvent->keyname;

                    if (strcmp(keyname, "Left") == 0) {
                        keyValue = ::LWE::ArrowLeftKey;
                    } else if (strcmp(keyname, "Up") == 0) {
                        keyValue = ::LWE::ArrowUpKey;
                    } else if (strcmp(keyname, "Right") == 0) {
                        keyValue = ::LWE::ArrowRightKey;
                    } else if (strcmp(keyname, "Down") == 0) {
                        keyValue = ::LWE::ArrowDownKey;
                    } else if (strcmp(keyname, "Return") == 0) {
                        keyValue = ::LWE::EnterKey;
                    } else if (strcmp(keyname, "Escape") == 0) {
                        keyValue = ::LWE::EscapeKey;
                    } else if (strcmp(keyname, "BackSpace") == 0) {
                        keyValue = ::LWE::BackspaceKey;
                    } else if (strcmp(keyname, "Tab") == 0) {
                        keyValue = ::LWE::TabKey;
                    } else if (strcmp(keyname, "Delete") == 0) {
                        keyValue = ::LWE::DeleteKey;
                    } else if (strcmp(keyname, "Home") == 0) {
                        keyValue = ::LWE::HomeKey;
                    } else if (strcmp(keyname, "End") == 0) {
                        keyValue = ::LWE::EndKey;
                    } else if (strcmp(keyname, "Page_Up") == 0) {
                        keyValue = ::LWE::PageUpKey;
                    } else if (strcmp(keyname, "Page_Down") == 0) {
                        keyValue = ::LWE::PageDownKey;
                    } else if (strcmp(keyname, "Insert") == 0) {
                        keyValue = ::LWE::InsertKey;
                    } else if (strlen(keyname) == 1) {
                        unsigned char c = keyname[0];
                        if (c >= 'A' && c <= 'Z') {
                            keyValue = static_cast<::LWE::KeyValue>(
                                static_cast<int>(::LWE::AKey) + (c - 'A'));
                        } else if (c >= 'a' && c <= 'z') {
                            keyValue = static_cast<::LWE::KeyValue>(
                                static_cast<int>(::LWE::LowerAKey) + (c - 'a'));
                        } else if (c >= '0' && c <= '9') {
                            keyValue = static_cast<::LWE::KeyValue>(
                                static_cast<int>(::LWE::Digit0Key) + (c - '0'));
                        } else if (c == ' ') {
                            keyValue = ::LWE::SpaceKey;
                        } else {
                            keyValue = static_cast<::LWE::KeyValue>(c);
                        }
                    } else if (strncmp(keyname, "F", 1) == 0 &&
                               strlen(keyname) <= 3) {
                        int fnum = atoi(keyname + 1);
                        if (fnum >= 1 && fnum <= 12) {
                            keyValue = static_cast<::LWE::KeyValue>(
                                static_cast<int>(::LWE::F1Key) + (fnum - 1));
                        }
                    } else {
                        keyValue = ::LWE::UnidentifiedKey;
                    }

                    win->m_keyCallback(static_cast<int>(keyValue), 1);
                }
                return ECORE_CALLBACK_PASS_ON;
            },
            this);

        m_keyUpHandler = ecore_event_handler_add(
            ECORE_EVENT_KEY_UP,
            [](void* data, int type, void* event) -> Eina_Bool {
                WebViewEcoreX* win = static_cast<WebViewEcoreX*>(data);
                Ecore_Event_Key* keyEvent =
                    static_cast<Ecore_Event_Key*>(event);

                if (win->m_keyCallback && keyEvent->keyname) {
                    ::LWE::KeyValue keyValue = ::LWE::UnidentifiedKey;
                    const char* keyname = keyEvent->keyname;

                    if (strcmp(keyname, "Left") == 0) {
                        keyValue = ::LWE::ArrowLeftKey;
                    } else if (strcmp(keyname, "Up") == 0) {
                        keyValue = ::LWE::ArrowUpKey;
                    } else if (strcmp(keyname, "Right") == 0) {
                        keyValue = ::LWE::ArrowRightKey;
                    } else if (strcmp(keyname, "Down") == 0) {
                        keyValue = ::LWE::ArrowDownKey;
                    } else if (strcmp(keyname, "Return") == 0) {
                        keyValue = ::LWE::EnterKey;
                    } else if (strcmp(keyname, "Escape") == 0) {
                        keyValue = ::LWE::EscapeKey;
                    } else if (strcmp(keyname, "BackSpace") == 0) {
                        keyValue = ::LWE::BackspaceKey;
                    } else if (strcmp(keyname, "Tab") == 0) {
                        keyValue = ::LWE::TabKey;
                    } else if (strcmp(keyname, "Delete") == 0) {
                        keyValue = ::LWE::DeleteKey;
                    } else if (strcmp(keyname, "Home") == 0) {
                        keyValue = ::LWE::HomeKey;
                    } else if (strcmp(keyname, "End") == 0) {
                        keyValue = ::LWE::EndKey;
                    } else if (strcmp(keyname, "Page_Up") == 0) {
                        keyValue = ::LWE::PageUpKey;
                    } else if (strcmp(keyname, "Page_Down") == 0) {
                        keyValue = ::LWE::PageDownKey;
                    } else if (strcmp(keyname, "Insert") == 0) {
                        keyValue = ::LWE::InsertKey;
                    } else if (strlen(keyname) == 1) {
                        unsigned char c = keyname[0];
                        if (c >= 'A' && c <= 'Z') {
                            keyValue = static_cast<::LWE::KeyValue>(
                                static_cast<int>(::LWE::AKey) + (c - 'A'));
                        } else if (c >= 'a' && c <= 'z') {
                            keyValue = static_cast<::LWE::KeyValue>(
                                static_cast<int>(::LWE::LowerAKey) + (c - 'a'));
                        } else if (c >= '0' && c <= '9') {
                            keyValue = static_cast<::LWE::KeyValue>(
                                static_cast<int>(::LWE::Digit0Key) + (c - '0'));
                        } else if (c == ' ') {
                            keyValue = ::LWE::SpaceKey;
                        } else {
                            keyValue = static_cast<::LWE::KeyValue>(c);
                        }
                    } else if (strncmp(keyname, "F", 1) == 0 &&
                               strlen(keyname) <= 3) {
                        int fnum = atoi(keyname + 1);
                        if (fnum >= 1 && fnum <= 12) {
                            keyValue = static_cast<::LWE::KeyValue>(
                                static_cast<int>(::LWE::F1Key) + (fnum - 1));
                        }
                    } else {
                        keyValue = ::LWE::UnidentifiedKey;
                    }

                    win->m_keyCallback(static_cast<int>(keyValue), 0);
                }
                return ECORE_CALLBACK_PASS_ON;
            },
            this);

        m_mouseDownHandler = ecore_event_handler_add(
            ECORE_EVENT_MOUSE_BUTTON_DOWN,
            [](void* data, int type, void* event) -> Eina_Bool {
                WebViewEcoreX* win = static_cast<WebViewEcoreX*>(data);
                Ecore_Event_Mouse_Button* buttonEvent =
                    static_cast<Ecore_Event_Mouse_Button*>(event);

                if (win->m_mouseButtonCallback) {
                    if (buttonEvent->buttons == 1) {
                        win->m_mouseButtonCallback(1, buttonEvent->x,
                                                   buttonEvent->y);
                    }
                }
                return ECORE_CALLBACK_PASS_ON;
            },
            this);

        m_mouseUpHandler = ecore_event_handler_add(
            ECORE_EVENT_MOUSE_BUTTON_UP,
            [](void* data, int type, void* event) -> Eina_Bool {
                WebViewEcoreX* win = static_cast<WebViewEcoreX*>(data);
                Ecore_Event_Mouse_Button* buttonEvent =
                    static_cast<Ecore_Event_Mouse_Button*>(event);

                if (win->m_mouseButtonCallback) {
                    if (buttonEvent->buttons == 1) {
                        win->m_mouseButtonCallback(0, buttonEvent->x,
                                                   buttonEvent->y);
                    }
                }
                return ECORE_CALLBACK_PASS_ON;
            },
            this);

        m_mouseMoveHandler = ecore_event_handler_add(
            ECORE_EVENT_MOUSE_MOVE,
            [](void* data, int type, void* event) -> Eina_Bool {
                WebViewEcoreX* win = static_cast<WebViewEcoreX*>(data);
                Ecore_Event_Mouse_Move* moveEvent =
                    static_cast<Ecore_Event_Mouse_Move*>(event);

                if (win->m_mouseMoveCallback) {
                    win->m_mouseMoveCallback(moveEvent->x, moveEvent->y);
                }
                return ECORE_CALLBACK_PASS_ON;
            },
            this);

        m_mouseWheelHandler = ecore_event_handler_add(
            ECORE_EVENT_MOUSE_WHEEL,
            [](void* data, int type, void* event) -> Eina_Bool {
                WebViewEcoreX* win = static_cast<WebViewEcoreX*>(data);
                Ecore_Event_Mouse_Wheel* wheelEvent =
                    static_cast<Ecore_Event_Mouse_Wheel*>(event);

                if (win->m_mouseWheelCallback) {
                    win->m_mouseWheelCallback(wheelEvent->z, wheelEvent->x,
                                              wheelEvent->y);
                }
                return ECORE_CALLBACK_PASS_ON;
            },
            this);

        m_windowResizeHandler = ecore_event_handler_add(
            ECORE_X_EVENT_WINDOW_CONFIGURE,
            [](void* data, int type, void* event) -> Eina_Bool {
                WebViewEcoreX* win = static_cast<WebViewEcoreX*>(data);
                Ecore_X_Event_Window_Configure* configureEvent =
                    static_cast<Ecore_X_Event_Window_Configure*>(event);

                if (configureEvent->win == win->m_window) {
                    if (win->m_eglSurface != EGL_NO_SURFACE) {
                        eglMakeCurrent(win->m_eglDisplay, EGL_NO_SURFACE,
                                       EGL_NO_SURFACE, EGL_NO_CONTEXT);
                        eglDestroySurface(win->m_eglDisplay, win->m_eglSurface);
                        win->m_eglSurface = EGL_NO_SURFACE;
                    }

                    if (!createEGLSurface(win->m_eglSurface, win->m_eglDisplay,
                                          win->m_eglConfig, win->m_window)) {
                        STARFISH_LOG_ERROR(
                            "Failed to create new EGL surface after resize");
                    } else {
                        if (!eglMakeCurrent(
                                win->m_eglDisplay, win->m_eglSurface,
                                win->m_eglSurface, win->m_eglContext)) {
                            STARFISH_LOG_ERROR(
                                "Failed to make context current after resize");
                        }
                    }

                    win->m_lastWidth = configureEvent->w;
                    win->m_lastHeight = configureEvent->h;

                    if (win->m_resizeCallback) {
                        win->m_resizeCallback(configureEvent->w,
                                              configureEvent->h);
                    }
                }
                return ECORE_CALLBACK_PASS_ON;
            },
            this);

        m_windowDeleteHandler = ecore_event_handler_add(
            ECORE_X_EVENT_WINDOW_DELETE_REQUEST,
            [](void* data, int type, void* event) -> Eina_Bool {
                WebViewEcoreX* win = static_cast<WebViewEcoreX*>(data);
                Ecore_X_Event_Window_Delete_Request* deleteEvent =
                    static_cast<Ecore_X_Event_Window_Delete_Request*>(event);

                if (deleteEvent->win == win->m_window) {
                    if (win->m_closeCallback) {
                        win->m_closeCallback();
                    }
                }
                return ECORE_CALLBACK_PASS_ON;
            },
            this);
    }

    void setupIMF()
    {
        if (m_isImfInitialized) {
            return;
        }

        ecore_imf_init();

        const char* imfMethod = ecore_imf_context_default_id_get();
        if (!imfMethod) {
            printf("Warning: No default IMF method available\n");
            return;
        }

        m_imfContext = ecore_imf_context_add(imfMethod);
        if (!m_imfContext) {
            printf("Warning: Failed to create IMF context\n");
            return;
        }

        ecore_imf_context_client_window_set(m_imfContext,
                                            (void*)(uintptr_t)m_window);

        ecore_imf_context_event_callback_add(
            m_imfContext, ECORE_IMF_CALLBACK_COMMIT,
            [](void* data, Ecore_IMF_Context* ctx, void* event_info) {
                WebViewEcoreX* self = static_cast<WebViewEcoreX*>(data);
                char* commitStr = static_cast<char*>(event_info);
                if (commitStr && self->m_webContainer) {
                    self->m_webContainer->DispatchCompositionEndEvent(
                        commitStr);
                }
            },
            this);

        ecore_imf_context_event_callback_add(
            m_imfContext, ECORE_IMF_CALLBACK_PREEDIT_CHANGED,
            [](void* data, Ecore_IMF_Context* ctx, void* event_info) {
                WebViewEcoreX* self = static_cast<WebViewEcoreX*>(data);
                char* preeditStr = nullptr;
                int cursorPos = 0;
                ecore_imf_context_preedit_string_get(self->m_imfContext,
                                                     &preeditStr, &cursorPos);
                if (preeditStr) {
                    if (self->m_webContainer) {
                        self->m_webContainer->DispatchCompositionUpdateEvent(
                            preeditStr);
                    }
                    free(preeditStr);
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

        ecore_imf_context_hide(m_imfContext);
        ecore_imf_context_focus_out(m_imfContext);
        ecore_imf_context_reset(m_imfContext);
        ecore_imf_context_client_window_set(m_imfContext, nullptr);
        ecore_imf_context_del(m_imfContext);
        m_imfContext = nullptr;
        ecore_imf_shutdown();
        m_isImfInitialized = false;
    }

private:
    bool m_ownsWindow;
    Ecore_X_Window m_window;
    EGLDisplay m_eglDisplay;
    EGLSurface m_eglSurface;
    EGLContext m_eglContext;
    EGLConfig m_eglConfig;
    int m_lastWidth;
    int m_lastHeight;
    bool m_isMouseLbuttonDown = false;

    WebContainer* m_webContainer;

    ResizeCallback m_resizeCallback;
    MouseMoveCallback m_mouseMoveCallback;
    MouseButtonCallback m_mouseButtonCallback;
    MouseWheelCallback m_mouseWheelCallback;
    KeyCallback m_keyCallback;
    RenderCallback m_renderCallback;
    CloseCallback m_closeCallback;

    Ecore_IMF_Context* m_imfContext;
    bool m_isImfInitialized;

    Ecore_Event_Handler* m_keyDownHandler;
    Ecore_Event_Handler* m_keyUpHandler;
    Ecore_Event_Handler* m_mouseDownHandler;
    Ecore_Event_Handler* m_mouseUpHandler;
    Ecore_Event_Handler* m_mouseMoveHandler;
    Ecore_Event_Handler* m_mouseWheelHandler;
    Ecore_Event_Handler* m_windowResizeHandler;
    Ecore_Event_Handler* m_windowDeleteHandler;
};

WebView* WebView::Create(void* win, unsigned x, unsigned y, unsigned width,
                         unsigned height, float devicePixelRatio,
                         const char* defaultFontName, const char* locale,
                         const char* timezoneID)
{
    return new WebViewEcoreX(win, x, y, width, height, devicePixelRatio,
                             defaultFontName, locale, timezoneID);
}

} // namespace LWEDelegate

#endif
