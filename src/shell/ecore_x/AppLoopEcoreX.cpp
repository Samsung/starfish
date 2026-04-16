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

#if defined(STARFISH_SHELL_ECORE_X)

#include "AppLoop.h"

#include <Ecore.h>
#include <glib.h>

namespace StarfishShell {

class AppLoopEcoreX : public AppLoop {
public:
    AppLoopEcoreX();
    ~AppLoopEcoreX();

    virtual void init() override;
    virtual int start(double timeoutInSec = 0) override;
    virtual void stop() override;
    virtual void deinit() override;

private:
    Ecore_Timer* m_timerID = nullptr;
};

AppLoopEcoreX::AppLoopEcoreX()
{
    ecore_init();
    ecore_main_loop_glib_integrate();
}

AppLoopEcoreX::~AppLoopEcoreX()
{
    ecore_shutdown();
}

void AppLoopEcoreX::init()
{
}

int AppLoopEcoreX::start(double timeoutInSec)
{
    if (timeoutInSec) {
        m_timerID = ecore_timer_add(
            timeoutInSec,
            [](void* data) -> Eina_Bool {
                AppLoopEcoreX* self = static_cast<AppLoopEcoreX*>(data);
                self->stop();
                self->m_timerID = nullptr;
                return ECORE_CALLBACK_DONE;
            },
            this);
    }
    ecore_main_loop_begin();
    return 0;
}

void AppLoopEcoreX::stop()
{
    if (m_timerID) {
        ecore_timer_freeze(m_timerID);
        ecore_timer_del(m_timerID);
        m_timerID = nullptr;
    }
    ecore_main_loop_quit();
}

void AppLoopEcoreX::deinit()
{
}

std::unique_ptr<AppLoop> AppLoop::create()
{
    return std::unique_ptr<AppLoopEcoreX>(new AppLoopEcoreX());
}

} // namespace StarfishShell

#endif
