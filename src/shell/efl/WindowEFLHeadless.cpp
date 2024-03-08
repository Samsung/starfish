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

#if defined(STARFISH_SHELL_EFL_HEADLESS)

#include "Window.h"

#include <Ecore.h>

namespace StarfishShell {

class WindowEFLHeadless final : public Window {
public:
    WindowEFLHeadless()
    {
    }

    bool init(const char* appName, int width, int height) override
    {
        ecore_init();
        return true;
    }

    void terminate() override
    {
    }

    void getCursorPos(double& xpos, double& ypos)
    {
    }

    void* getNativeWindowHandle() override
    {
        return nullptr;
    }

    bool initEGL() override
    {
        return true;
    }

    bool makeCurrent() override
    {
        return true;
    }

    bool resetCurrent() override
    {
        return true;
    }

    bool swapBuffer() override
    {
        return true;
    }

private:
};

Window* Window::create()
{
    return new WindowEFLHeadless();
}

LWE::KeyValue Window::convertKeyCode(const unsigned long key, INPUT action,
                                     unsigned mods)
{
    return LWE::KeyValue::UnidentifiedKey;
}
} // namespace StarfishShell

#endif
