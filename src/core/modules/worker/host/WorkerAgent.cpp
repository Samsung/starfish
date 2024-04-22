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

#if defined(STARFISH_ENABLE_WORKER) && defined(STARFISH_WEBWORKER_HOST)

#include "StarfishConfig.h"
#include "Starfish.h"

#include "core/modules/worker/WorkerManager.h"
#include "core/modules/worker/host/WorkerHostManager.h"
#include "core/modules/sharedworker/host/SharedWorkerAgent.h"
#include "core/modules/serviceworker/host/ServiceWorkerAgent.h"
#include "core/modules/worker/host/WorkerAgent.h"

namespace Starfish {

WorkerAgent* WorkerAgent::g_workerAgentInstance = nullptr;

WorkerAgent* WorkerAgent::instance()
{
    STARFISH_ASSERT(g_workerAgentInstance);
    return g_workerAgentInstance;
}

bool WorkerAgent::isCreated()
{
    return (g_workerAgentInstance != nullptr);
}

WorkerAgent::WorkerAgent(Starfish* starfish)
    : m_starfish(starfish)
    , m_workerHostManager(starfish->workerManager()->asWorkerHostManager())
{
    LoggerOption::instance()->parseEnv();
}

void WorkerAgent::registerOnStatusChangedHandler(WorkerAgentStateHandler func)
{
    m_clientFunc = func;
}

PerProcess* WorkerAgent::perProcess() const
{
    return m_workerHostManager->perProcess();
}

void WorkerAgent::destroy()
{
    STARFISH_ASSERT(g_workerAgentInstance != nullptr);

    GC_FREE(g_workerAgentInstance);
    g_workerAgentInstance = nullptr;
}

} // namespace Starfish

#endif
