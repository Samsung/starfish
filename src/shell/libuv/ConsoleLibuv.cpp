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

#if defined(STARFISH_SHELL_X11) && defined(STARFISH_UV_CAIRO_GL)
#include "AppLoop.h"
#include "Console.h"
#include "MiniBrowser.h"

namespace StarfishShell {

class ConsoleLibuv : public Console {
public:
    ConsoleLibuv(MiniBrowser* browser)
        : Console(browser)
    {
    }

    // Called on the stdin reader thread. write() drives the browser, so it
    // has to run on the shell's own thread like every other console backend
    // does -- not on LWE's loop thread, which the shell doesn't own.
    void send(Param* param) override
    {
        m_browser->appLoop()->postTask([param]() {
            param->console->write(param->input);
            delete param;
        });
    }
};

Console* Console::create(MiniBrowser* browser)
{
    return new ConsoleLibuv(browser);
}

} // namespace StarfishShell

#endif
