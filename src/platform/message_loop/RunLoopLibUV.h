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
#if defined(PORT_EVENTLOOP_BACKEND_LIBUV) || defined(STARFISH_ENABLE_WORKER)

#ifndef __StarfishRunLoopLibUV__
#define __StarfishRunLoopLibUV__

#include "core/modules/message_loop/RunLoop.h"

#include <uv.h>

namespace Starfish {

class RunLoopLibUV final : public RunLoop {
    friend class RunLoop;

public:
    RunLoopLibUV(uv_loop_t* loop);

    ~RunLoopLibUV();

    uv_loop_t* uvLoop()
    {
        return m_uvLoop;
    }

    void run() override;
    void stop() override;

private:
    RunLoopLibUV();

    uv_loop_t* m_uvLoop;
    std::atomic<size_t> m_uvRunCount{ 0 };
};

} // namespace Starfish

#endif

#endif
