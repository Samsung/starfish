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

#if defined(STARFISH_USE_WORKER_PROCESS)

#include "StarfishConfig.h"

#include "core/modules/worker/PerProcess.h"
#include "core/modules/worker/WorkerSettings.h"
#include "core/modules/worker/WorkerHostManager.h"
#include "core/modules/worker/client/WorkerClientManager.h"
#include "core/modules/worker/WorkerManager.h"

namespace Starfish {

WorkerManager* WorkerManager::create(Starfish* starfish)
{
#if defined(STARFISH_WEBWORKER_HOST)
    return new WorkerHostManager(starfish);
#else
    return new WorkerClientManager(starfish);
#endif
}

WorkerManager::WorkerManager(Starfish* starfish)
    : m_workerSettings(new WorkerSettings())
    , m_perProcess(new PerProcess(starfish, m_workerSettings))
{
}

void WorkerManager::destroy()
{
    if (m_perProcess) {
        m_perProcess->destroy();
        m_perProcess = nullptr;
    }
}

WorkerHostManager* WorkerManager::asWorkerHostManager()
{
    STARFISH_ASSERT(isWorkerHostManager());
    return reinterpret_cast<WorkerHostManager*>(this);
}

WorkerClientManager* WorkerManager::asWorkerClientManager()
{
    STARFISH_ASSERT(isWorkerClientManager());
    return reinterpret_cast<WorkerClientManager*>(this);
}

} // namespace Starfish

#endif
