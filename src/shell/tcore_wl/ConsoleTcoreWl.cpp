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

#if defined(STARFISH_SHELL_TCORE_WL)
#include "Console.h"

#include <tizen_core.h>

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

namespace StarfishShell {

class ConsoleTcoreWl : public Console {
public:
    ConsoleTcoreWl(MiniBrowser* browser)
        : Console(browser)
    {
    }

    ~ConsoleTcoreWl()
    {
    }

    void send(Param* param)
    {
        tizen_core_h core = nullptr;
        // TODO delete source
        tizen_core_source_h source = nullptr;
        tizen_core_find("main", &core);
        tizen_core_add_idle_job(
            core,
            [](void* data) -> bool {
                Param* p = reinterpret_cast<Param*>(data);
                p->console->write(p->input);
                delete p;
                return false; // one-shot idler
            },
            param, &source);
    }
};

Console* Console::create(MiniBrowser* browser)
{
    return new ConsoleTcoreWl(browser);
}

} // namespace StarfishShell

#endif
