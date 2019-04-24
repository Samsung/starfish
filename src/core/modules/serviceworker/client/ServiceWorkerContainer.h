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
#include "core/util/Archivable.h"
#include "core/modules/serviceworker/ServiceWorkerJobData.h"
#include "core/modules/serviceworker/ServiceWorkerJob.h"
#include "core/modules/serviceworker/client/RegistrationOptions.h"

namespace Starfish {

class Promise;
class Document;
class ExecutionContext;
class ServiceWorker;
class ServiceWorkerJob;
class ServiceWorkerRequest;
class ServiceWorkerClientConnection;
class ServiceWorkerRegistrationData;

typedef ServiceWorkerRequest NullableServiceWorkerRequest;
typedef ServiceWorkerJob NullableServiceWorkerJob;

// typedef void (*Handler_t)(ServiceWorkerRequest*, TaskParam);
// // using RequestId = Id<ServiceWorkerRequest>;
// using RequestTask = Task<Handler_t>;
// using RequestTasker = Tasker<RequestId, RequestTask*>;

class ServiceWorkerContainer : public EventTarget {
public:
    ServiceWorkerContainer(ExecutionContext* executionContext);
    virtual ~ServiceWorkerContainer();

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isServiceWorkerContainer() const override;
    virtual ExecutionContext* executionContext() const override;
    void dispose();

    Promise* registerServiceWorker(
        String* scriptURL,
        RegistrationOptions* options = nullptr); // binding interface
    Promise* registerServiceWorker(
        String* url, RegistrationOptions& options);        // binding interface
    Promise* getRegistration(String* scriptURL = nullptr); // binding interface
    ServiceWorker* controller();                           // binding interface

    void startRegister(ResourceURL* scopeURL, ResourceURL* scriptURL,
                       Promise* p, ExecutionContext* client);

    NullableServiceWorkerJob* findJob(Id<ServiceWorkerJob> id);
    ServiceWorkerJob* createJob(ServiceWorkerJobType type, String* scopeURL,
                                String* scriptURL, Promise* p,
                                ExecutionContext* client);
    void scheduleJob(ServiceWorkerJob* job);
    void finishJob(ServiceWorkerJob* job);
    void resolveJobPromise(ServiceWorkerJob* job,
                           ServiceWorkerRegistrationData* registration);

    void matchRegistration(ServiceWorkerRequest* request,
                           ResourceURL* clientURL);
    void resolveMatchRegistration(ServiceWorkerRequest* request,
                                  ServiceWorkerRegistrationData* registration);

    ServiceWorkerRequest* createRequest(const char* requestName,
                                        Promise* promise);
    NullableServiceWorkerRequest* findRequest(Id<ServiceWorkerRequest> id);
    void finishRequest(ServiceWorkerRequest* request);

private:
    enum class State {
        Started,
        Stopped,
        Disposed,
    };

    ExecutionContext* m_executionContext;
    GCUnorderedMap<Id<ServiceWorkerJob>, ServiceWorkerJob*, IdHash> m_jobMap;
    GCUnorderedMap<Id<ServiceWorkerRequest>, ServiceWorkerRequest*, IdHash>
        m_requestMap;
    State m_state;
};
} // namespace Starfish

#endif
