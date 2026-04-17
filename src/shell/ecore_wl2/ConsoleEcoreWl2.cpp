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

#if defined(STARFISH_SHELL_ECORE_WL2)
#include "Console.h"

#include <Ecore.h>

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

namespace StarfishShell {

class ConsoleEcoreWl2 : public Console {
public:
    ConsoleEcoreWl2(MiniBrowser* browser)
        : Console(browser)
    {
    }

    ~ConsoleEcoreWl2()
    {
    }

    void send(Param* param)
    {
        ecore_thread_main_loop_begin();
        ecore_animator_add(
            [](void* data) -> Eina_Bool {
                Param* p = reinterpret_cast<Param*>(data);
                p->console->write(p->input);
                delete p;
                return ECORE_CALLBACK_CANCEL;
            },
            param);
        ecore_thread_main_loop_end();
    }
};

Console* Console::create(MiniBrowser* browser)
{
    return new ConsoleEcoreWl2(browser);
}

} // namespace StarfishShell

#endif