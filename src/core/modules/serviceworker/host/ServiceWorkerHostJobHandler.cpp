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

#include "core/util/Id.h"
#include "core/modules/serviceworker/ServiceWorkerTypes.h"
#include "core/modules/serviceworker/MessageParam.h"
#include "core/modules/serviceworker/ServiceWorkerJobData.h"
#include "core/modules/serviceworker/ServiceWorkerJob.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/serviceworker/JobQueue.h"
#include "core/modules/serviceworker/host/ServiceWorkerHostJobHandler.h"

#include "core/modules/serviceworker/ServiceWorkerData.h"
#include "core/modules/serviceworker/ServiceWorkerRegistrationData.h"
#include "core/modules/serviceworker/ServiceWorkerProcessInterface.h"
#include "core/modules/serviceworker/host/ServiceWorkerHostProcess.h"

namespace Starfish {

ServiceWorkerHostJobHandler::ServiceWorkerHostJobHandler(
    MessageLoop* messageLoop)
    : m_messageLoop(messageLoop)
{
}

void ServiceWorkerHostJobHandler::scheduleJob(ServiceWorkerJob* job)
{
    // https://w3c.github.io/ServiceWorker/#schedule-job-algorithm
    // 1. Let jobQueue be null.
    JobQueue* jobQueue = nullptr;

    // 2. Let jobScope be job’s scope url, serialized.
    auto jobScope = job->data()->scopeURL;

    // 3. If scope to job queue map[jobScope] does not exist, set scope to job
    // queue map[jobScope] to a new job queue.
    // 4. Set jobQueue to scope to job queue map[jobScope].
    auto scope = m_jobQueueMap.find(jobScope);
    if (scope == m_jobQueueMap.end()) {
        jobQueue = new JobQueue();
        m_jobQueueMap.insert(std::make_pair(jobScope, jobQueue));
    } else {
        jobQueue = scope->second;
    }

    // 5. If jobQueue is empty, then:
    if (jobQueue->size() == 0) {
        // 5.1. Set job’s containing job queue to jobQueue, and enqueue job to
        // jobQueue.
        job->setContainingJobQueue(jobQueue);
        jobQueue->enqueueJob(job);

        // 5.2. Invoke Run Job with jobQueue.
        runJob(jobQueue);
    } else {
        // 6. Else:
    }
}

ServiceWorkerRegistrationData* ServiceWorkerHostJobHandler::getRegistration(
    String* queriedScope)
{
    // https://w3c.github.io/ServiceWorker/#get-registration-algorithm
    auto it = m_registrationMap.find(queriedScope);
    if (it == m_registrationMap.end()) {
        return nullptr;
    }
    return it->second;
}

void ServiceWorkerHostJobHandler::setRegistration(
    String* scope, ServiceWorkerUpdateViaCache updateViaCacheMode)
{
    // https://w3c.github.io/ServiceWorker/#set-registration-algorithm

    // 3. Let registration be a new service worker registration whose scope url
    // is set to scope and update via cache mode is set to updateViaCache.
    auto registration = new ServiceWorkerRegistrationData();

    m_registrationMap[scope] = registration;
}

void ServiceWorkerHostJobHandler::runJob(JobQueue* jobQueue)
{
    // https://w3c.github.io/ServiceWorker/#run-job-algorithm

    // 1. Assert: jobQueue is not empty.
    STARFISH_ASSERT(jobQueue->size() != 0);

    // 2. Queue a task to run these steps in parallel.
    // TODO: remove params. use another function instead
    struct Params : public gc {
        ServiceWorkerHostJobHandler* self;
        ServiceWorkerJob* job;
    };

    auto params = new Params();
    params->self = this;

    // 2.1 Let job be the first item in jobQueue.
    params->job = jobQueue->firstJob();

    queueTask(
        [](size_t handle, void* data) {
            Params* params = static_cast<Params*>(data);
            ServiceWorkerHostJobHandler* self = params->self;
            ServiceWorkerJob* job = params->job;

            // 2.2, 2.3, 2.4
            switch (job->data()->type) {
            case ServiceWorkerJobType::Register:
                self->registerServiceWorker(job);
                break;
            case ServiceWorkerJobType::Update:
                self->update(job);
                break;
            case ServiceWorkerJobType::Unregister:
                self->unregisterServiceWorker(job);
                break;
            default:
                break;
            }
        },
        params);
}

void ServiceWorkerHostJobHandler::queueTask(void (*fn)(size_t, void*),
                                            void* data)
{
    m_messageLoop->addIdler(nullptr, fn, data);
}

void ServiceWorkerHostJobHandler::registerServiceWorker(ServiceWorkerJob* job)
{
    // https://w3c.github.io/ServiceWorker/#register-algorithm

    // 4. Let registration be the result of running the Get Registration
    // algorithm passing job’s scope url as the argument.
    auto registration = getRegistration(job->data()->scopeURL);

    if (registration) {
        // 5. If registration is not null, then:
    } else {
        // 6. Invoke Set Registration algorithm with job’s scope url and job’s
        // update via cache mode.
        setRegistration(job->data()->scopeURL, job->data()->updateViaCacheMode);
    }

    // 7. Invoke Update algorithm passing job as the argument
    update(job);
}

void ServiceWorkerHostJobHandler::update(ServiceWorkerJob* job)
{
    // https://w3c.github.io/ServiceWorker/#update-algorithm
    // 1. Let registration be the result of running the Get Registration
    // algorithm passing job’s scope url as the argument.
    auto registration = getRegistration(job->data()->scopeURL);

    // 7. Let hasUpdatedResources be false.
    bool hasUpdatedResources = false;

    // 12. Let scopeURL be registration’s scope url.
    String* scopeURL = job->data()->scopeURL;

    // FIXME: simulate resource update for now.
    hasUpdatedResources = true;

    // 10. If hasUpdatedResources is false, then:
    if (!hasUpdatedResources) {
        // 10.1. Invoke Resolve Job Promise with job and registration.
        resolveJobPromise(job, registration);

        // 10.2. Invoke Finish Job with job and abort these steps.
        finishJob(job);
        return;
    }

    // 11. Let worker be a new service worker.
    auto worker = new ServiceWorkerData();

    // 12. Set worker’s script url to job’s script url,
    // TODO: worker’s script resource to script, and worker’s type to job’s
    // worker type.
    worker->scriptURL = job->data()->scriptURL;

    // 16. Invoke Run Service Worker algorithm given worker
    runServiceWorker(worker);

    // 16.2 Else, invoke Install algorithm with job, worker, and registration as
    // its arguments.
    install(job, worker, registration);
}

void ServiceWorkerHostJobHandler::runServiceWorker(
    ServiceWorkerData* serviceWorker)
{
    // https://w3c.github.io/ServiceWorker/#run-service-worker
}

void ServiceWorkerHostJobHandler::install(
    ServiceWorkerJob* job, ServiceWorkerData* worker,
    ServiceWorkerRegistrationData* registration)
{
    // https://w3c.github.io/ServiceWorker/#install

    // 1. Let installFailed be false.
    bool installFailed = false;

    // 3. Run the Update Registration State algorithm passing registration,
    // "installing" and worker as the arguments.
    updateRegistrationState(registration, "installing", worker);

    // 6. Invoke Resolve Job Promise with job and registration.
    resolveJobPromise(job, registration);

    // 7. Queue a task to fire an event named updatefound at all the
    // ServiceWorkerRegistration objects for all
    // the service worker clients whose creation URL matches registration’s
    // scope url and all the service workers
    // whose containing service worker registration is registration.

    // 20. Invoke Try Activate with registration.
}

void ServiceWorkerHostJobHandler::resolveJobPromise(
    ServiceWorkerJob* job, ServiceWorkerRegistrationData* registration)
{
    // https://w3c.github.io/ServiceWorker/#resolve-job-promise-algorithm
    job->hostConnection()->resolveJobPromise(job, registration);
}

void ServiceWorkerHostJobHandler::updateRegistrationState(
    ServiceWorkerRegistrationData* registration, const char* target,
    ServiceWorkerData* source)
{
    // https://w3c.github.io/ServiceWorker/#update-registration-state-algorithm
    // TODO: for target, use ServiceWorkerRegistrationState instead of char

    // 1. Let registrationObjects be an array containing all the
    // ServiceWorkerRegistration objects associated with registration.

    // 2. If target is "installing", then:
    if (!strncmp(target, "installing", 10)) {
        // 2.1. Set registration’s installing worker to source.
        registration->installingWorker = source;
        // 2.2 For each registrationObject in registrationObjects:
        // 2.2.1 Queue a task to set the installing attribute of
        // registrationObject to the ServiceWorker object that represents
        // registration’s installing worker, or null if registration’s
        // installing worker is null.
    } else if (!strncmp(target, "waiting", 10)) {
    } else if (!strncmp(target, "active", 10)) {
    }
}

void ServiceWorkerHostJobHandler::unregisterServiceWorker(ServiceWorkerJob* job)
{
    // TODO: meet https://w3c.github.io/ServiceWorker/#unregister-algorithm
}

void ServiceWorkerHostJobHandler::finishJob(ServiceWorkerJob* job)
{
    // https://w3c.github.io/ServiceWorker/#finish-job-algorithm

    // 1. Let jobQueue be job’s containing job queue.
    auto jobQueue = job->containingJobQueue();

    // 2. Assert: the first item in jobQueue is job.
    STARFISH_ASSERT(jobQueue->firstJob() == job);

    // 3. Dequeue from jobQueue.
    // 4. If jobQueue is not empty, invoke Run Job with jobQueue.
    jobQueue->dequeueJob();
    if (!jobQueue->empty()) {
        runJob(jobQueue);
    }
}
} // namespace Starfish

#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
