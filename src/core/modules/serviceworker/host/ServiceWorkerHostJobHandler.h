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
    !defined(__ServiceWorkerHostJobHandler__)
#define __ServiceWorkerHostJobHandler__

namespace Starfish {

class MessageLoop;
class ServiceWorker;
class ServiceWorkerJob;
class ServiceWorkerData;
class JobQueue;
class ServiceWorkerRegistrationData;
class ErrorData;
class ServiceWorkerServerClient;

using ServiceWorkerRegistrationKey = String*;

struct RegistrationIdentifier : public gc {
    String* m_scope;
    String* m_origin;
};

struct ServiceWorkerRegistrationKeyComparator {
    bool operator()(const ServiceWorkerRegistrationKey& lhs,
                    const ServiceWorkerRegistrationKey& rhs) const
    {
        return lhs < rhs;
    }
};

class ServiceWorkerHostJobHandler : public gc {
public:
    ServiceWorkerHostJobHandler(MessageLoop* messageLoop,
                                ServiceWorkerServerClient* client);

    void scheduleJob(ServiceWorkerJob* job);
    void runJob(JobQueue* jobQueue);
    void finishJob(ServiceWorkerJob* job);
    void registerServiceWorker(ServiceWorkerJob* job);
    void unregisterServiceWorker(ServiceWorkerJob* job);
    void update(ServiceWorkerJob* job);
    void runServiceWorker(ServiceWorkerData* serviceWorker);
    void install(ServiceWorkerJob* job, ServiceWorkerData* worker,
                 ServiceWorkerRegistrationData* registration);
    void updateRegistrationState(ServiceWorkerRegistrationData* registration,
                                 const char* target,
                                 NULLABLE ServiceWorkerData* source);
    void updateWorkerState(ServiceWorkerData* worker, ServiceWorkerState state);

    void resolveJobPromise(
        ServiceWorkerJob* job,
        NULLABLE ServiceWorkerRegistrationData* registration);

    bool tryClearRegistration(ServiceWorkerRegistrationData* registration);
    void clearRegistration(ServiceWorkerRegistrationData* registration);

    void rejectJobPromise(ServiceWorkerJob* job, ErrorData* errorData);

    NULLABLE ServiceWorkerRegistrationData* getRegistration(String* scope);
    NULLABLE ServiceWorkerRegistrationData* getRegistration(
        ServiceWorkerRegistrationId registrationId);

    void setRegistration(String* scope,
                         ServiceWorkerUpdateViaCache updateViaCacheMode);

    NULLABLE ServiceWorkerRegistrationData* matchRegistration(
        ServiceWorkerRequest* request, String* clientURL);

    void updateServiceWorkerClient(ContextRequestData* request);

private:
    void queueTask(void (*fn)(size_t, void*), void* data);

    MessageLoop* m_messageLoop;
    ServiceWorkerServerClient* m_SWServerClient;

    GCUnorderedMap<ServiceWorkerRegistrationKey, JobQueue*>
        m_scopeToJobQueueMap;

    GCUnorderedMap<ServiceWorkerClientId, ServiceWorkerRegistrationId, IdHash>
        m_clientIdToRegistrationIdMap;

    GCMap<ServiceWorkerRegistrationKey, ServiceWorkerRegistrationData*,
          ServiceWorkerRegistrationKeyComparator>
        m_scopeToRegistrationMap;
};
} // namespace Starfish

#endif
