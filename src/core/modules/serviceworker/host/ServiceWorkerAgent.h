/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_SERVICE_WORKER) && defined(STARFISH_WEBWORKER_HOST)
#ifndef __StarfishServiceWorkerAgent__
#define __StarfishServiceWorkerAgent__

#include "core/modules/worker/WorkerAgent.h"
#include "core/modules/serviceworker/ServiceWorkerTypes.h"

namespace Starfish {

class WebWorker;
class WorkerHostManager;
class PerProcess;
class ServiceWorkerData;
class NotificationService;
class ServiceWorkerServer;
class CastServer;
class ServiceWorkerGlobalScope;
class WorkerIPCAddress;

class ServiceWorkerAgent final : public WorkerAgent {
    friend class WorkerAgent;

public:
    static ServiceWorkerAgent* instance();

    void start() override;

    void destroy() override;

    void onWebWorkerTerminated(WebWorker* worker);

    void runServiceWorker(ServiceWorkerData* serviceWorker,
                          bool forceBypassCache = false);
    void runServiceWorker(ResourceURL* scriptURL);
    void abortServiceWorkerScript(ServiceWorkerData* serviceWorker);

#if defined(STARFISH_ENABLE_SERVICE_WORKER_NOTIFICATION)
    NotificationService* notificationService()
    {
        return m_notificationService;
    }
#endif

#if defined(STARFISH_ENABLE_CAST_SERVICE)
    CastServer* castServer()
    {
        return m_castServer;
    }
#endif

    ServiceWorkerServer* serviceWorkerServer()
    {
        return m_SWServer;
    }

    void addGlobalScope(ServiceWorkerContextId id,
                        ServiceWorkerGlobalScope* globalScope);
    void removeGlobalScope(ServiceWorkerContextId id);
    Optional<ServiceWorkerGlobalScope*> findGlobalScopeByContextId(
        ServiceWorkerContextId id);

    WorkerHostManager* workerHostManager() const
    {
        return m_workerHostManager;
    }

    Starfish* starfish() const
    {
        return m_starfish;
    }

private:
    ServiceWorkerAgent(Starfish* starfish);

    static ServiceWorkerAgent* m_instance;
    WorkerIPCAddress* m_ipcAddress;
    ServiceWorkerServer* m_SWServer;
#if defined(STARFISH_ENABLE_SERVICE_WORKER_NOTIFICATION)
    NotificationService* m_notificationService;
#endif
    GCVector<WebWorker*> m_webWorkerList;
    GCUnorderedMap<ServiceWorkerContextId, ServiceWorkerGlobalScope*, IdHash>
        m_globalScopeMap;

#if defined(STARFISH_ENABLE_CAST_SERVICE)
    CastServer* m_castServer;
#endif
};
} // namespace Starfish

#endif
#endif
