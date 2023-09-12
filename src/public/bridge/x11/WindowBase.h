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

#ifndef __StarfishWindowBase__
#define __StarfishWindowBase__

#include "public/bridge/x11/WindowType.h"
#include "public/bridge/x11/WindowKeyType.h"
#include <functional>

namespace LWE {

#define HINT_VISIBLE 0x0001

class WindowBase {
public:
    using MotionEventHandler = std::function<void(int xpos, int ypos)>;
    using WindowSizeEventHandler = std::function<void(int width, int height)>;
    using ScrollEventHandler = std::function<void(double xmove, double ymove)>;
    using ButtonEventHandler = std::function<void(INPUT type, INPUT action)>;
    using ExitEventHandler = std::function<void()>;
    using KeyEventHandler =
        std::function<void(unsigned long code, INPUT action, unsigned mods)>;

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

    virtual bool init(const char* applicationName, int width, int height) = 0;
    virtual void pollEvent() = 0;
    virtual void terminate() = 0;
    virtual NativeWindowType getNativeWindowHandle() = 0;
    virtual void getCursorPos(double& xpos, double& ypos) = 0;
    virtual void setInitHint(int hint, int value) = 0;

protected:
    MotionEventHandler m_motionEventHandler;
    ButtonEventHandler m_buttonEventHandler;
    WindowSizeEventHandler m_windowSizeEventHandler;
    KeyEventHandler m_keyEventHandler;
    ScrollEventHandler m_scrollEventHandler;
    ExitEventHandler m_exitEventHandler;
};

} // namespace LWE

#endif
