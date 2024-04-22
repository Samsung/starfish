/*
 * Copyright (c) 2024-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_SHARED_WORKER) && defined(STARFISH_WEBWORKER_HOST)

#include "StarfishConfig.h"
#include "Starfish.h"

#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/worker/util/Trace.h"
#include "core/modules/worker/WorkerIPCAddress.h"
#include "core/modules/worker/WorkerConfig.h"
#include "core/modules/worker/PerProcess.h"
#include "core/modules/worker/host/WorkerHostManager.h"
#include "core/modules/sharedworker/host/SharedWorkerAgentServer.h"
#include "core/modules/sharedworker/host/SharedWorkerAgent.h"

namespace Starfish {

WorkerAgent* WorkerAgent::create(Starfish* starfish)
{
    TRACE(SHAREDWORKER);

    if (!WorkerAgent::g_workerAgentInstance) {
        WorkerAgent::g_workerAgentInstance =
            new (NoGC) SharedWorkerAgent(starfish);
    }

    return WorkerAgent::g_workerAgentInstance;
}

SharedWorkerAgent* SharedWorkerAgent::instance()
{
    STARFISH_ASSERT(WorkerAgent::g_workerAgentInstance != nullptr);

    return reinterpret_cast<SharedWorkerAgent*>(
        WorkerAgent::g_workerAgentInstance);
}

SharedWorkerAgent::SharedWorkerAgent(Starfish* starfish)
    : WorkerAgent(starfish)
{
    PerProcess* perProcess = m_workerHostManager->perProcess();
    perProcess->initialize();

    m_ipcAddress = new WorkerIPCAddress(perProcess->workerSettings(),
                                        PATH_SHARED_WORKER_IPC_DIR);
    m_ipcAddress->acquire();

    m_server = new SharedWorkerAgentServer(
        perProcess, m_ipcAddress->createIPCAddress(WORKER_IPC_PROCESS_NAME));
}

void SharedWorkerAgent::start()
{
    m_server->start();
}

void SharedWorkerAgent::destroy()
{
    TRACE(SHAREDWORKER);

    m_server->close();

    m_ipcAddress->release();

    WorkerAgent::destroy();
}

} // namespace Starfish

#endif
