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

#ifdef STARFISH_ENABLE_SERVICE_WORKER

#include "StarfishConfig.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/serviceworker/ServiceWorkerServiceHostJobQueue.h"
#include "core/modules/serviceworker/ServiceWorkerJob.h"
#include "core/modules/serviceworker/ServiceWorkerServiceHost.h"
#include "core/modules/serviceworker/ServiceWorkerRegistration.h"
#include "core/modules/serviceworker/ServiceWorkerServiceClient.h"
#include "core/modules/serviceworker/ServiceWorker.h"

namespace Starfish {

ServiceWorkerServiceHostJobQueue::ServiceWorkerServiceHostJobQueue(
    MessageLoop* messageLoop)
    : m_jobQueue()
    , m_messageLoop(messageLoop)
{
}

void ServiceWorkerServiceHostJobQueue::enqueueJob(ServiceWorkerJob* job)
{
    m_jobQueue.push_back(job);
}

size_t ServiceWorkerServiceHostJobQueue::size() const
{
    return m_jobQueue.size();
}

const ServiceWorkerJob* ServiceWorkerServiceHostJobQueue::firstJob() const
{
    return m_jobQueue.front();
}

const ServiceWorkerJob* ServiceWorkerServiceHostJobQueue::lastJob() const
{
    return m_jobQueue.back();
}

void ServiceWorkerServiceHostJobQueue::runJob()
{
    // https://w3c.github.io/ServiceWorker/#run-job-algorithm

    // 1. Assert: jobQueue is not empty.
    STARFISH_ASSERT(size() != 0);

    // 2. Queue a task to run these steps in parallel.
    struct Params : public gc {
        ServiceWorkerServiceHostJobQueue* queue;
        ServiceWorkerJob* job;
    };

    auto params = new Params();
    params->queue = this;

    // 2.1 Let job be the first item in jobQueue.
    params->job = m_jobQueue.front();

    queueTask(
        [](size_t handle, void* data) {
            Params* params = static_cast<Params*>(data);
            ServiceWorkerServiceHostJobQueue* queue = params->queue;
            ServiceWorkerJob* job = params->job;

            // 2.2, 2.3, 2.4
            switch (job->type) {
            case ServiceWorkerJobType::Register:
                queue->runRegisterJob(job);
                break;
            case ServiceWorkerJobType::Update:
                queue->runUpdateJob(job);
                break;
            case ServiceWorkerJobType::Unregister:
                queue->runUnregisterJob(job);
                break;
            default:
                break;
            }
        },
        params);
}

void ServiceWorkerServiceHostJobQueue::queueTask(void (*fn)(size_t, void*),
                                                 void* data)
{
    m_messageLoop->addIdler(nullptr, fn, data);
}

void ServiceWorkerServiceHostJobQueue::runRegisterJob(ServiceWorkerJob* job)
{
    // https://w3c.github.io/ServiceWorker/#register-algorithm

    // 4. Let registration be the result of running the Get Registration
    // algorithm passing job’s scope url as the argument.
    auto host = ServiceWorkerServiceHost::getInstance();
    auto registration = host->getRegistration(job->scopeURL);

    if (registration) {
        // 5. If registration is not null, then:
    } else {
        // 6. Invoke Set Registration algorithm with job’s scope url and job’s
        // update via cache mode.
        host->setRegistration(job->scopeURL, job->updateViaCacheMode);
    }

    // 7. Invoke Update algorithm passing job as the argument
    runUpdateJob(job);
}

void ServiceWorkerServiceHostJobQueue::runUpdateJob(ServiceWorkerJob* job)
{
    // https://w3c.github.io/ServiceWorker/#update-algorithm
    auto host = ServiceWorkerServiceHost::getInstance();

    // 1. Let registration be the result of running the Get Registration
    // algorithm passing job’s scope url as the argument.
    auto registration = host->getRegistration(job->scopeURL);

    // 7. Let hasUpdatedResources be false.
    bool hasUpdatedResources = false;

    // 12. Let scopeURL be registration’s scope url.
    String* scopeURL = job->scopeURL;

    // FIXME: simulate resource update for now.
    hasUpdatedResources = true;

    // 10. If hasUpdatedResources is false, then:
    if (!hasUpdatedResources) {
        // 10.1. Invoke Resolve Job Promise with job and registration.
        runResolveJobPromise(job);

        // 10.2. Invoke Finish Job with job and abort these steps.
        finishJob();
        return;
    }

    // 11. Let worker be a new service worker.
    auto worker = new ServiceWorkerData();

    // 12. Set worker’s script url to job’s script url,
    // TODO: worker’s script resource to script, and worker’s type to job’s
    // worker type.
    worker->setScriptURL(job->scriptURL);

    // 16. Invoke Run Service Worker algorithm given worker
    runServiceWorker(worker);

    // 16.2 Else, invoke Install algorithm with job, worker, and registration as
    // its arguments.
    runInstallJob(job, worker, registration);
}

void ServiceWorkerServiceHostJobQueue::runServiceWorker(
    ServiceWorkerData* serviceWorker)
{
    // https://w3c.github.io/ServiceWorker/#run-service-worker
}

void ServiceWorkerServiceHostJobQueue::runInstallJob(
    ServiceWorkerJob* job, ServiceWorkerData* worker,
    ServiceWorkerRegistrationData* registration)
{
    // https://w3c.github.io/ServiceWorker/#install

    // 1. Let installFailed be false.
    bool installFailed = false;

    // 3. Run the Update Registration State algorithm passing registration,
    // "installing" and worker as the arguments.
    runUpdateRegistrationState(registration, "installing", worker);

    // 6. Invoke Resolve Job Promise with job and registration.
    runResolveJobPromise(job);

    // 7. Queue a task to fire an event named updatefound at all the
    // ServiceWorkerRegistration objects for all
    // the service worker clients whose creation URL matches registration’s
    // scope url and all the service workers
    // whose containing service worker registration is registration.

    // 20. Invoke Try Activate with registration.
}

void ServiceWorkerServiceHostJobQueue::runResolveJobPromise(
    ServiceWorkerJob* job)
{
    // https://w3c.github.io/ServiceWorker/#resolve-job-promise-algorithm
    // NOTE: a job should end where it started, swervice worker client.
    ServiceWorkerServiceHost::getInstance()->client()->resolveJobPromise(job);
}

void ServiceWorkerServiceHostJobQueue::runUpdateRegistrationState(
    ServiceWorkerRegistrationData* registration, const char* target,
    ServiceWorkerData* source)
{
    // https://w3c.github.io/ServiceWorker/#update-registration-state-algorithm
    // TODO: for target, use ServiceWorkerRegistrationState instead of char

    // 1. Let registrationObjects be an array containing all the
    // ServiceWorkerRegistration objects associated with registration.

    // 2. If target is "installing", then:
    if (!strncmp(target, "installing", 10)) {
        registration->updateRegistrationState(
            ServiceWorkerRegistrationState::Installing, source);
        // 2.1 Queue a task to set the installing attribute of
        // registrationObject to the ServiceWorker object that represents
        // registration’s installing worker, or null if registration’s
        // installing worker is null.
    } else if (!strncmp(target, "waiting", 10)) {
    } else if (!strncmp(target, "active", 10)) {
    }
}

void ServiceWorkerServiceHostJobQueue::runUnregisterJob(ServiceWorkerJob* job)
{
    // TODO: meet https://w3c.github.io/ServiceWorker/#unregister-algorithm
}

void ServiceWorkerServiceHostJobQueue::finishJob()
{
    // TODO: meet https://w3c.github.io/ServiceWorker/#finish-job-algorithm
    m_jobQueue.pop_front();
    if (!m_jobQueue.empty()) {
        runJob();
    }
}
}

#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
