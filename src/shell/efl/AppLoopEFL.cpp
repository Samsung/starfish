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

#if defined(STARFISH_SHELL_EFL)

#include "AppLoop.h"

#include <Elementary.h>

namespace StarfishShell {

class AppLoopEFL : public AppLoop {
public:
    AppLoopEFL();
    ~AppLoopEFL();

    virtual void init() override;
    virtual int start(double timeoutInSec = 0) override;
    virtual void stop() override;
    virtual void deinit() override;

private:
    Ecore_Timer* m_timerID = nullptr;
};

AppLoopEFL::AppLoopEFL()
{
}

AppLoopEFL::~AppLoopEFL()
{
}

void AppLoopEFL::init()
{
    elm_init(0, 0);
}

int AppLoopEFL::start(double timeoutInSec)
{
    if (timeoutInSec) {
        m_timerID = ecore_timer_add(
            timeoutInSec,
            [](void* data) -> Eina_Bool {
                AppLoopEFL* self = static_cast<AppLoopEFL*>(data);
                self->stop();
                self->m_timerID = nullptr;
                return ECORE_CALLBACK_DONE;
            },
            this);
    }
    elm_run();
    return 0;
}

void AppLoopEFL::stop()
{
    if (m_timerID) {
        ecore_timer_freeze(m_timerID);
        ecore_timer_del(m_timerID);
    }
    elm_exit();
}

void AppLoopEFL::deinit()
{
    // FIXME: Occasionally, crash occur on elm_shutdown with this error
    // Error: corrupted double-linked list
    // elm_shutdown();
}

std::unique_ptr<AppLoop> AppLoop::create()
{
    return std::unique_ptr<AppLoopEFL>(new AppLoopEFL());
}

} // namespace StarfishShell

#endif
