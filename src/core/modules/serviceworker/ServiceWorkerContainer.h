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
#include "core/util/Id.h"
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
class ServiceWorkerClientConnection;
class ServiceWorkerRegistrationData;

class ServiceWorkerContainer : public EventTarget,
                               public ServiceWorkerJobClient {
public:
    ServiceWorkerContainer(Document* document);
    virtual ~ServiceWorkerContainer();

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isServiceWorkerContainer() const override;

    virtual ExecutionContext* executionContext() override;

    ServiceWorker* controller();

    Promise* registerServiceWorker(String* scriptURL,
                                   RegistrationOptions* options = nullptr);
    Promise* registerServiceWorker(String* url, RegistrationOptions& options);
    Promise* getRegistration(String* scriptURL = nullptr);

    void startRegister(ResourceURL* scopeURL, ResourceURL* scriptURL,
                       Promise* p, ExecutionContext* client);

    ServiceWorkerJob* findJob(Id<ServiceWorkerJob> id);
    ServiceWorkerJob* createJob(ServiceWorkerJobType type, String* scopeURL,
                                String* scriptURL, Promise* p,
                                ExecutionContext* client);
    void scheduleJob(ServiceWorkerJob* job);
    void finishJob(ServiceWorkerJob* job);

    void resolveJobPromise(
        ServiceWorkerJob* job,
        ServiceWorkerRegistrationData* registration) override;

private:
    GCUnorderedMap<Id<ServiceWorkerJob>, ServiceWorkerJob*, IdHash> m_jobMap;
};
}

#endif
