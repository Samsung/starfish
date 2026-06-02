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

#if (defined(STARFISH_SHELL_X11) ||              \
     defined(STARFISH_SHELL_X11_WEBCONTAINER) || \
     defined(STARFISH_SHELL_GLIB_HEADLESS)) &&   \
    (defined(STARFISH_GLIB_CAIRO_GL) || defined(STARFISH_GLIB_HEADLESS))

#include "AppLoop.h"

#include <glib.h>
#include <glib-unix.h>
#include <cstdio>

namespace StarfishShell {

class AppLoopGlib : public AppLoop {
public:
    AppLoopGlib();
    ~AppLoopGlib();

    virtual void init() override;
    virtual int start(double timeoutInSec = 0) override;
    virtual void stop() override;
    virtual void deinit() override;

private:
    static gboolean onSignal(gpointer data);

    GMainLoop* m_mainLoop = nullptr;
    guint m_timerID = 0;
    guint m_sigintSourceID = 0;
    guint m_sigtermSourceID = 0;
};

AppLoopGlib::AppLoopGlib()
{
    m_mainLoop = g_main_loop_new(nullptr, FALSE);
}

AppLoopGlib::~AppLoopGlib()
{
    if (m_mainLoop) {
        g_main_loop_unref(m_mainLoop);
        m_mainLoop = nullptr;
    }
}

void AppLoopGlib::init()
{
    // Use g_unix_signal_add for safe signal handling in GLib main loop
    m_sigintSourceID = g_unix_signal_add(SIGINT, onSignal, this);
    m_sigtermSourceID = g_unix_signal_add(SIGTERM, onSignal, this);
}

gboolean AppLoopGlib::onSignal(gpointer data)
{
    AppLoopGlib* self = static_cast<AppLoopGlib*>(data);
    // Mark source IDs as 0 since GLib will auto-remove the source after
    // G_SOURCE_REMOVE This prevents g_source_remove() from being called on
    // already-removed sources
    if (self->m_sigintSourceID) {
        self->m_sigintSourceID = 0;
    } else if (self->m_sigtermSourceID) {
        self->m_sigtermSourceID = 0;
    }
    self->stop();
    return G_SOURCE_REMOVE;
}

int AppLoopGlib::start(double timeoutInSec)
{
    if (timeoutInSec) {
        m_timerID = g_timeout_add_seconds(
            static_cast<guint>(timeoutInSec),
            [](gpointer data) -> gboolean {
                AppLoopGlib* self = static_cast<AppLoopGlib*>(data);
                self->m_timerID = 0;
                self->stop();
                return G_SOURCE_REMOVE;
            },
            this);
    }
    g_main_loop_run(m_mainLoop);
    return 0;
}

void AppLoopGlib::stop()
{
    if (m_timerID) {
        g_source_remove(m_timerID);
        m_timerID = 0;
    }
    if (g_main_loop_is_running(m_mainLoop)) {
        g_main_loop_quit(m_mainLoop);
    }
}

void AppLoopGlib::deinit()
{
    if (m_sigintSourceID) {
        g_source_remove(m_sigintSourceID);
        m_sigintSourceID = 0;
    }
    if (m_sigtermSourceID) {
        g_source_remove(m_sigtermSourceID);
        m_sigtermSourceID = 0;
    }
}

std::unique_ptr<AppLoop> AppLoop::create()
{
    return std::unique_ptr<AppLoopGlib>(new AppLoopGlib());
}

} // namespace StarfishShell

#endif
