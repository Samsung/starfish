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

#if defined(STARFISH_SHELL_EFL_HEADLESS)

#include "AppLoop.h"
#include <Ecore.h>

namespace StarfishShell {

class AppLoopEFLHeadless : public AppLoop {
public:
    AppLoopEFLHeadless();
    ~AppLoopEFLHeadless();

    virtual void init() override;
    virtual int start(double timeoutInSec = 0) override;
    virtual void stop() override;
    virtual void deinit() override;

private:
    Ecore_Timer* m_timerID = nullptr;
};

AppLoopEFLHeadless::AppLoopEFLHeadless()
{
}

AppLoopEFLHeadless::~AppLoopEFLHeadless()
{
}

void AppLoopEFLHeadless::init()
{
    ecore_init();
}

int AppLoopEFLHeadless::start(double timeoutInSec)
{
    if (timeoutInSec) {
        m_timerID = ecore_timer_add(
            timeoutInSec,
            [](void* data) -> Eina_Bool {
                AppLoopEFLHeadless* self =
                    static_cast<AppLoopEFLHeadless*>(data);
                self->stop();
                self->m_timerID = nullptr;
                return ECORE_CALLBACK_DONE;
            },
            this);
    }
    ecore_main_loop_begin();
    return 0;
}

void AppLoopEFLHeadless::stop()
{
    if (m_timerID) {
        ecore_timer_freeze(m_timerID);
        ecore_timer_del(m_timerID);
    }
    ecore_main_loop_quit();
}

void AppLoopEFLHeadless::deinit()
{
    ecore_shutdown();
}

std::unique_ptr<AppLoop> AppLoop::create()
{
    return std::unique_ptr<AppLoopEFLHeadless>(new AppLoopEFLHeadless());
}

} // namespace StarfishShell

#endif
