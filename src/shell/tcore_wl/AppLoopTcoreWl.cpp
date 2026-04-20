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

#include "AppLoop.h"

#include <tizen_core.h>
#include <tizen_core_wl.h>

#include <cstdio>

namespace StarfishShell {

class AppLoopTcoreWl : public AppLoop {
public:
    AppLoopTcoreWl();
    ~AppLoopTcoreWl();

    virtual void init() override;
    virtual int start(double timeoutInSec = 0) override;
    virtual void stop() override;
    virtual void deinit() override;

private:
    tizen_core_source_h m_timerID = nullptr;
    tizen_core_task_h m_task = nullptr;
};

AppLoopTcoreWl::AppLoopTcoreWl()
{
    tizen_core_init();
    tizen_core_task_create("main", false, &m_task);
}

AppLoopTcoreWl::~AppLoopTcoreWl()
{
    tizen_core_task_destroy(m_task);
    tizen_core_shutdown();
}

void AppLoopTcoreWl::init()
{
}

int AppLoopTcoreWl::start(double timeoutInSec)
{
    if (timeoutInSec > 0) {
        tizen_core_h core = nullptr;
        tizen_core_task_get_tizen_core(m_task, &core);
        tizen_core_add_timer(
            core, timeoutInSec * 1000,
            [](void* data) -> bool {
                AppLoopTcoreWl* self = static_cast<AppLoopTcoreWl*>(data);
                self->stop();
                return false; // one-shot timer
            },
            this, &m_timerID);
    }
    tizen_core_task_run(m_task);
    return 0;
}

void AppLoopTcoreWl::stop()
{
    if (m_timerID) {
        tizen_core_h core = nullptr;
        tizen_core_task_get_tizen_core(m_task, &core);
        tizen_core_remove_source(core, m_timerID);
        m_timerID = nullptr;
    }
    tizen_core_task_quit(m_task);
}

void AppLoopTcoreWl::deinit()
{
}

std::unique_ptr<AppLoop> AppLoop::create()
{
    return std::unique_ptr<AppLoopTcoreWl>(new AppLoopTcoreWl());
}

} // namespace StarfishShell

#endif
