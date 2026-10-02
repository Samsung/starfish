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

#ifndef __StarfishShellAppLoop__
#define __StarfishShellAppLoop__

#include <cstdio>
#include <cstdlib>
#include <functional>
#include <memory>

namespace StarfishShell {

class AppLoop {
public:
    static std::unique_ptr<AppLoop> create();

    virtual ~AppLoop() = default;
    virtual void init() = 0;
    virtual int start(double timeoutInSec = 0) = 0;
    virtual void stop() = 0;
    virtual void deinit() = 0;

    // Runs `task` on the thread that is inside start(). Callable from any
    // thread. Loops built on a toolkit that already has a thread-safe way
    // to reach its main loop (g_idle_add, ecore_thread_main_loop_begin, ...)
    // don't need this yet; the default aborts rather than silently dropping
    // the task so a missing override is caught the first time it's used.
    virtual void postTask(std::function<void()> task)
    {
        fprintf(stderr, "AppLoop::postTask is not supported by this loop\n");
        abort();
    }

protected:
    AppLoop() = default;
};

} // namespace StarfishShell

#endif
