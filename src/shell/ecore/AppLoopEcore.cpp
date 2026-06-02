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

#if defined(STARFISH_SHELL_ECORE_X) || defined(STARFISH_SHELL_ECORE_WL2)

#include "AppLoop.h"

#include <Ecore.h>
#include <glib.h>

namespace StarfishShell {

class AppLoopEcore : public AppLoop {
public:
    AppLoopEcore();
    ~AppLoopEcore();

    virtual void init() override;
    virtual int start(double timeoutInSec = 0) override;
    virtual void stop() override;
    virtual void deinit() override;

private:
    Ecore_Timer* m_timerID = nullptr;
};

AppLoopEcore::AppLoopEcore()
{
    ecore_init();
}

AppLoopEcore::~AppLoopEcore()
{
    ecore_shutdown();
}

void AppLoopEcore::init()
{
}

int AppLoopEcore::start(double timeoutInSec)
{
    if (timeoutInSec) {
        m_timerID = ecore_timer_add(
            timeoutInSec,
            [](void* data) -> Eina_Bool {
                AppLoopEcore* self = static_cast<AppLoopEcore*>(data);
                self->stop();
                self->m_timerID = nullptr;
                return ECORE_CALLBACK_DONE;
            },
            this);
    }
    ecore_main_loop_begin();
    return 0;
}

void AppLoopEcore::stop()
{
    if (m_timerID) {
        ecore_timer_freeze(m_timerID);
        ecore_timer_del(m_timerID);
        m_timerID = nullptr;
    }
    ecore_main_loop_quit();
}

void AppLoopEcore::deinit()
{
}

std::unique_ptr<AppLoop> AppLoop::create()
{
    return std::unique_ptr<AppLoop>(new AppLoopEcore());
}

} // namespace StarfishShell

#endif
