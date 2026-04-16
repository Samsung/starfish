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

#ifndef __StarfishShellConsole__
#define __StarfishShellConsole__

#include <string>
#include <pthread.h>
#include <atomic>

namespace StarfishShell {

class MiniBrowser;

class Console {
public:
    struct Param {
        Console* console;
        std::string input;
    };

    static Console* create(MiniBrowser* browser);

    virtual ~Console();
    void run();
    virtual void send(Param* param) = 0;
    void write(const std::string& input);
    void stop();

protected:
    Console(MiniBrowser* browser);

    MiniBrowser* m_browser;
    pthread_t m_thread;
    std::atomic<bool> m_running{ true };
};

} // namespace StarfishShell

#endif
