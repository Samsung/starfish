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

#if defined(STARFISH_SHELL_GLFW) || defined(STARFISH_SHELL_X11)
#include "Console.h"

#include <uv.h>

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

namespace StarfishShell {

class ConsoleLibuv : public Console {
public:
    ConsoleLibuv(MiniBrowser* browser)
        : Console(browser)
    {
        m_idlerThreadAsyncHandle =
            static_cast<uv_async_t*>(malloc(sizeof(uv_async_t)));
        uv_async_init(uv_default_loop(), m_idlerThreadAsyncHandle,
                      [](uv_async_t* handle) {
                          Param* param = reinterpret_cast<Param*>(handle->data);
                          param->console->write(param->input);
                          delete param;
                      });
    }

    ~ConsoleLibuv()
    {
        if (m_idlerThreadAsyncHandle) {
            uv_close(reinterpret_cast<uv_handle_t*>(m_idlerThreadAsyncHandle),
                     [](uv_handle_t* handle) { free(handle); });
        }
    }

    void send(Param* param) override
    {
        m_idlerThreadAsyncHandle->data = param;
        uv_async_send(m_idlerThreadAsyncHandle);
    }

private:
    uv_async_t* m_idlerThreadAsyncHandle = nullptr;
};

Console* Console::create(MiniBrowser* browser)
{
    return new ConsoleLibuv(browser);
}

} // namespace StarfishShell

#endif
