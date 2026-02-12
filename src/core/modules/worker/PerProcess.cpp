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

#if defined(STARFISH_USE_WORKER_PROCESS)

#include "StarfishConfig.h"
#include "platform/file/PlatformDirectory.h"
#include "core/util/String.h"
#include "core/util/debug/Trace.h"

#include "core/modules/worker/WorkerConfig.h"
#include "core/modules/worker/WorkerSettings.h"
#include "core/modules/worker/util/network/IORunnable.h"
#include "core/modules/worker/util/network/Connection.h"
#include "core/modules/threading/ThreadPool.h"
#include "core/modules/threading/IRunnable.h"
#include "core/modules/threading/AdaptedThread.h"
#include "core/modules/message_loop/MessageLoop.h"

#include "core/modules/worker/PerProcess.h"

namespace Starfish {

#define IO_EVENT_POLLING_TIMEOUT_MS 300

PerProcess::PerProcess(Starfish* starfish, WorkerSettings* settings)
{
    LogOption::setExternalIsEnabled([](const std::string& id) -> bool {
        if (GlobalOptions::instance().has("TRACE", id.c_str())) {
            return true;
        }
        return false;
    });

    TRACE_SCOPE(PERPROC);
    static bool isOnceCreated = false;
    STARFISH_ASSERT(!isOnceCreated);
    isOnceCreated = true;

    m_starfish = starfish;
    m_workerSettings = settings;
}

void PerProcess::initialize()
{
    TRACE_SCOPE(PERPROC);
    if (m_isInitialized) {
        return;
    }

    m_messageLoop = MessageLoop::create();

    m_threadPool =
        new ThreadPool(m_workerSettings->threadPoolSize(), m_messageLoop);
    m_ioRunnable = new IORunnable(m_messageLoop, IO_EVENT_POLLING_TIMEOUT_MS);
    m_ioThread = new AdaptedThread(m_threadPool);

    m_ioThread->start(m_ioRunnable);

    m_isInitialized = true;
}

void PerProcess::destroy()
{
    if (!m_isInitialized) {
        return;
    }

    TRACE_SCOPE(PERPROC);
    STARFISH_ASSERT(m_ioThread);
    STARFISH_ASSERT(m_threadPool);
    STARFISH_ASSERT(m_messageLoop);

    m_ioThread->stop();
    m_ioThread->join();

    m_threadPool->destroy();
    m_messageLoop->destroy();
}

} // namespace Starfish

#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
