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
#include "core/modules/serviceworker/WorkerConfig.h"
#include "core/modules/serviceworker/util/Trace.h"

#include "core/util/String.h"
#include "platform/file/PlatformDirectory.h"
#include "core/modules/serviceworker/Connection.h"

namespace Starfish {

#define SERVICE_WORKER_THREAD_POOL_SIZE 1

class ProcessResource {
public:
    static void acquire()
    {
        std::string path;
        const char* homePath = getenv("HOME");
        if (!homePath || strlen(homePath) == 0) {
            path = "/tmp";
        } else {
            path = homePath;
        }
        path += "/.ipc";

        // create a directory for ipc handles
        auto dir = PlatformDirectory::create();
        // TODO: Replace creating a GC-allocated string with `std::string`.
        if (!dir->open(
                String::createASCIIString(path.c_str(), path.length()))) {
            if (!dir->mkDir()) {
                STARFISH_LOG_ERROR("FAIL: Create a directory for ipc handles.");
                STARFISH_RELEASE_ASSERT(false);
            }
            TRACE(HOST, "New %s", path);
        }
        dir->close();

        // set the above directory path
        Connection::Config::setHandleDir(path);
    }

    static void release()
    {
        TRACE_SCOPE(CONFIG);
        // release the directory for ipc handles
        auto path = Connection::Config::getHandleDir();
        auto dir = PlatformDirectory::create();
        // TODO: Replace creating a GC-allocated string with `std::string`.
        if (dir->open(String::createASCIIString(path.c_str(), path.length()))) {
            dir->removeDir();
            TRACE(HOST, "Remove %s", path);
        }
        dir->close();
    }
};

PerProcess::PerProcess()
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
}

void PerProcess::initialize()
{
    TRACE_SCOPE(PERPROC);

    ProcessResource::acquire();

    m_messageLoop = new MessageLoop();

    m_threadPool =
        new ThreadPool(SERVICE_WORKER_THREAD_POOL_SIZE, m_messageLoop);
    m_ioRunnable = new IORunnable(m_messageLoop);
    m_ioThread = new AdaptedThread(m_threadPool);

    m_ioThread->start(m_ioRunnable);
}

void PerProcess::destroy()
{
    TRACE_SCOPE(PERPROC);
    STARFISH_ASSERT(m_ioThread);
    STARFISH_ASSERT(m_threadPool);
    STARFISH_ASSERT(m_messageLoop);

    m_ioThread->stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    m_threadPool->destroy();
    m_messageLoop->destroy();

    ProcessResource::release();
}

} // namespace Starfish

#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
