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

#include "ShellConfig.h"

#if (defined(STARFISH_SHELL_X11) || defined(STARFISH_SHELL_GLFW) || \
     defined(STARFISH_SHELL_GLIB_HEADLESS)) &&                      \
    (defined(STARFISH_GLIB_CAIRO_GL) || defined(STARFISH_GLIB_HEADLESS))
#include "Console.h"

#include <glib.h>

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

namespace StarfishShell {

class ConsoleGlibX : public Console {
public:
    ConsoleGlibX(MiniBrowser* browser)
        : Console(browser)
    {
    }

    ~ConsoleGlibX()
    {
    }

    void send(Param* param)
    {
        g_idle_add(
            [](gpointer data) -> gboolean {
                Param* p = reinterpret_cast<Param*>(data);
                p->console->write(p->input);
                delete p;
                return G_SOURCE_REMOVE;
            },
            param);
    }
};

Console* Console::create(MiniBrowser* browser)
{
    return new ConsoleGlibX(browser);
}

} // namespace StarfishShell

#endif
