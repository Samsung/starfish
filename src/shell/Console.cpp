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

#include "Console.h"
#include "MiniBrowser.h"

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

namespace StarfishShell {

Console::Console(MiniBrowser* browser)
    : m_browser(browser)
{
}

void Console::run()
{
    pthread_t t;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_create(
        &t, &attr,
        [](void* data) -> void* {
            char buf[1024];
            Console* console = reinterpret_cast<Console*>(data);
            sleep(1);
            while (1) {
                // Poll input
                fgets(buf, 1024, stdin);

                Param* param = new Param;
                param->console = console;
                param->input = std::string(buf);

                console->send(param);
            }
            return NULL;
        },
        this);
}

void Console::write(const std::string& input)
{
    static const std::string prefix = "\\";
    std::string command =
        input.substr(0, input.find_last_not_of(" \t\n\v\f\r") + 1);
    if (command.find(prefix, 0) == 0) {
        command.erase(0, prefix.size());
        if (command == "reload") {
            m_browser->reload();
        }
        return;
    }
    puts(m_browser->evaluateJavaScript(input).c_str());
}

} // namespace StarfishShell
