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

#if defined(STARFISH_SHELL_X11)

#define STARFISH_ENABLE_PROFILE_TIMER

// define mini class gc for avoding `GC` name collusion
#define GC_CPP_H
#include <stddef.h>
class gc {
public:
    inline void* operator new(size_t);
    inline void operator delete(void*);
};

#include "StarfishConfig.h"
#include "PlatformIntegrationData.h"
#include "public/delegate/LWEWebViewDelegateImpl.h"
#include "public/delegate/LWEWebContainerDelegate.h"
#include "LWEWebView.h"

void* gc::operator new(size_t n)
{
    return GC_MALLOC(n);
}

void gc::operator delete(void* p)
{
    return GC_FREE(p);
}

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xlocale.h>
#include <X11/Xatom.h>
#include <X11/keysymdef.h>
#include <EGL/egl.h>
#include <glib-unix.h>

#include <memory>
#include <cstring>
#include <locale>

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

} // namespace

namespace LWEDelegate {

using namespace LWE;

class WebViewX11 : public WebViewImpl {
public:
    WebViewX11(void* winArg, unsigned x, unsigned y, unsigned width,
               unsigned height, float devicePixelRatio,
               const char* defaultFontName, const char* locale,
               const char* timezoneID)
        : m_ownsWindow(false)
        , m_display(nullptr)
        , m_window(0)
        , m_eglDisplay(EGL_NO_DISPLAY)
        , m_eglSurface(EGL_NO_SURFACE)
        , m_eglContext(EGL_NO_CONTEXT)
        , m_wmDeleteWindow(0)
        , m_im(nullptr)
        , m_ic(nullptr)
        , m_lastWidth(width)
        , m_lastHeight(height)
        , m_webContainer(nullptr)
        , m_x11FdSource(0)
    {
        STARFISH_LOG_INFO("WebViewX11::WebViewX11");

        if (!setlocale(LC_ALL, "")) {
            STARFISH_LOG_ERROR("Cannot set system locale");
        }

        m_display = XOpenDisplay(nullptr);
        if (m_display == nullptr) {
            STARFISH_LOG_ERROR("Cannot open display");
            return;
        }

        const char* xmodifiers = getenv("XMODIFIERS");
        if (!XSetLocaleModifiers(xmodifiers ? xmodifiers : "")) {
            STARFISH_LOG_ERROR("Cannot set X modifiers");
        }

        if (winArg == nullptr) {
            m_window = XCreateSimpleWindow(
                m_display, DefaultRootWindow(m_display), x, y, width, height, 0,
                0, WhitePixel(m_display, 0));
            m_ownsWindow = true;

            const long eventMask = StructureNotifyMask | ButtonPressMask |
                                   PointerMotionMask | ButtonReleaseMask |
                                   KeyPressMask | KeyReleaseMask;
            XSelectInput(m_display, m_window, eventMask);

            XStoreName(m_display, m_window, "Starfish");
            XMapWindow(m_display, m_window);

            m_wmDeleteWindow = XInternAtom(m_display, "WM_DELETE_WINDOW", true);
            XSetWMProtocols(m_display, m_window, &m_wmDeleteWindow, 1);

            XFlush(m_display);
        } else {
            m_window = reinterpret_cast<unsigned long>(winArg);
            m_ownsWindow = false;
        }

        m_im = XOpenIM(m_display, nullptr, nullptr, nullptr);
        if (!m_im) {
            STARFISH_LOG_ERROR("Cannot connect to XIM server");
        } else {
            m_ic = XCreateIC(
                m_im, XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
                XNClientWindow, m_window, XNFocusWindow, m_window, nullptr);
            if (!m_ic) {
                STARFISH_LOG_ERROR("Cannot create XIC");
                XCloseIM(m_im);
                m_im = nullptr;
            }
        }

        if (!initializeEGL()) {
            STARFISH_LOG_ERROR("Failed to initialize EGL");
            return;
        }

        // Register X11 file descriptor to glib main loop
        registerX11Fd();

        // Create WebContainer with GL rendering
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

        // Connect resize callback to WebContainer
        setResizeCallback(
            [this](int w, int h) { m_webContainer->ResizeTo(w, h); });

        // Connect mouse events to WebContainer
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

        // Connect key events to WebContainer
        setKeyCallback([this](int key, int isKeyDown) {
            KeyValue keyValue = static_cast<KeyValue>(key);
            if (isKeyDown) {
                m_webContainer->DispatchKeyDownEvent(keyValue);
                m_webContainer->DispatchKeyPressEvent(keyValue);
            } else {
                m_webContainer->DispatchKeyUpEvent(keyValue);
            }
        });

        // Connect close callback
        setCloseCallback([this]() {
            STARFISH_LOG_INFO("Window close requested");
            raise(SIGINT);
        });

        m_webContainer->RegisterOnShowSoftwareKeyboardIfPossibleHandler(
            [this](WebContainer*) {
                if (m_ic) {
                    XSetICFocus(m_ic);
                }
            });

        m_webContainer->RegisterOnHideSoftwareKeyboardIfPossibleHandler(
            [this](WebContainer* t) {
                if (m_ic) {
                    XUnsetICFocus(m_ic);
                }
            });
    }

