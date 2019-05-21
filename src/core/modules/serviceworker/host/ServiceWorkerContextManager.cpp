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

#include "core/util/Id.h"
#include "core/modules/serviceworker/ServiceWorkerTypes.h"
#include "core/modules/serviceworker/host/ServiceWorkerContextManager.h"

namespace Starfish {

ServiceWorkerContextManager* ServiceWorkerContextManager::m_instance = nullptr;

ServiceWorkerContextManager* ServiceWorkerContextManager::instance()
{
    if (m_instance == nullptr) {
        m_instance = new ServiceWorkerContextManager();
    }

    STARFISH_ASSERT(m_instance != nullptr);
    return m_instance;
}

void ServiceWorkerContextManager::destroy()
{
    if (m_instance != nullptr) {
        m_instance->~ServiceWorkerContextManager();
        m_instance = nullptr;
    }
}

void ServiceWorkerContextManager::init(ThreadPool* threadPool)
{
    STARFISH_ASSERT(threadPool != nullptr);
}

ServiceWorkerContextManager::~ServiceWorkerContextManager()
{
}

void ServiceWorkerContextManager::runServiceWorker(
    ServiceWorkerData* serviceWorker)
{
    STARFISH_ASSERT(serviceWorker != nullptr);
    // https://w3c.github.io/ServiceWorker/#run-service-worker

    // 4.1 Call the JavaScript InitializeHostDefinedRealm() abstract
    // operation with the following customizations:

    // - For the global object, create a new ServiceWorkerGlobalScope object.
    // Let workerGlobalScope be the created object.

    // - Let realmExecutionContext be the created JavaScript execution context.

    // 4.2 Set serviceWorker’s global object to workerGlobalScope.

    // 4.3 Let workerEventLoop be a newly created event loop.

    // 4.4 Let settingsObject be a new environment settings object whose
    // algorithms are defined as follows:

    // 4.11. If serviceWorker is an active worker, and there are any tasks
    // queued in serviceWorker’s containing service worker registration’s
    // task queues, queue them to serviceWorker’s event loop’s task queues
    // in the same order using their original task sources.

    // 4.15. Run the responsible event loop specified by settingsObject until
    // it is destroyed.

    // 4.16. Empty workerGlobalScope’s list of active timers.
}

void ServiceWorkerContextManager::abortServiceWorkerScript(
    ServiceWorkerData* serviceWorker)
{
    STARFISH_ASSERT(serviceWorker != nullptr);
}

} // namespace Starfish

#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
