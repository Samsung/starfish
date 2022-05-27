/*
 * Copyright (c) 2022-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_SERVICE_WORKER)

#include "PerProcess.h"
#include "StarfishConfig.h"
#include "core/modules/threading/ThreadPool.h"
#include "core/modules/threading/AdaptedThread.h"
#include "core/modules/threading/IRunnable.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/serviceworker/IORunnable.h"

namespace Starfish {

#define SERVICE_WORKER_THREAD_POOL_SIZE 1

PerProcess::PerProcess()
{
    static bool isOnceCreated = false;
    STARFISH_ASSERT(!isOnceCreated);
    isOnceCreated = true;
}

void PerProcess::initialize()
{
    messageLoop_ = new MessageLoop();
    threadPool_ = new ThreadPool(SERVICE_WORKER_THREAD_POOL_SIZE, messageLoop_);
    ioRunnable_ = new IORunnable(messageLoop_);
    ioThread_ = new AdaptedThread(threadPool_);

    ioThread_->start(ioRunnable_);
}

void PerProcess::destroy()
{
    STARFISH_ASSERT(ioThread_);
    STARFISH_ASSERT(threadPool_);
    STARFISH_ASSERT(messageLoop_);

    ioThread_->stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    threadPool_->destroy();
    messageLoop_->destroy();
}

} // namespace Starfish

#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