    ~WebViewX11()
    {
        if (m_webContainer) {
            m_webContainer->Destroy();
            m_webContainer = nullptr;
        }

        cleanupEGL();

        if (m_ic) {
            XDestroyIC(m_ic);
            m_ic = nullptr;
        }
        if (m_im) {
            XCloseIM(m_im);
            m_im = nullptr;
        }

        if (m_x11FdSource) {
            g_source_remove(m_x11FdSource);
            m_x11FdSource = 0;
        }

        if (m_ownsWindow && m_window && m_display) {
            XDestroyWindow(m_display, m_window);
            XCloseDisplay(m_display);
        }
    }

    virtual void Destroy() override
    {
        delete this;
    }

    void pollEvent()
    {
        if (!m_display)
            return;

        while (XPending(m_display)) {
            XEvent event;
            XNextEvent(m_display, &event);

            if (XFilterEvent(&event, None)) {
                continue;
            }

            switch (event.type) {
            case ConfigureNotify: {
                XWindowAttributes attr;
                XGetWindowAttributes(m_display, m_window, &attr);

                if (attr.width == m_lastWidth && attr.height == m_lastHeight) {
                    break; // Skip if size hasn't changed
                }

                m_lastWidth = attr.width;
                m_lastHeight = attr.height;

                STARFISH_LOG_INFO("Resize event: %dx%d", attr.width,
                                  attr.height);

                // Destroy old EGL surface and create new one with new size
                if (m_eglSurface != EGL_NO_SURFACE) {
                    eglDestroySurface(m_eglDisplay, m_eglSurface);
                    m_eglSurface = EGL_NO_SURFACE;
                }

                // Create new EGL surface with new window size
                if (!createEGLSurface(m_eglSurface, m_eglDisplay, m_eglConfig,
                                      m_window)) {
                    STARFISH_LOG_ERROR(
                        "Failed to create new EGL surface after resize");
                    break;
                }

                // Make context current with new surface
                if (!eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface,
                                    m_eglContext)) {
                    STARFISH_LOG_ERROR(
                        "Failed to make context current after resize");
                    break;
                }

                if (m_resizeCallback) {
                    m_resizeCallback(attr.width, attr.height);
                }

                break;
            }

            case MotionNotify:
                if (m_mouseMoveCallback) {
                    m_mouseMoveCallback(event.xmotion.x, event.xmotion.y);
                }
                break;

            case ButtonPress:
            case ButtonRelease:
                if (m_mouseButtonCallback) {
                    if (event.xbutton.button == Button1) {
                        m_mouseButtonCallback(event.type == ButtonPress ? 1 : 0,
                                              event.xbutton.x, event.xbutton.y);
                    } else if (event.xbutton.button == Button4 ||
                               event.xbutton.button == Button5) {
                        if (m_mouseWheelCallback) {
                            m_mouseWheelCallback(
                                event.xbutton.button == Button4 ? -1 : 1,
                                event.xbutton.x, event.xbutton.y);
                        }
                    }
                }
                break;

            case KeyPress:
            case KeyRelease:
                if (m_keyCallback) {
                    char buf[128] = { 0 };
                    KeySym keysym;
                    Status status;
                    bool handled = false;

                    // Helper lambda to convert X11 KeySym to KeyValue
                    auto convertKeySymToKeyValue =
                        [](KeySym keysym) -> ::LWE::KeyValue {
                        // X11 KeySym to KeyValue mapping
                        switch (keysym) {
                        case XK_BackSpace:
                            return ::LWE::KeyValue::BackspaceKey;
                        case XK_Tab:
                            return ::LWE::KeyValue::TabKey;
                        case XK_Return:
                            return ::LWE::KeyValue::EnterKey;
                        case XK_Escape:
                            return ::LWE::KeyValue::EscapeKey;
                        case XK_Left:
                            return ::LWE::KeyValue::ArrowLeftKey;
                        case XK_Up:
                            return ::LWE::KeyValue::ArrowUpKey;
                        case XK_Right:
                            return ::LWE::KeyValue::ArrowRightKey;
                        case XK_Down:
                            return ::LWE::KeyValue::ArrowDownKey;
                        case XK_Delete:
                            return ::LWE::KeyValue::DeleteKey;
                        case XK_Insert:
                            return ::LWE::KeyValue::InsertKey;
                        case XK_Home:
                            return ::LWE::KeyValue::HomeKey;
                        case XK_End:
                            return ::LWE::KeyValue::EndKey;
                        case XK_Page_Up:
                            return ::LWE::KeyValue::PageUpKey;
                        case XK_Page_Down:
                            return ::LWE::KeyValue::PageDownKey;
                        case XK_Shift_L:
                        case XK_Shift_R:
                            return ::LWE::KeyValue::ShiftLeftKey;
                        case XK_Control_L:
                        case XK_Control_R:
                            return ::LWE::KeyValue::ControlLeftKey;
                        case XK_Alt_L:
                        case XK_Alt_R:
                            return ::LWE::KeyValue::AltLeftKey;
                        case XK_F1:
                            return ::LWE::KeyValue::F1Key;
                        case XK_F2:
                            return ::LWE::KeyValue::F2Key;
                        case XK_F3:
                            return ::LWE::KeyValue::F3Key;
                        case XK_F4:
                            return ::LWE::KeyValue::F4Key;
                        case XK_F5:
                            return ::LWE::KeyValue::F5Key;
                        case XK_F6:
                            return ::LWE::KeyValue::F6Key;
                        case XK_F7:
                            return ::LWE::KeyValue::F7Key;
                        case XK_F8:
                            return ::LWE::KeyValue::F8Key;
                        case XK_F9:
                            return ::LWE::KeyValue::F9Key;
                        case XK_F10:
                            return ::LWE::KeyValue::F10Key;
                        case XK_F11:
                            return ::LWE::KeyValue::F11Key;
                        case XK_F12:
                            return ::LWE::KeyValue::F12Key;
                        default:
                            // Handle arrow keys and other special keys in
                            // 0xFF50-0xFFFF range
                            if (static_cast<KeySym>(0xFF50) <= keysym &&
                                keysym <= static_cast<KeySym>(0xFFFF)) {
                                if (keysym == XK_Left)
                                    return ::LWE::KeyValue::ArrowLeftKey;
                                if (keysym == XK_Up)
                                    return ::LWE::KeyValue::ArrowUpKey;
                                if (keysym == XK_Right)
                                    return ::LWE::KeyValue::ArrowRightKey;
                                if (keysym == XK_Down)
                                    return ::LWE::KeyValue::ArrowDownKey;
                            }
                            // For printable characters, use corresponding
                            // KeyValue
                            if (keysym >= 32 && keysym <= 126) {
                                return static_cast<::LWE::KeyValue>(keysym);
                            }
                            return ::LWE::KeyValue::UnidentifiedKey;
                        }
                    };

                    if (m_ic) {
                        int len =
                            XmbLookupString(m_ic, &event.xkey, buf,
                                            sizeof(buf) - 1, &keysym, &status);

                        if (status == XLookupChars || status == XLookupBoth) {
                            if (len > 0) {
                                buf[len] = '\0';
                                if (len == 1 &&
                                    isprint(
                                        static_cast<unsigned char>(buf[0]))) {
                                    m_keyCallback(
                                        static_cast<unsigned char>(buf[0]),
                                        event.type == KeyPress ? 1 : 0);
                                    handled = true;
                                } else {
                                    // NOTE there is no compositing event in XIM
                                    std::string committedStr(buf, len);
                                    m_webContainer->DispatchCompositionEndEvent(
                                        committedStr);
                                    handled = true;
                                }
                            }
                        }
                    } else {
                        char keychar;
                        int result = XLookupString(&event.xkey, &keychar, 1,
                                                   &keysym, nullptr);
                        if (result > 0 &&
                            isprint(static_cast<unsigned char>(keychar))) {
                            m_keyCallback(keychar,
                                          event.type == KeyPress ? 1 : 0);
                            handled = true;
                        } else {
                            // No printable character, use keysym conversion
                            int keyValue = convertKeySymToKeyValue(keysym);
                            if (keyValue != 0) {
                                m_keyCallback(keyValue,
                                              event.type == KeyPress ? 1 : 0);
                                handled = true;
                            }
                        }
                    }
                }
                break;

            case ClientMessage: {
                if (event.xclient.data.l[0] ==
                    static_cast<long>(m_wmDeleteWindow)) {
                    if (m_closeCallback) {
                        m_closeCallback();
                    }
                }
                break;
            }

            default:
                break;
            }
        }
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

