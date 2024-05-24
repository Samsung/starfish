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

#if defined(STARFISH_SHELL_X11)

#include "Window.h"

#include <X11/Xutil.h>
#include <EGL/egl.h>

#include <memory>
#include <cstring>

using XWindow = Window;

namespace {

bool createSimpleWindow(Display* display, XWindow& window, int width,
                        int height)
{
    window = XCreateSimpleWindow(display, DefaultRootWindow(display), 0, 0,
                                 width, height, 0, 0, WhitePixel(display, 0));

    const long eventMask = StructureNotifyMask | ButtonPressMask |
                           PointerMotionMask | ButtonReleaseMask |
                           KeyPressMask | KeyReleaseMask;

    XSelectInput(display, window, eventMask);

    XSetWindowAttributes attributes = {};
    attributes.event_mask = eventMask;
    XChangeWindowAttributes(display, window, CWEventMask, &attributes);

    return true;
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
            16,
            EGL_STENCIL_SIZE,
            8,
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

    bool initialize(XWindow window);
    void deinitialize();

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
};

bool RendererDelegateEGL::initialize(XWindow window)
{
    if (!createEGLDisplay(m_eglDisplay, m_eglConfig) ||
        !createEGLSurface(m_eglSurface, m_eglDisplay, m_eglConfig, window) ||
        !createGLContext(m_eglContext, m_eglDisplay, m_eglConfig, nullptr)) {
        return false;
    }
    return true;
}

void RendererDelegateEGL::deinitialize()
{
    eglDestroySurface(m_eglDisplay, m_eglSurface);
    eglDestroyContext(m_eglDisplay, m_eglContext);
    eglTerminate(m_eglDisplay);
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

class WindowX11 final : public Window {
public:
    WindowX11();
    ~WindowX11();

    bool init(const char* appName, int width, int height) override;
    void pollEvent() override;
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
    Display* m_display = nullptr;
    XWindow m_window = 0;
    Atom m_wmDeleteWindow = 0;
    std::unique_ptr<RendererDelegateEGL> m_renderer;
};

WindowX11::WindowX11()
{
}

WindowX11::~WindowX11()
{
}

bool WindowX11::init(const char* appName, int width, int height)
{
    Display* display = nullptr;
    XWindow window;
    Atom wmDeleteWindow;

    display = XOpenDisplay(nullptr);

    if (display == nullptr) {
        printf("Cannot open display\n");
        return false;
    }

    createSimpleWindow(display, window, width, height);

    if (m_isVisible) {
        XMapWindow(display, window);
    } else {
        XUnmapWindow(display, window);
    }

    XStoreName(display, window, appName);

    // Set window manager protocols to handle window deletion events
    wmDeleteWindow = XInternAtom(display, "WM_DELETE_WINDOW", false);
    XSetWMProtocols(display, window, &wmDeleteWindow, 1);

    XMoveWindow(display, window, 0, 0);
    XFlush(display);

    m_display = display;
    m_window = window;
    m_wmDeleteWindow = wmDeleteWindow;

    m_renderer =
        std::unique_ptr<RendererDelegateEGL>(new RendererDelegateEGL());
    if (!m_renderer->initialize(m_window)) {
        return false;
    }

    return true;
}

void WindowX11::getCursorPos(double& xpos, double& ypos)
{
    Display* display = m_display;
    XEvent event;
    XQueryPointer(display, m_window, &event.xbutton.root, &event.xbutton.window,
                  &event.xbutton.x_root, &event.xbutton.y_root,
                  &event.xbutton.x, &event.xbutton.y, &event.xbutton.state);
    xpos = event.xbutton.x;
    ypos = event.xbutton.y;
}

void WindowX11::pollEvent()
{
    Display* display = m_display;

    if (!XPending(display)) {
        return;
    }

    while (QLength(display)) {
        XEvent event;
        XNextEvent(display, &event);

        switch (event.type) {
        case ConfigureNotify:
            if (m_windowSizeEventHandler) {
                XWindowAttributes attr;
                XGetWindowAttributes(display, m_window, &attr);
                m_windowSizeEventHandler(attr.width, attr.height);
            }
            break;

        case MotionNotify:
            if (m_motionEventHandler) {
                m_motionEventHandler(event.xmotion.x, event.xmotion.y);
            }
            break;

        case ButtonPress:
        case ButtonRelease:
            if (m_buttonEventHandler) {
                if (event.xbutton.button == Button1) {
                    m_buttonEventHandler(INPUT::MOUSE_LBUTTON,
                                         event.type == ButtonPress
                                             ? INPUT::PRESS
                                             : INPUT::RELEASE);
                }
                // As an alternative for wheel events, we may use XInput2
                // extension.
                else if (event.xbutton.button == Button4 ||
                         event.xbutton.button == Button5) {
                    if (m_scrollEventHandler) {
                        double xpos, ypos;
                        getCursorPos(xpos, ypos);
                        m_scrollEventHandler(
                            xpos, ypos,
                            event.xbutton.button == Button4 ? -1 : 1);
                    }
                }
            }
            break;

        case KeyPress:
        case KeyRelease:
            if (m_keyEventHandler) {
                char keychar;
                KeySym keysym;
                INPUT type =
                    event.type == KeyPress ? INPUT::PRESS : INPUT::RELEASE;
                // Convert the system keycodes to the ascii keycodes if exists.
                if (XLookupString(&event.xkey, &keychar, 1, &keysym, nullptr)) {
                    m_keyEventHandler(keychar, type, event.xkey.state);
                    return;
                } else {
                    if (static_cast<KeySym>(INPUT::LEFT) <= keysym &&
                        keysym < static_cast<KeySym>(INPUT::CODE_END)) {
                        m_keyEventHandler(keysym, type, event.xkey.state);
                        return;
                    }
                }
                printf("UNIMPLEMENTED\n");
            }
            break;

        case ClientMessage: {
            if (event.xclient.data.l[0] ==
                static_cast<long>(m_wmDeleteWindow)) {
                if (m_exitEventHandler) {
                    m_exitEventHandler();
                }
            }
        } break;

        default:
            break;
        }
    }
}

void WindowX11::terminate()
{
    m_renderer->deinitialize();
    m_renderer = nullptr;

    XDestroyWindow(m_display, m_window);
    XCloseDisplay(m_display);

    m_window = 0;
    m_display = nullptr;
}

Window* Window::create()
{
    return new WindowX11();
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
