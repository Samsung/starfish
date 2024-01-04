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
#ifndef __StarfishServiceWorkerHostConnection__
#define __StarfishServiceWorkerHostConnection__

#include "core/modules/serviceworker/Connection.h"
#include "core/modules/serviceworker/ConnectionInterface.h"

namespace Starfish {

class ServiceWorkerServerInterface;
class ServiceWorkerJob;
class ServiceWorkerRegistrationData;
class ServiceWorkerRequest;
class FetchEventResponseData;
class String;
class ExceptionData;
class Socket;

class ServiceWorkerHostConnection final
    : public Connection,
      public IServiceWorkerClientConnection {
public:
    ServiceWorkerHostConnection(ServiceWorkerServerInterface* server);

    // send
    void resolveJobPromise(
        ServiceWorkerJob* job,
        ServiceWorkerRegistrationData* registration) override;

    void rejectJobPromise(ServiceWorkerJob* job,
                          ExceptionData* errorData) override;

    void resolveRequest(ServiceWorkerRequest* request,
                        NULLABLE Archivable* archivable) override;

    void onUpdateRegistrationState(ServiceWorkerRegistrationData* registration,
                                   ServiceWorkerRegistrationState target,
                                   ServiceWorkerData* source) override;

    void onUpdateWorkerState(String* scriptURL,
                             ServiceWorkerState target) override;

    void fireEventRequest(String* scriptURL, String* eventName) override;

    void respondFetchEvent(FetchEventResponseData* data) override;

    // receive
    void onReceived(Socket* socket, const char* data, size_t len) override;

private:
    ServiceWorkerServerInterface* m_SWServer{ nullptr };
};

} // namespace Starfish

#endif
#endif
