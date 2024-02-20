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

#ifndef __StarfishMessageLoopLibUV__
#define __StarfishMessageLoopLibUV__

#include "core/modules/message_loop/MessageLoop.h"

#include <uv.h>

namespace Starfish {

class RunLoopLibUV;

class MessageLoopLibUV : public MessageLoop {
    friend class MessageLoop;

public:
    static void init();
    static void run();
    static void stop();
    static void runOnMainThreadSync(const std::function<void()>& functor);

    size_t addIdler(GlobalScope* globalScope, void (*fn)(size_t handle, void*),
                    void* data) override;
    size_t addIdler(GlobalScope* globalScope,
                    void (*fn)(size_t handle, void*, void*), void* data,
                    void* data1) override;
    size_t addIdler(GlobalScope* globalScope,
                    void (*fn)(size_t handle, void*, void*, void*), void* data,
                    void* data1, void* data2) override;

    void removeIdler(size_t handle) override;
    void removeIdlerWithNoGCRooting(size_t handle) override;

    void clearPendingIdlers(GlobalScope* globalScope) override;

    void destroy() override;

    void runOnMainThreadAsync(const std::function<void()>& functor) override;

    size_t addIdlerWithNoGCRootingInOtherThread(GlobalScope* globalScope,
                                                void (*fn)(size_t handle,
                                                           void*),
                                                void* data) override;
    size_t addIdlerWithNoGCRootingInOtherThread(
        GlobalScope* globalScope, void (*fn)(size_t handle, void*, void*),
        void* data, void* data1) override;

    RunLoop* runLoop() override;

    uv_loop_t* uvLoop();

private:
    MessageLoopLibUV();
    MessageLoopLibUV(RunLoopLibUV* runLoop);

    RunLoopLibUV* m_runLoop;
    uv_async_t* m_idlerThreadAsyncHandle{ nullptr };
    std::list<size_t> m_idlersFromOtherThreadForUV;
};

} // namespace Starfish

#endif

#endif
