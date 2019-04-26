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

class Socket;
class ServiceWorkerJob;
class ServiceWorkerRegistrationData;
class ServiceWorkerContainer;

typedef ServiceWorkerContainer* NullableServiceWorkerContainer;

class ServiceWorkerClientConnection final
    : public Connection,
      public ServiceWorkerHostProcessInterface {
public:
    ServiceWorkerClientConnection();

    // send
    void scheduleJob(ServiceWorkerJob* job) override;
    void matchRegistration(ServiceWorkerRequest* request,
                           String* clientURL) override;

    // receive
    void onReceived(Socket* socket, const char* data, size_t len) override;
    void resolveJobPromise(ServiceWorkerJob* job,
                           ServiceWorkerRegistrationData* registration);

    NullableServiceWorkerContainer findServiceWorkerContainer(
        ServiceWorkerContextId id);
};
} // namespace Starfish

#endif
