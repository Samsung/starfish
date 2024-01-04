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
#ifndef __ServiceWorkerHostJobHandler__
#define __ServiceWorkerHostJobHandler__

#include "core/modules/serviceworker/ServiceWorkerUpdateViaCache.h"
#include <string>

namespace Starfish {

class Starfish;
class MessageLoop;
class ServiceWorker;
class ServiceWorkerJob;
class ServiceWorkerData;
class JobQueue;
class ServiceWorkerRegistrationData;
class ExceptionData;
class ServiceWorkerServerInterface;
class ContextRequestData;
class FetchEventRequestData;
class ServiceWorkerHostConnection;
class RegistrationStore;

using ServiceWorkerRegistrationKey = String*;

struct RegistrationIdentifier : public gc {
    String* m_scope;
    String* m_origin;
};

class ServiceWorkerHostJobHandler : public gc {
public:
    ServiceWorkerHostJobHandler(Starfish* starfish, MessageLoop* messageLoop,
                                ServiceWorkerServerInterface* server);

    void scheduleJob(ServiceWorkerJob* job);
    void runJob(JobQueue* jobQueue);
    void finishJob(ServiceWorkerJob* job);
    void registerServiceWorker(ServiceWorkerJob* job);
    void unregisterServiceWorker(ServiceWorkerJob* job);
    void update(ServiceWorkerJob* job);
    void startServiceWorkerContext(ServiceWorkerData* serviceWorkerData);
    void runServiceWorker(ServiceWorkerData* serviceWorker);
    void install(ServiceWorkerJob* job, ServiceWorkerData* worker,
                 ServiceWorkerRegistrationData* registration);
    void updateRegistrationState(ServiceWorkerRegistrationData* registration,
                                 ServiceWorkerRegistrationState target,
                                 ServiceWorkerData* source);
    void updateWorkerState(ServiceWorkerData* worker, ServiceWorkerState state);
    ServiceWorkerData* getNewestWorker(
        ServiceWorkerRegistrationData* registration);

    void activate(ServiceWorkerRegistrationData* registration);
    void tryActivate(ServiceWorkerRegistrationData* registration);
    bool serviceWorkerHasNoPendingEvents(ServiceWorkerData* serviceWorker);
    bool shouldSkipEvent(std::string eventName,
                         ServiceWorkerData* serviceWorker);

    void resolveJobPromise(
        ServiceWorkerJob* job,
        NULLABLE ServiceWorkerRegistrationData* registration);

    bool tryClearRegistration(ServiceWorkerRegistrationData* registration);
    void clearRegistration(ServiceWorkerRegistrationData* registration);
    void terminateServiceWorker(ServiceWorkerData* serviceWorker);

    void rejectJobPromise(ServiceWorkerJob* job, ExceptionData* errorData);

    NULLABLE ServiceWorkerRegistrationData* getRegistration(String* scope);
    NULLABLE ServiceWorkerRegistrationData* getRegistration(
        ServiceWorkerRegistrationId registrationId);

    void setRegistration(String* scope,
                         ServiceWorkerUpdateViaCache updateViaCacheMode);

    NULLABLE ServiceWorkerRegistrationData* matchRegistration(
        ServiceWorkerRequest* request, String* clientURL);

    void updateServiceWorkerClient(ContextRequestData* request);

    bool isEmptyRegistrationMap()
    {
        return m_scopeToRegistrationMap.size() == 0;
    }

    void handleFetch(FetchEventRequestData* data,
                     ServiceWorkerHostConnection* connection);

    RegistrationStore* registrationStore()
    {
        return m_registrationStore;
    }

private:
    void queueTask(void (*fn)(size_t, void*), void* data);

    MessageLoop* m_messageLoop;
    ServiceWorkerServerInterface* m_SWServer;
    RegistrationStore* m_registrationStore;

    GCUnorderedMap<ServiceWorkerRegistrationKey, JobQueue*>
        m_scopeToJobQueueMap;

    GCUnorderedMap<ServiceWorkerClientId, ServiceWorkerRegistrationId, IdHash>
        m_clientIdToRegistrationIdMap;

    ServiceWorkerRegistrationMap m_scopeToRegistrationMap;
};
} // namespace Starfish

#endif
#endif
