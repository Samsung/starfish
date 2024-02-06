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
#ifndef __StarfishServiceWorkerServer__
#define __StarfishServiceWorkerServer__

#include "core/modules/serviceworker/host/ServiceWorkerServerInterface.h"

namespace Starfish {

/*
    TODO: generate ServiceWorkerRegistrationKey
    an ordered map where the keys are scope urls, serialized,
    and the values are service worker registrations.
*/

class JobQueue;
class PerProcess;
class ProgramOptions;
class ServiceWorkerJob;
class ServiceWorkerHostJobHandler;
class ServiceWorkerHostConnection;
class ServiceWorkerRegistrationData;
class IServiceWorkerClientConnection;
class ServiceWorkerContextManager;

class ServiceWorkerServer : public gc, public ServiceWorkerServerInterface {
public:
    ServiceWorkerServer(PerProcess* perProcess);
    virtual ~ServiceWorkerServer();

    ServiceWorkerServer(ServiceWorkerServer const&) = delete;
    void operator=(ServiceWorkerServer const&) = delete;

    void destroy();
    void start();
    void start(std::shared_ptr<ProgramOptions> programOptions);

    void getConnections(
        GCVector<IServiceWorkerClientConnection*>& connections) override;
    ServiceWorkerHostJobHandler* jobHandler() override;

    bool tryTerminate() override;
    bool isTerminating() override;

    DEFINE_GETTER(ServiceWorkerHostConnection*, connection);
    DEFINE_SETTER(ServiceWorkerServerClient*, client, Client);

private:
    void registerConnection(ServiceWorkerHostConnection* connection);

    PerProcess* m_perProcess{ nullptr };
    ServiceWorkerHostJobHandler* m_jobHandler{ nullptr };
    ServiceWorkerHostConnection* m_connection{ nullptr };
    GCVector<ServiceWorkerHostConnection*> m_connections;
    bool m_isTerminating{ false };
    ServiceWorkerServerClient* m_client{ nullptr };
};

} // namespace Starfish

#endif
#endif
