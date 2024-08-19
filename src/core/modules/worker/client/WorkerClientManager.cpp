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

#if defined(STARFISH_USE_WORKER_PROCESS) && !defined(STARFISH_WEBWORKER_HOST)

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/modules/worker/WorkerSettings.h"
#include "core/modules/sharedworker/client/SharedWorkerProcessManager.h"
#include "core/modules/serviceworker/client/ServiceWorkerProcessManager.h"
#include "core/modules/worker/client/WorkerClientManager.h"

namespace Starfish {

WorkerClientManager::WorkerClientManager(Starfish* starfish)
    : WorkerManager(starfish)
{
    m_workerSettings->setThreadPoolSize(s_threadPoolSize);

#if defined(STARFISH_ENABLE_SHARED_WORKER)
    m_sharedWorkerProcessManager = SharedWorkerProcessManager::instance();
    m_sharedWorkerProcessManager->init(m_perProcess);
#endif

#if defined(STARFISH_ENABLE_SERVICE_WORKER)
    m_serviceWorkerProcessManager = ServiceWorkerProcessManager::instance();
    m_serviceWorkerProcessManager->init(m_perProcess);
#endif
}

void WorkerClientManager::destroy()
{
#if defined(STARFISH_ENABLE_SHARED_WORKER)
    if (m_sharedWorkerProcessManager) {
        m_sharedWorkerProcessManager->destroy();
        m_sharedWorkerProcessManager = nullptr;
    }
#endif

#if defined(STARFISH_ENABLE_SERVICE_WORKER)
    if (m_serviceWorkerProcessManager) {
        m_serviceWorkerProcessManager->destroy();
        m_serviceWorkerProcessManager = nullptr;
    }
#endif

    WorkerManager::destroy();
}

} // namespace Starfish

#endif
