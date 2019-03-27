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
    !defined(__StarfishServiceWorkerHostJobQueue__)
#define __StarfishServiceWorkerHostJobQueue__

namespace Starfish {

class ServiceWorkerJob;
class MessageLoop;
class ServiceWorkerRegistrationData;
class ServiceWorker;
class ServiceWorkerData;

class ServiceWorkerHostJobQueue : public gc {
public:
    ServiceWorkerHostJobQueue(MessageLoop* messageLoop);

    void enqueueJob(ServiceWorkerJob* job);
    size_t size() const;
    const ServiceWorkerJob* firstJob() const;
    const ServiceWorkerJob* lastJob() const;

    // TODO: The followings jobs needs to be encapsulated on Job class.
    void runJob();
    void runRegisterJob(ServiceWorkerJob* job);
    void runUnregisterJob(ServiceWorkerJob* job);
    void runUpdateJob(ServiceWorkerJob* job);
    void runServiceWorker(ServiceWorkerData* serviceWorker);
    void runInstallJob(ServiceWorkerJob* job, ServiceWorkerData* worker,
                       ServiceWorkerRegistrationData* registration);
    void runUpdateRegistrationState(ServiceWorkerRegistrationData* registration,
                                    const char* target,
                                    ServiceWorkerData* source);
    void runResolveJobPromise(ServiceWorkerJob* job);
    void finishJob();

    void queueTask(void (*fn)(size_t, void*), void* data);

private:
    GCDeque<ServiceWorkerJob*> m_jobQueue;
    MessageLoop* m_messageLoop;
};
}

#endif
