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

#include "core/modules/worker/WorkerConfig.h"
#include "core/modules/worker/util/Trace.h"
#include "core/modules/worker/util/network/IORunnable.h"
#include "core/modules/worker/util/network/Connection.h"
#include "core/modules/worker/util/LocalStorageHelper.h"
#include "core/modules/threading/ThreadPool.h"
#include "core/modules/threading/IRunnable.h"
#include "core/modules/threading/AdaptedThread.h"
#include "core/modules/message_loop/MessageLoop.h"

#include "core/modules/worker/PerProcess.h"

namespace Starfish {

#define IO_EVENT_POLLING_TIMEOUT_MS 300

class ProcessResource {
public:
    static void acquire(const std::string& rootPath)
    {
        // TODO: consider making parent directories as needed.
        LocalStorageHelper::File::mkdirIfNotExists(rootPath);
        std::string path = rootPath;

        path += PATH_IPC_DIR;
        // create a directory for ipc handles
        auto dir = PlatformDirectory::create();
        // TODO: Replace creating a GC-allocated string with `std::string`.
        LocalStorageHelper::File::mkdirIfNotExists(path);
        // set the above directory path
        Connection::Config::setHandleDir(path);
    }

    static void release()
    {
        TRACE_SCOPE(CONFIG);
        // release the directory for ipc handles
        auto path = Connection::Config::getHandleDir();
        // TODO: Replace creating a GC-allocated string with `std::string`.
        if (!GlobalOptions::instance().has("--leave-ipc-handle")) {
            LocalStorageHelper::File::remove(path);
            TRACE(IPC, "Remove", path);
        }
    }
};

PerProcess::PerProcess(WorkerSettings* settings)
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

    m_workerSettings = settings;
}

void PerProcess::initialize()
{
    TRACE_SCOPE(PERPROC);
    if (m_isInitialized) {
        return;
    }

    ProcessResource::acquire(m_workerSettings->dataDirectoryPath());
    m_workerSettings->addOnChangeDataDirectoryPathCallback(
        [](const std::string& path) {
            ProcessResource::release();
            ProcessResource::acquire(path);
        });

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

    TRACE(PERPROC, "Start waiting for the I/O thread stopped");
    m_ioThread->stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    TRACE(PERPROC, "Stop waiting");

    m_threadPool->destroy();
    m_messageLoop->destroy();

    ProcessResource::release();
}

Nullable<WorkerSettings::ProcessExecutorCallback>
PerProcess::serviceWorkerProcessExecutor()
{
    WorkerSettings::ProcessExecutorCallback executor =
        m_workerSettings->serviceWorkerProcessExecutor();
    if (!executor) {
        return Nullable<WorkerSettings::ProcessExecutorCallback>();
    }

    return Nullable<WorkerSettings::ProcessExecutorCallback>(executor);
}

} // namespace Starfish

#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