    void registerX11Fd()
    {
        if (!m_display)
            return;

        int fd = ConnectionNumber(m_display);
        GIOChannel* channel = g_io_channel_unix_new(fd);

        m_x11FdSource = g_io_add_watch_full(
            channel, G_PRIORITY_DEFAULT,
            (GIOCondition)(G_IO_IN | G_IO_HUP | G_IO_ERR),
            [](GIOChannel* source, GIOCondition condition,
               gpointer data) -> gboolean {
                WebViewX11* self = static_cast<WebViewX11*>(data);

                if (condition & (G_IO_HUP | G_IO_ERR)) {
                    return G_SOURCE_CONTINUE;
                }

                self->pollEvent();

                return G_SOURCE_CONTINUE;
            },
            this,
            [](gpointer data) {
                GIOChannel* channel = static_cast<GIOChannel*>(data);
                g_io_channel_unref(channel);
            });

        g_io_channel_unref(channel);
    }

    Display* getDisplay()
    {
        return m_display;
    }
    Window getX11Window()
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

private:
    bool m_ownsWindow;
    Display* m_display;
    unsigned long m_window;
    EGLDisplay m_eglDisplay;
    EGLSurface m_eglSurface;
    EGLContext m_eglContext;
    EGLConfig m_eglConfig;
    Atom m_wmDeleteWindow;
    XIM m_im;
    XIC m_ic;
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
    guint m_x11FdSource;
};

WebView* WebView::Create(void* win, unsigned x, unsigned y, unsigned width,
                         unsigned height, float devicePixelRatio,
                         const char* defaultFontName, const char* locale,
                         const char* timezoneID)
{
    return new WebViewX11(win, x, y, width, height, devicePixelRatio,
                          defaultFontName, locale, timezoneID);
}

} // namespace LWEDelegate

#endif
