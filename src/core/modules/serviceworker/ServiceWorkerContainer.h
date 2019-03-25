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
    !defined(__StarfishServiceWorkerContainer__)
#define __StarfishServiceWorkerContainer__

#include "core/dom/EventTarget.h"
#include "core/modules/serviceworker/ServiceWorkerTypes.h"
#include "core/modules/serviceworker/ServiceWorkerJob.h"
#include "core/modules/serviceworker/RegistrationOptions.h"
#include "core/modules/serviceworker/client/ServiceWorkerJobClient.h"

namespace Starfish {

class Promise;
class Document;
class ExecutionContext;
class ServiceWorker;
class ServiceWorkerJob;

class ServiceWorkerContainer : public EventTarget,
                               public ServiceWorkerJobClient {
public:
    ServiceWorkerContainer(Document* document);
    virtual ~ServiceWorkerContainer();

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(ServiceWorkerContainer)

    ServiceWorker* controller();

    Promise* registerServiceWorker(String* scriptURL,
                                   RegistrationOptions* options = nullptr);
    Promise* registerServiceWorker(String* url, RegistrationOptions& options);
    Promise* getRegistration(String* scriptURL = nullptr);

    void startRegister(ResourceURL* scopeURL, ResourceURL* scriptURL,
                       Promise* p, ExecutionContext* client);

    ServiceWorkerJob* createJob(ServiceWorkerJobType type, String* scopeURL,
                                String* scriptURL, Promise* p,
                                ExecutionContext* client);
    void scheduleJob(ServiceWorkerJob* job);

    virtual void resolveJobPromise(ServiceWorkerJob* job) override;

private:
    GCUnorderedMap<ServiceWorkerJobId, ServiceWorkerJob*> m_jobMap;
    ServiceWorkerJobId m_refValueToMakeServiceWorkerJobId;
};
}

#endif
