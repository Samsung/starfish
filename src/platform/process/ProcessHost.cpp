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

#include "platform/process/base/Process.h"
#include "platform/process/ProcessHost.h"

namespace Starfish {

ProcessHost::ProcessHost()
    : m_processState(ProcessState::INITIALIZED)
    , m_childPid(-1)
{
}

ProcessHost::~ProcessHost()
{
}

void ProcessHost::setState(const ProcessState state)
{
    std::unique_lock<std::mutex> lock(m_stateMutex);
    m_processState = state;
}

bool ProcessHost::launch(std::vector<std::string>& args)
{
    PID pid = -1;

    ProcessUtil::launchProcess(args, &pid);

    if (pid > 0) {
        m_childPid = pid;
        setState(ProcessState::CHILD_PROCESS_STARTED);
        return true;
    }

    return false;
}

bool ProcessHost::terminate()
{
    bool result = false;

    {
        std::unique_lock<std::mutex> lock(m_stateMutex);
        result = ProcessUtil::killProcess(m_childPid, false);
    }

    if (result) {
        setState(ProcessState::CHILD_PROCESS_STOPPED);
    } else {
        setState(ProcessState::ERROR);
    }

    return result;
}

} // namespace Starfish

#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
