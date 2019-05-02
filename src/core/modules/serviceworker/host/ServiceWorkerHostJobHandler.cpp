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
#include "core/util/Archivable.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "platform/loader/ResourceURL.h"
#include "core/modules/serviceworker/ProgramOptions.h"
#include "core/modules/serviceworker/WorkerConfig.h"

#include "core/modules/serviceworker/ServiceWorkerTypes.h"
#include "core/modules/serviceworker/JobQueue.h"
#include "core/modules/serviceworker/ServiceWorkerData.h"
#include "core/modules/serviceworker/ServiceWorkerJobData.h"
#include "core/modules/serviceworker/ServiceWorkerRegistrationData.h"
#include "core/modules/serviceworker/ServiceWorkerJob.h"
#include "core/modules/serviceworker/ServiceWorkerProcessInterface.h"
#include "core/modules/serviceworker/host/ServiceWorkerHostJobHandler.h"

namespace Starfish {

ServiceWorkerHostJobHandler::ServiceWorkerHostJobHandler(
    MessageLoop* messageLoop)
    : m_messageLoop(messageLoop)
{
}

void ServiceWorkerHostJobHandler::scheduleJob(ServiceWorkerJob* job)
{
    STARFISH_ASSERT(job != nullptr);
    SWHOST_LOG_IF_ALLOWED(1, "0: type: %d\n",
                          toUnderlyingType(job->data()->type));

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
        STARFISH_ASSERT(jobQueue != nullptr);
        m_jobQueueMap.insert(std::make_pair(jobScope, jobQueue));
    } else {
        jobQueue = scope->second;
    }

    // 5. If jobQueue is empty, then:
    SWHOST_LOG_IF_ALLOWED(1, "5: is jobQueue empty? (%s)\n",
                          jobQueue->empty() ? "true" : "false");

    if (jobQueue->empty()) {
        // 5.1. Set job’s containing job queue to jobQueue, and enqueue job to
        // jobQueue.
        job->setContainingJobQueue(jobQueue);
        jobQueue->enqueueJob(job);

        // 5.2. Invoke Run Job with jobQueue.
        runJob(jobQueue);
    } else {
        // 6. Else:
        // 6.1 Let lastJob be the element at the back of jobQueue.

        // 6.2 If job is equivalent to lastJob and lastJob’s job promise has not
        // settled, append job to lastJob’s list of equivalent jobs.

        // 6.3 Else, set job’s containing job queue to jobQueue, and enqueue job
        // to jobQueue.
    }
}

NULLABLE ServiceWorkerRegistrationData*
ServiceWorkerHostJobHandler::getRegistration(String* queriedScope)
{
    STARFISH_ASSERT(queriedScope != nullptr);

    // https://w3c.github.io/ServiceWorker/#get-registration-algorithm

    // NOTE: Using GCMap.find doesn't work well, so we use its iterator.
    // e.g) auto it = m_scopeToRegistrationMap.find(queriedScope);
    // if (it == m_scopeToRegistrationMap.end()) {
    // ...
    // }

    for (const auto& pair : m_scopeToRegistrationMap) {
        if (pair.first->equals(queriedScope)) {
            SWHOST_LOG_IF_ALLOWED(1, "1: %s (Found)\n", CSTR(queriedScope));
            return pair.second;
        }
    }

    SWHOST_LOG_IF_ALLOWED(1, "1: %s (Not Found)\n", CSTR(queriedScope));
    return nullptr;
}

void ServiceWorkerHostJobHandler::setRegistration(
    String* scope, ServiceWorkerUpdateViaCache updateViaCache)
{
    STARFISH_ASSERT(scope != nullptr);

    SWHOST_LOG_IF_ALLOWED(1, "0: %s\n", CSTR(scope));

    // https://w3c.github.io/ServiceWorker/#set-registration-algorithm

    // 3. Let registration be a new service worker registration whose scope url
    // is set to scope and update via cache mode is set to updateViaCache.
    auto registration = new ServiceWorkerRegistrationData();
    registration->scope = scope;
    registration->updateViaCache = updateViaCache;

    m_scopeToRegistrationMap[scope] = registration;
}

