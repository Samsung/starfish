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

#include "StarfishConfig.h"
#include "PlatformIntegrationData.h"
#include "public/delegate/LWEWebViewDelegateImpl.h"
#include "public/delegate/LWEWebContainerDelegate.h"
#include "public/bridge/x11/XWindow.h"
#include "public/bridge/x11/WindowEGL.h"
#include "platform/canvas/webgl/XGL.h"
#include "platform/canvas/webgl/XGLUtil.h"

#if defined(PORT_WEBVIEW_BRIDGE_X11)

using namespace Starfish;

namespace LWEDelegate {

using namespace LWE;

class WebViewX11 : public WebViewImpl {
public:
    WebViewX11(void* winArg, unsigned x, unsigned y, unsigned width,
               unsigned height, float devicePixelRatio,
               const char* defaultFontName, const char* locale,
               const char* timezoneID)
    {
        m_window = std::make_shared<XWindow>();

#if defined(STARFISH_ENABLE_TEST)
        if (getenv("SCREEN_SHOT") || getenv("HIDE_WINDOW")) {
            m_window->setInitHint(HINT_VISIBLE, 0);
        }
#endif

        if (!CreateWindow(width, height)) {
            STARFISH_LOG_ERROR("Creating a Window failed.");
            exit(1);
        }

        if (!initEGL(m_window->getNativeWindowHandle(), &m_glPlatform)) {
            STARFISH_LOG_ERROR("Initializing EGL failed.");
            exit(1);
        }

#if defined(STARFISH_ENABLE_TEST)
        // For screenshot
        XGLUtil::makeCurrentXGLContext(m_glPlatform.context);
        eglSwapInterval(m_glPlatform.egl.display, 0);
        XGLUtil::resetCurrentXGLContext();
#endif

        WebContainer* webContainer = WebContainer::CreateGL(
            width, height,
            [this](WebContainer* wc) {
                XGLUtil::makeCurrentXGLContext(m_glPlatform.context);
            },
            [this](WebContainer* wc, bool mayNeedsSync) {
                XGLUtil::swapXGLBuffer();
            },
            devicePixelRatio, defaultFontName, locale, timezoneID);

        SetWebContainer(webContainer);
        RequestPollEvent(10);
    }

    bool CreateWindow(unsigned width, unsigned height)
    {
        if (!m_window->init("Starfish", width, height)) {
            return false;
        }

        // Set event callbacks
        m_window->setWindowSizeEventHandler([this](int width, int height) {
            FetchWebContainer()->ResizeTo(width, height);
        });

        m_window->setMotionEventHandler([this](int xpos, int ypos) {
            MouseButtonsValue buttons = m_isMouseLbuttonDown
                                            ? MouseButtonsValue::LeftButtonDown
                                            : MouseButtonsValue::NoButtonDown;
            FetchWebContainer()->DispatchMouseMoveEvent(
                MouseButtonValue::NoButton, buttons, xpos, ypos);
        });

        m_window->setButtonEventHandler([this](INPUT type, INPUT action) {
            if (type == INPUT::MOUSE_LBUTTON) {
                double xpos, ypos;
                m_window->getCursorPos(xpos, ypos);
                if (action == INPUT::PRESS) {
                    m_isMouseLbuttonDown = true;
                    FetchWebContainer()->DispatchMouseDownEvent(
                        MouseButtonValue::NoButton,
                        MouseButtonsValue::LeftButtonDown, xpos, ypos);
                } else {
                    m_isMouseLbuttonDown = false;
                    FetchWebContainer()->DispatchMouseUpEvent(
                        MouseButtonValue::NoButton,
                        MouseButtonsValue::NoButtonDown, xpos, ypos);
                }
            }
        });

        m_window->setScrollEventHandler([this](double x, double y, int delta) {
            FetchWebContainer()->DispatchMouseWheelEvent(x, y, delta);
        });

        m_window->setKeyEventHandler(
            [this](unsigned long code, INPUT action, unsigned mods) {
                KeyValue keyValue = ConvertKeyCode(code, action, mods);
                if (action == INPUT::PRESS) {
                    FetchWebContainer()->DispatchKeyDownEvent(keyValue);
                    FetchWebContainer()->DispatchKeyPressEvent(keyValue);
                } else {
                    FetchWebContainer()->DispatchKeyUpEvent(keyValue);
                }
            });

        m_window->setExitEventHandler(
            []() { setenv("SHELL_DONE_FLAG", "1", 1); });

        return true;
    }

    void RequestPollEvent(unsigned timeout)
    {
        m_pollTimer = FetchWebContainer()->AddTimeout(
            [](void* data) {
                WebViewX11* self = reinterpret_cast<WebViewX11*>(data);
                self->PollEvent();
                self->RequestPollEvent(25);
            },
            this, timeout);
    }

    void PollEvent()
    {
        m_window->pollEvent();
    }

    void Destroy() override
    {
        FetchWebContainer()->Blur();
        FetchWebContainer()->ClearTimeout(m_pollTimer);
        FetchWebContainer()->Destroy();

        eglDestroySurface(m_glPlatform.egl.display, m_glPlatform.egl.surface);
        eglDestroyContext(m_glPlatform.egl.display, m_glPlatform.context);
        eglTerminate(m_glPlatform.egl.display);

        m_window->terminate();
        delete this;
    }

    KeyValue ConvertKeyCode(const unsigned long code, INPUT action,
                            unsigned mods)
    {
        // clang-format off
        switch (static_cast<ASCII>(code)) {
            case ASCII::HT:  return TabKey;
            case ASCII::BS:  return BackspaceKey;
            case ASCII::CR:  return EnterKey;
            case ASCII::ESC: return EscapeKey;
            case ASCII::DEL: return DeleteKey;
            default:
                break;
        }
        switch (static_cast<INPUT>(code)) {
            case INPUT::LEFT:  return ArrowLeftKey;
            case INPUT::UP:    return ArrowUpKey;
            case INPUT::RIGHT: return ArrowRightKey;
            case INPUT::DOWN:  return ArrowDownKey;
            default:
                break;
        }
        // clang-format on
        return static_cast<KeyValue>(code);
    }

private:
    bool m_isMouseLbuttonDown{ false };
    size_t m_pollTimer{ 0 };
    XGLPlatform m_glPlatform;
    std::shared_ptr<WindowBase> m_window;
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
