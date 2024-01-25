/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
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

#include "public/bridge/x11/XWindow.h"

#if defined(PORT_WEBVIEW_BRIDGE_X11)

#include <X11/Xutil.h>

#define NO_EXPOSE_GC
#include "StarfishBase.h"

namespace LWE {

XWindow::XWindow()
{
    m_internal = std::make_shared<Internal>();
}

struct XWindow::Internal {
    Display* display{ nullptr };
    NativeWindowType window{ 0 };
    Atom wmDeleteWindow{ 0 };
    int isVisible{ 1 };
};

static bool createSimpleWindow(Display* display, Window& window, int width,
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

bool XWindow::init(const char* appName, int width, int height)
{
    Display* display = nullptr;
    NativeWindowType window;
    Atom wmDeleteWindow;

    display = XOpenDisplay(nullptr);

    if (display == nullptr) {
        STARFISH_LOG_ERROR("Cannot open display");
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    createSimpleWindow(display, window, width, height);

    if (m_internal->isVisible) {
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

    m_internal->display = display;
    m_internal->window = window;
    m_internal->wmDeleteWindow = wmDeleteWindow;

    return true;
}

void XWindow::setInitHint(int hint, int value)
{
    if (hint == HINT_VISIBLE) {
        m_internal->isVisible = value;
    }
}

void XWindow::getCursorPos(double& xpos, double& ypos)
{
    Display* display = m_internal->display;
    NativeWindowType window = m_internal->window;
    XEvent event;
    XQueryPointer(display, window, &event.xbutton.root, &event.xbutton.window,
                  &event.xbutton.x_root, &event.xbutton.y_root,
                  &event.xbutton.x, &event.xbutton.y, &event.xbutton.state);
    xpos = event.xbutton.x;
    ypos = event.xbutton.y;
}

void XWindow::pollEvent()
{
    Display* display = m_internal->display;
    NativeWindowType window = m_internal->window;

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
                XGetWindowAttributes(display, window, &attr);
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
                STARFISH_UNIMPLEMENTED();
            }
            break;

        case ClientMessage: {
            if (event.xclient.data.l[0] ==
                static_cast<long>(m_internal->wmDeleteWindow)) {
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

void XWindow::terminate()
{
    XDestroyWindow(m_internal->display, m_internal->window);
    XCloseDisplay(m_internal->display);
    m_internal->window = 0;
    m_internal->display = nullptr;
}

NativeWindowType XWindow::getNativeWindowHandle()
{
    return m_internal->window;
}

} // namespace LWE

#endif
