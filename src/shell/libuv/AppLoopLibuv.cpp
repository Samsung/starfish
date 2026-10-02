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

#include <signal.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <memory.h>
#include <stdio.h>
#include <cstdint>

#include <sys/time.h>

#include <deque>
#include <functional>
#include <mutex>

namespace {

volatile sig_atomic_t doneFlag = 0;

void setDoneFlag(int sig, siginfo_t* siginfo, void* context)
{
    _exit(0);
    puts("signal!!!");
    doneFlag = 1;
}

uint64_t timestamp()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000UL + tv.tv_usec / 1000UL;
}

} // namespace

namespace StarfishShell {

class AppLoopSimple : public AppLoop {
public:
    AppLoopSimple();
    ~AppLoopSimple();

    virtual void init() override;
    virtual int start(double timeoutInSec = 0) override;
    virtual void stop() override;
    virtual void deinit() override;
    virtual void postTask(std::function<void()> task) override;

private:
    void runPendingTasks();

    uint64_t m_timeoutInMs = 0;
    uint64_t m_startTimeInMs = 0;

    // With the libuv backend LWE runs the event loop on its own thread, so
    // this loop is only a wait loop and has no queue of its own. Shell code
    // that needs to run on this thread (the console's stdin reader, for
    // one) posts here instead of reaching into LWE's uv loop, whose handle
    // API is not thread-safe.
    std::mutex m_tasksMutex;
    std::deque<std::function<void()>> m_tasks;
};

AppLoopSimple::AppLoopSimple()
{
}

AppLoopSimple::~AppLoopSimple()
{
}

void AppLoopSimple::init()
{
    // Do nothing.
}

int AppLoopSimple::start(double timeoutInSec)
{
    doneFlag = 0;
    m_timeoutInMs = 0;
    m_startTimeInMs = 0;

    if (timeoutInSec > 0) {
        m_timeoutInMs = static_cast<uint64_t>(timeoutInSec) * 1000;
        m_startTimeInMs = timestamp();
    }

    struct sigaction act;
    memset(&act, '\0', sizeof(act));
    act.sa_sigaction = setDoneFlag;
    act.sa_flags = SA_SIGINFO;

    if (sigaction(SIGINT, &act, NULL) < 0) {
        perror("sigaction");
        return 1;
    }

    while (!doneFlag) {
        runPendingTasks();
        usleep(100);
        if (m_timeoutInMs > 0) {
            uint64_t current = timestamp();
            if (current - m_startTimeInMs >= m_timeoutInMs) {
                doneFlag = 1;
            }
        }
    }

    return 0;
}

void AppLoopSimple::stop()
{
    doneFlag = 1;
}

void AppLoopSimple::deinit()
{
    // Do nothing.
}

void AppLoopSimple::postTask(std::function<void()> task)
{
    std::lock_guard<std::mutex> lock(m_tasksMutex);
    m_tasks.push_back(std::move(task));
}

void AppLoopSimple::runPendingTasks()
{
    std::deque<std::function<void()>> tasks;
    {
        std::lock_guard<std::mutex> lock(m_tasksMutex);
        tasks.swap(m_tasks);
    }
    for (auto& task : tasks) {
        task();
    }
}

std::unique_ptr<AppLoop> AppLoop::create()
{
    return std::unique_ptr<AppLoopSimple>(new AppLoopSimple());
}

} // namespace StarfishShell

#endif
