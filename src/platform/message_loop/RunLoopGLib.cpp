/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
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

#include "StarfishPlatform.h"
#if defined(PORT_EVENTLOOP_BACKEND_GLIB)

#include "StarfishConfig.h"
#include "platform/message_loop/RunLoopGLib.h"

#include <glib.h>

namespace Starfish {

RunLoopGLib::RunLoopGLib(bool referMainContext)
    : m_context(nullptr)
    , m_loop(nullptr)
    , m_ownsContext(false)
    , m_running(false)
{
    if (referMainContext) {
        m_context = reinterpret_cast<void*>(g_main_context_default());
    } else {
        m_context = g_main_context_new();
        m_ownsContext = true;
    }
}

RunLoopGLib::~RunLoopGLib()
{
    if (m_loop) {
        g_main_loop_unref(reinterpret_cast<GMainLoop*>(m_loop));
        m_loop = nullptr;
    }
    if (m_ownsContext && m_context) {
        g_main_context_unref(reinterpret_cast<GMainContext*>(m_context));
        m_context = nullptr;
    }
}

void RunLoopGLib::run()
{
    m_running = true;
    if (!m_loop) {
        m_loop = reinterpret_cast<void*>(
            g_main_loop_new(reinterpret_cast<GMainContext*>(m_context), FALSE));
    }
    g_main_loop_run(reinterpret_cast<GMainLoop*>(m_loop));
    m_running = false;
}

void RunLoopGLib::stop()
{
    if (m_running && m_loop) {
        g_main_loop_quit(reinterpret_cast<GMainLoop*>(m_loop));
    }
}

Optional<RunLoopGLib*> g_threadedMainRunLoop;

void* glibMainContext()
{
    if (g_threadedMainRunLoop) {
        return g_threadedMainRunLoop->mainContext();
    }
    return nullptr;
}

} // namespace Starfish
#endif
