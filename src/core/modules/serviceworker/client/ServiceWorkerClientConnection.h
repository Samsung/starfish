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

#if defined(STARFISH_ENABLE_SERVICE_WORKER) && \
    !defined(__StarfishServiceWorkerClientConnection__)
#define __StarfishServiceWorkerClientConnection__

namespace Starfish {

class ServiceWorkerContainer;

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

    void sendMessage(const char* msgName, NULLABLE Archivable* param1 = nullptr,
                     NULLABLE Archivable* param2 = nullptr);

    // receive
    void onReceived(Socket* socket, const char* data, size_t len) override;
    void resolveJobPromise(
        ServiceWorkerJob* job,
        NULLABLE ServiceWorkerRegistrationData* registration);
    void rejectJobPromise(ServiceWorkerJob* job, ErrorData* errorData);
    void updateWorkerState(ServiceWorkerRegistrationId id,
                           ServiceWorkerState target);

    NULLABLE ServiceWorkerContainer* findServiceWorkerContainer(
        ServiceWorkerContextId id);
};

} // namespace Starfish

#endif
