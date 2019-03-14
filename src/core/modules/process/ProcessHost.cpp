/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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

#ifdef STARFISH_ENABLE_SERVICE_WORKER

#include "StarfishConfig.h"

#include <nanomsg/nn.h>
#include <nanomsg/pair.h>

#include "platform/process/base/ProcessType.h"
#include "platform/process/base/Process.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/threading/AdaptedThread.h"
#include "core/modules/threading/ThreadPool.h"
#include "core/modules/threading/IRunnable.h"
#include "core/modules/process/networking/Socket.h"
#include "core/modules/process/ProcessHostIORunnable.h"

#include "ProcessHost.h"

namespace Starfish {

ProcessHost::ProcessHost(ThreadPool* threadPool)
    : m_processState(ProcessState::INITIALIZED)
    , m_childPid(-1)
    , m_ioThread(nullptr)
    , m_threadPool(threadPool)
    , m_socket(nullptr)
{
}

ProcessHost::ProcessState ProcessHost::getState() const
{
    return m_processState;
}

Socket* ProcessHost::makeConnection(const char* endPointAddress)
{
    auto socket = new SocketNN(AF_SP, NN_PAIR);

    try {
        socket->connect(endPointAddress);

    } catch (const Socket::Exception& e) {
        if (e.num() == EADDRINUSE) {
            STARFISH_LOG_ERROR("The requested address is already in use.\n");
        } else {
            STARFISH_LOG_INFO("Networking failed due to %s\n", e.what());
        }
        return nullptr;
    }
    return socket;
}

bool ProcessHost::launch(std::vector<std::string>& args,
                         const char* endPointAddress)
{
    std::unique_lock<std::mutex> lock(m_stateMutex);

    if (m_processState == ProcessState::CHILD_PROCESS_STARTED) {
        return false;
    }

    STARFISH_ASSERT(m_threadPool);

    auto socket = makeConnection(endPointAddress);

    if (socket) {
        auto runnable =
            new ProcessHostIORunnable(m_threadPool->messageLoop(), this);

        runnable->addSocket(socket);

        m_ioThread = new AdaptedThread(m_threadPool);
        m_ioThread->start(runnable);

        PID pid = -1;
        // ProcessUtil::launchProcess(args, &pid);

        if (pid > 0) {
            m_childPid = pid;
            m_processState = ProcessState::CHILD_PROCESS_STARTED;
            return true;
        }
    }

    return false;
}

bool ProcessHost::terminate()
{
    bool result = false;

    std::unique_lock<std::mutex> lock(m_stateMutex);
    result = ProcessUtil::killProcess(m_childPid, false);

    if (result) {
        m_processState = ProcessState::CHILD_PROCESS_STOPPED;
    } else {
        m_processState = ProcessState::ERROR;
    }

    return result;
}

void ProcessHost::onReceived(int socketfd, const char* data)
{
    STARFISH_LOG_WARN("onReceived::data (%d): %s \n", socketfd, data);
};

void ProcessHost::onStopped()
{
    STARFISH_LOG_WARN("onStopped\n");
}

} // namespace Starfish

#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
