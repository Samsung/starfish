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
    !defined(__StarfishServiceWorkerServiceClient__)
#define __StarfishServiceWorkerServiceClient__

#include "core/modules/serviceworker/ServiceWorkerServiceInterface.h"

namespace Starfish {

struct ServiceWorkerJob;

class MessageLoop;
class ServiceWorkerServiceJobClient;

class ServiceWorkerServiceClient : public ServiceWorkerServiceClientInterface {
public:
    static ServiceWorkerServiceClient* getInstance();
    static void destroy();

    ServiceWorkerServiceClient(ServiceWorkerServiceClient const&) = delete;
    void operator=(ServiceWorkerServiceClient const&) = delete;

    void init(ServiceWorkerServiceJobClient* jobClient);
    ServiceWorkerServiceHostInterface* host();

    // Inteface overrided
    virtual void resolveJobPromise(ServiceWorkerJob* job) override;

private:
    static ServiceWorkerServiceClient* m_instance;
    ServiceWorkerServiceClient();
    virtual ~ServiceWorkerServiceClient();

    ServiceWorkerServiceJobClient* m_jobClient;
};

} // namespace Starfish
#endif