void ServiceWorkerHostJobHandler::runJob(JobQueue* jobQueue)
{
    // https://w3c.github.io/ServiceWorker/#run-job-algorithm

    // 1. Assert: jobQueue is not empty.
    STARFISH_ASSERT(jobQueue != nullptr);
    STARFISH_ASSERT(jobQueue->size() != 0);

    // 2. Queue a task to run these steps in parallel.
    // TODO: remove params. use another function instead
    struct Params : public gc {
        ServiceWorkerHostJobHandler* self;
        ServiceWorkerJob* job;
    };

    auto params = new Params();

    STARFISH_ASSERT(params != nullptr);

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
    STARFISH_ASSERT(job != nullptr);

    // https://w3c.github.io/ServiceWorker/#register-algorithm

    // 4. Let registration be the result of running the Get Registration
    // algorithm passing job’s scope url as the argument.
    auto registration = getRegistration(job->data()->scopeURL);

    if (registration != nullptr) {
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
    STARFISH_ASSERT(job != nullptr);

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
    if (hasUpdatedResources == false) {
        // 10.1. Invoke Resolve Job Promise with job and registration.
        resolveJobPromise(job, registration);

        // 10.2. Invoke Finish Job with job and abort these steps.
        finishJob(job);
        return;
    }

    // 11. Let worker be a new service worker.
    auto worker = new ServiceWorkerData();

    STARFISH_ASSERT(worker != nullptr);

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
    STARFISH_ASSERT(serviceWorker != nullptr);
    // https://w3c.github.io/ServiceWorker/#run-service-worker
}

void ServiceWorkerHostJobHandler::install(
    ServiceWorkerJob* job, ServiceWorkerData* worker,
    ServiceWorkerRegistrationData* registration)
{
    STARFISH_ASSERT(job != nullptr);
    STARFISH_ASSERT(worker != nullptr);
    STARFISH_ASSERT(registration != nullptr);
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

    // 21. Invoke Finish Job with job.
    finishJob(job);

    // 23. Invoke Try Activate with registration.
}

void ServiceWorkerHostJobHandler::resolveJobPromise(
    ServiceWorkerJob* job, NULLABLE ServiceWorkerRegistrationData* registration)
{
    STARFISH_ASSERT(job != nullptr);
    // https://w3c.github.io/ServiceWorker/#resolve-job-promise-algorithm
    job->hostConnection()->resolveJobPromise(job, registration);
}

void ServiceWorkerHostJobHandler::updateRegistrationState(
    ServiceWorkerRegistrationData* registration, const char* target,
    ServiceWorkerData* source)
{
    STARFISH_ASSERT(registration != nullptr);
    STARFISH_ASSERT(target != nullptr);
    STARFISH_ASSERT(source != nullptr);
    // https://w3c.github.io/ServiceWorker/#update-registration-state-algorithm
    // TODO: for target, use ServiceWorkerRegistrationState instead of char

    // 1. Let registrationObjects be an array containing all the
    // ServiceWorkerRegistration objects associated with registration.

    // 2. If target is "installing", then:
    if (strncmp(target, "installing", 10) == 0) {
        // 2.1. Set registration’s installing worker to source.
        registration->installingWorker = source;
        // 2.2 For each registrationObject in registrationObjects:
        // 2.2.1 Queue a task to set the installing attribute of
        // registrationObject to the ServiceWorker object that represents
        // registration’s installing worker, or null if registration’s
        // installing worker is null.
    } else if (strncmp(target, "waiting", 10) == 0) {
    } else if (strncmp(target, "active", 10) == 0) {
    }
}

void ServiceWorkerHostJobHandler::unregisterServiceWorker(ServiceWorkerJob* job)
{
    STARFISH_ASSERT(job != nullptr);
    SWHOST_LOG_IF_ALLOWED(1, "0: type: %d\n",
                          toUnderlyingType(job->data()->type));

    // TODO: meet https://w3c.github.io/ServiceWorker/#unregister-algorithm
    auto registration = getRegistration(job->data()->scopeURL);
    resolveJobPromise(job, registration);
}

void ServiceWorkerHostJobHandler::finishJob(ServiceWorkerJob* job)
{
    STARFISH_ASSERT(job != nullptr);
    SWHOST_LOG_IF_ALLOWED(1, "0: type: %d\n",
                          toUnderlyingType(job->data()->type));

    // https://w3c.github.io/ServiceWorker/#finish-job-algorithm

    // 1. Let jobQueue be job’s containing job queue.
    auto jobQueue = job->containingJobQueue();

    // 2. Assert: the first item in jobQueue is job.
    STARFISH_ASSERT(jobQueue != nullptr);
    STARFISH_ASSERT(jobQueue->firstJob() == job);

    // 3. Dequeue from jobQueue.
    // 4. If jobQueue is not empty, invoke Run Job with jobQueue.
    jobQueue->dequeueJob();
    if (jobQueue->empty() == false) {
        runJob(jobQueue);
    }
}

NULLABLE ServiceWorkerRegistrationData*
ServiceWorkerHostJobHandler::matchRegistration(ServiceWorkerRequest* request,
                                               String* clientURLString)
{
    STARFISH_ASSERT(request != nullptr);
    STARFISH_ASSERT(clientURLString != nullptr);

    SWHOST_LOG_IF_ALLOWED(1, "0: %s\n", CSTR(clientURLString));

    // https://w3c.github.io/ServiceWorker/#match-service-worker-registration

    // 1. Run the following steps atomically.
    // 2. Let clientURLString be serialized clientURL.
    // NOTE: consider using URLData instead of serialized clientURL,
    // clientURLString.

    // 3. Let matchingScopeString be the empty string.
    String* matchingScopeString = String::emptyString;

    // 4. Let scopeStringSet be the result of getting the keys from `scope to
    // registration map`.
    for (const auto& pair : m_scopeToRegistrationMap) {
        ServiceWorkerRegistrationKey selectedRegistrationKey = pair.first;

        SWHOST_LOG_IF_ALLOWED(1, "4: %s\n", CSTR(selectedRegistrationKey));
        if (clientURLString->startsWith(selectedRegistrationKey, false) ==
            false) {
            continue;
        }

        // 5. Set matchingScopeString to the longest value in scopeStringSet
        // which the value of clientURLString starts with, if it exists.
        if (matchingScopeString->length() < selectedRegistrationKey->length()) {
            SWHOST_LOG_IF_ALLOWED(1, "5: %s\n", CSTR(selectedRegistrationKey));
            matchingScopeString = selectedRegistrationKey;
        }
    }

    // 6. Let matchingScope be null.
    String* matchingScope = nullptr;

    NULLABLE ServiceWorkerRegistrationData* registration = nullptr;

    // 7. If matchingScopeString is not the empty string, then:
    if (matchingScopeString->isEmpty() == false) {
        // 7.1. Set matchingScope to the result of parsing matchingScopeString.
        auto matchingScope = new ResourceURL(matchingScopeString);

        // 7.2. Assert: matchingScope’s origin and clientURL’s origin are same
        // origin.
        auto clientURL = new ResourceURL(clientURLString);
        STARFISH_ASSERT(matchingScope->origin()->equals(clientURL->origin()));

        // 8. Let registration be the result of running Get Registration
        // algorithm passing matchingScope as the argument.
        registration = getRegistration(matchingScopeString);
    }

    // 9. If registration is not null and registration’s uninstalling flag is
    // set, return null.
    if ((registration != nullptr) && registration->isUninstalling()) {
        SWHOST_LOG_IF_ALLOWED(1, "9: done\n");
        return nullptr;
    }

    // 10. Return registration.
    return registration;
}

} // namespace Starfish

#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
