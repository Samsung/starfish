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
#if defined(PORT_EVENTLOOP_BACKEND_LIBUV)

#include "StarfishConfig.h"

#include "platform/message_loop/RunLoopLibUV.h"

namespace Starfish {

RunLoopLibUV::RunLoopLibUV()
    : m_uvLoop(new uv_loop_t())
{
    uv_loop_init(m_uvLoop);
}

RunLoopLibUV::~RunLoopLibUV()
{
    // Worker teardown closes async and timer handles on this loop too.
    // Their close callbacks must finish before the loop storage is freed.
    uv_run(m_uvLoop, UV_RUN_NOWAIT);
    int result = uv_loop_close(m_uvLoop);
    STARFISH_RELEASE_ASSERT(result == 0);
    delete m_uvLoop;
}

void RunLoopLibUV::run()
{
    size_t theCountBefore = m_uvRunCount;
    m_uvRunCount++;
    while (true) {
        uv_run(m_uvLoop, UV_RUN_ONCE);
        if (UNLIKELY(theCountBefore >= m_uvRunCount)) {
            break;
        }
    }
}

void RunLoopLibUV::stop()
{
    if (m_uvRunCount) {
        m_uvRunCount--;
    }
    uv_stop(m_uvLoop);
}

} // namespace Starfish
#endif
