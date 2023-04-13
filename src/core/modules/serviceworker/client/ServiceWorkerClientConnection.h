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

#if defined(STARFISH_ENABLE_SERVICE_WORKER) && \
    !defined(__StarfishServiceWorkerClientConnection__)
#define __StarfishServiceWorkerClientConnection__

#include "core/modules/serviceworker/Connection.h"
#include "core/modules/serviceworker/ConnectionInterface.h"
#include "core/modules/serviceworker/ServiceWorkerTypes.h"

namespace Starfish {

class ServiceWorkerContainer;
class FetchEventRequestData;
class String;
class ServiceWorkerData;
class ServiceWorkerRegistrationData;

class ServiceWorkerClientConnection final
    : public Connection,
      public IServiceWorkerHostConnection {
public:
    ServiceWorkerClientConnection();

    // send
    void scheduleJob(ServiceWorkerJob* job) override;
    void matchRegistration(ServiceWorkerRequest* request,
                           String* clientURL) override;
    void updateServiceWorkerClient(ContextRequestData* request) override;
    void fetchEvent(FetchEventRequestData* FetchEventRequestData) override;
    void startServiceWorkerContext(
        ServiceWorkerData* serviceWorkerData) override;

    void sendMessage(const char* msgName, NULLABLE Archivable* param1 = nullptr,
                     NULLABLE Archivable* param2 = nullptr);

    // receive
    void onReceived(Socket* socket, const char* data, size_t len) override;
    void resolveJobPromise(
        ServiceWorkerJob* job,
        NULLABLE ServiceWorkerRegistrationData* registration);
    void rejectJobPromise(ServiceWorkerJob* job, ExceptionData* errorData);
    void updateRegistrationState(ServiceWorkerRegistrationData* registration,
                                 ServiceWorkerRegistrationState target,
                                 ServiceWorkerData* source);
    void updateWorkerState(String* scriptURL, ServiceWorkerState target);
    void fireEventRequest(String* scriptURL, String* eventName);
    void respondFetchEvent(FetchEventResponseData* data);

    Nullable<ServiceWorkerContainer*> findServiceWorkerContainer(
        ServiceWorkerContextId id);
};

} // namespace Starfish

#endif
