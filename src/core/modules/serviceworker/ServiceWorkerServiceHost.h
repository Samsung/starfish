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
    !defined(__StarfishServiceWorkerServiceHost__)
#define __StarfishServiceWorkerServiceHost__

#include "core/modules/serviceworker/ServiceWorkerEnums.h"
#include "core/modules/serviceworker/ServiceWorkerServiceInterface.h"

namespace Starfish {

/*
    TODO: generate ServiceWorkerRegistrationKey
    an ordered map where the keys are scope urls, serialized,
    and the values are service worker registrations.
*/
using ServiceWorkerRegistrationKey = String*;

struct RegistrationIdentifier : public gc {
    String* m_scope;
    String* m_origin;
};

class ServiceWorkerJob;
class ServiceWorkerServiceHostJobQueue;
class MessageLoop;
class ServiceWorkerRegistrationData;
class ServiceWorkerServiceClientInterface;

struct ServiceWorkerRegistrationKeyComparator {
    bool operator()(const ServiceWorkerRegistrationKey& lhs,
                    const ServiceWorkerRegistrationKey& rhs) const
    {
        return lhs < rhs;
    }
};

class ServiceWorkerServiceHost : public ServiceWorkerServiceHostInterface {
public:
    static ServiceWorkerServiceHost* getInstance();
    static void destroy();
    ServiceWorkerServiceHost(ServiceWorkerServiceHost const&) = delete;
    void operator=(ServiceWorkerServiceHost const&) = delete;

    void init(MessageLoop* messageLoop);

    virtual void scheduleJob(ServiceWorkerJob* job) override;

    ServiceWorkerRegistrationData* getRegistration(String* scope);
    void setRegistration(String* scope,
                         ServiceWorkerUpdateViaCache updateViaCacheMode);

    ServiceWorkerServiceClientInterface* client();

private:
    static ServiceWorkerServiceHost* m_instance;
    ServiceWorkerServiceHost();
    virtual ~ServiceWorkerServiceHost();

    MessageLoop* m_messageLoop;
    GCUnorderedMap<ServiceWorkerRegistrationKey,
                   ServiceWorkerServiceHostJobQueue*>
        m_jobQueueMap;
    GCMap<ServiceWorkerRegistrationKey, ServiceWorkerRegistrationData*,
          ServiceWorkerRegistrationKeyComparator>
        m_registrationMap;
};

} // namespace Starfish
#endif
