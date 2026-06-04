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

#ifndef __StarfishShellWindowBase__
#define __StarfishShellWindowBase__

#include <string>
#include <cstdint>
#include <functional>

#include "WindowKeyType.h"
#include "PlatformIntegrationData.h"
#include "RendererDelegate.h"
#include "AppLoop.h"

namespace StarfishShell {

#define HINT_VISIBLE 0x0001

class Window {
public:
    using MotionEventHandler = std::function<void(int xpos, int ypos)>;
    using WindowSizeEventHandler = std::function<void(int width, int height)>;
    using ScrollEventHandler =
        std::function<void(double x, double y, int delta)>;
    using ButtonEventHandler = std::function<void(INPUT type, INPUT action)>;
    using ExitEventHandler = std::function<void()>;
    using KeyEventHandler =
        std::function<void(unsigned long code, INPUT action, unsigned mods)>;
    using FocusInHandler = std::function<void()>;
    using CompositionEventHandler =
        std::function<void(const char* text, bool isEnd)>;

    static Window* create();
    static LWE::KeyValue convertKeyCode(const unsigned long key, INPUT action,
                                        unsigned mods);

    virtual ~Window()
    {
        m_appLoop->deinit();
        m_appLoop = nullptr;
    }

    void setMotionEventHandler(const MotionEventHandler& handler)
    {
        m_motionEventHandler = handler;
    }

    void setButtonEventHandler(const ButtonEventHandler& handler)
    {
        m_buttonEventHandler = handler;
    }

    void setWindowSizeEventHandler(const WindowSizeEventHandler& handler)
    {
        m_windowSizeEventHandler = handler;
    }

    void setKeyEventHandler(const KeyEventHandler& handler)
    {
        m_keyEventHandler = handler;
    }

    void setScrollEventHandler(const ScrollEventHandler& handler)
    {
        m_scrollEventHandler = handler;
    }

    void setExitEventHandler(const ExitEventHandler& handler)
    {
        m_exitEventHandler = handler;
    }

    void setFocusInHandler(const FocusInHandler& handler)
    {
        m_focusInHandler = handler;
    }

    void setCompositionEventHandler(const CompositionEventHandler& handler)
    {
        m_compositionEventHandler = handler;
    }

    void setInitHint(int hint, int value)
    {
        if (hint == HINT_VISIBLE) {
            m_isVisible = value;
        }
    }

    virtual bool init(const char* applicationName, int width, int height) = 0;
    virtual void pollEvent(){};
    virtual void terminate() = 0;

#if defined(STARFISH_SHELL_EFL)
    virtual void addAutoFitChild(void* child)
    {
    }
#endif

    virtual void* getNativeWindowHandle() = 0;
    virtual void getCursorPos(double& xpos, double& ypos) = 0;

    virtual RendererDelegate* renderer() = 0;

    virtual void setRotate(int degree){};

    virtual void ShowSoftwareKeyboardIfPossible()
    {
    }
    virtual void HideSoftwareKeyboardIfPossible()
    {
    }

    AppLoop* appLoop()
    {
        return m_appLoop.get();
    }

protected:
    Window()
    {
        m_appLoop = AppLoop::create();
    }

    MotionEventHandler m_motionEventHandler;
    ButtonEventHandler m_buttonEventHandler;
    WindowSizeEventHandler m_windowSizeEventHandler;
    KeyEventHandler m_keyEventHandler;
    ScrollEventHandler m_scrollEventHandler;
    ExitEventHandler m_exitEventHandler;
    FocusInHandler m_focusInHandler;
    CompositionEventHandler m_compositionEventHandler;

    std::unique_ptr<AppLoop> m_appLoop;
    int m_isVisible = 1;
};
} // namespace StarfishShell

#endif
