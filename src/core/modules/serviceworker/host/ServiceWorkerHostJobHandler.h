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
    ServiceWorkerHostJobHandler(MessageLoop* messageLoop);

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
                                 const char* target, ServiceWorkerData* source);
    void resolveJobPromise(ServiceWorkerJob* job,
                           ServiceWorkerRegistrationData* registration);
    ServiceWorkerRegistrationData* getRegistration(String* scope);
    void setRegistration(String* scope,
                         ServiceWorkerUpdateViaCache updateViaCacheMode);

    void matchRegistration(ServiceWorkerRequest* request, String* clientURL);

private:
    void queueTask(void (*fn)(size_t, void*), void* data);
    MessageLoop* m_messageLoop;
    GCUnorderedMap<ServiceWorkerRegistrationKey, JobQueue*> m_jobQueueMap;
    GCMap<ServiceWorkerRegistrationKey, ServiceWorkerRegistrationData*,
          ServiceWorkerRegistrationKeyComparator>
        m_registrationMap;
};
}

#endif
