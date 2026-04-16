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

#include <cstdio>
#include <unistd.h>
#include <signal.h>
#include <sys/select.h>
#include <cerrno>

namespace StarfishShell {

Console::Console(MiniBrowser* browser)
    : m_browser(browser)
    , m_thread(0)
{
}

Console::~Console()
{
    stop();
    if (m_thread) {
        pthread_join(m_thread, NULL);
    }
}

void Console::stop()
{
    m_running = false;
}

void Console::run()
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_create(
        &m_thread, &attr,
        [](void* data) -> void* {
            char buf[1024];
            Console* console = reinterpret_cast<Console*>(data);
            sleep(1);
            while (console->m_running) {
                fd_set readfds;
                struct timeval tv;

                FD_ZERO(&readfds);
                FD_SET(STDIN_FILENO, &readfds);

                tv.tv_sec = 0;
                tv.tv_usec = 100000;

                int ret = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv);

                if (ret < 0) {
                    if (errno == EINTR)
                        continue;
                    break;
                }

                if (ret == 0) {
                    continue;
                }

                if (FD_ISSET(STDIN_FILENO, &readfds)) {
                    if (!std::fgets(buf, sizeof(buf), stdin)) {
                        continue;
                    }

                    Param* param = new Param;
                    param->console = console;
                    param->input = std::string(buf);

                    console->send(param);
                }
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
        } else if (command.find("rotate", 0) == 0) { // ex) \rotate 90
            int degrees = std::atoi(command.c_str() + 7);
            if (degrees == 0 || degrees == 90 || degrees == 180 ||
                degrees == 270) {
                m_browser->setRotate(degrees);
            }
        }
        return;
    }
    puts(m_browser->evaluateJavaScript(input).c_str());
}

} // namespace StarfishShell
