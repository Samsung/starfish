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

#ifdef STARFISH_ENABLE_SERVICE_WORKER

#include "StarfishConfig.h"

#include "core/util/Id.h"
#include "core/util/Archivable.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "platform/loader/ResourceURL.h"
#include "core/dom/DOMException.h"
#include "core/modules/threading/IRunnable.h"
#include "core/modules/serviceworker/IORunnable.h"
#include "core/modules/serviceworker/Connection.h"
#include "core/modules/resource_request/ResourceRequest.h"
#include "core/dom/ExecutionContext.h"
#include "core/modules/worker/host/WorkerGlobalScope.h"
#include "platform/network/http/HTTPStatus.h"

#include "core/modules/serviceworker/WorkerConfig.h"
#include "core/modules/serviceworker/ServiceWorkerTypes.h"
#include "core/modules/serviceworker/ErrorData.h"
#include "core/modules/serviceworker/MessageServiceWorker.h"
#include "core/modules/serviceworker/JobQueue.h"
#include "core/modules/serviceworker/ServiceWorkerData.h"
#include "core/modules/serviceworker/ServiceWorkerJobData.h"
#include "core/modules/serviceworker/ServiceWorkerRegistrationData.h"
#include "core/modules/serviceworker/ServiceWorkerJob.h"
#include "core/modules/serviceworker/ConnectionInterface.h"
#include "core/modules/serviceworker/ServiceWorkerAgent.h"
#include "core/modules/serviceworker/host/ServiceWorkerServerInterface.h"
#include "core/modules/serviceworker/host/ServiceWorkerHostJobHandler.h"

namespace Starfish {

class FetchClient : public ResourceRequestClient {
public:
    FetchClient(ServiceWorkerHostJobHandler* jobHandler, ServiceWorkerJob* job,
                Nullable<ServiceWorkerData*> data,
                ServiceWorkerRegistrationData* registration)
        : m_jobHandler(jobHandler)
        , m_job(job)
        , m_serviveWorker(data)
        , m_registration(registration)
    {
        STARFISH_ASSERT(jobHandler != nullptr);
        STARFISH_ASSERT(job != nullptr);
        STARFISH_ASSERT(registration != nullptr);
    }

    void onProgressEvent(ResourceRequest* request, bool isExplicitAction)
    {
        TRACE_SCOPE(HOST);
        STARFISH_ASSERT(request != nullptr);
        ProgressState progState = request->progressState();
    }

    void onReadyStateChange(NULLABLE ResourceRequest* request,
                            bool fromExplicit)
    {
        TRACE_SCOPE(HOST);
        if (request == nullptr) {
            // TODO: STARFISH_ASSERT(request != nullptr);
            continuePendingUpdateJob(nullptr);
            return;
        }

        if (request->readyState() == ReadyState::Done) {
            if ((request->isError() == false) &&
                (request->status() == HTTPStatusCode::HTTP_STATUS_OK)) {
                auto& response = request->response();
                String* text =
                    String::fromUTF8(response.data(), response.size());

                response.clear();
                response.shrink_to_fit();
            }
        }
    }

    void continuePendingUpdateJob(NULLABLE ResourceRequest* request)
    {
        TRACE_SCOPE(HOST);
        auto job = m_job;
        auto registration = m_registration;
        auto newestWorker = m_serviveWorker;

        // 5. Let httpsState be "none".
        String* httpsState = String::createASCIIString("none");

        // 6. Let referrerPolicy be the empty string.
        String* referrerPolicy = String::emptyString;

        // 7. Let hasUpdatedResources be false.
        bool hasUpdatedResources = false;

        // 8. Let updatedResourceMap be an ordered map where the keys are URLs
        // and the values are responses.
        auto updatedResourceMap = new ScriptResourceMap_t;

        // NOTE: the steps, 9.1 - 9.6, are done in `update`

        // NOTE: 9.7 verify a MIME type from the response’s header list.

        // NOTE: 9.8 - 9.14 verify Service-Worker-Allowed header in Appendix B:
        // Extended HTTP headers.

        // NOTE: 9.15 - 9.18 verify scopeString with scopeURL and maxScopeString

        // 9.22 Let map be newestWorker’s script resource map if newestWorker is
        // not null, and null otherwise.
        ScriptResourceMap_t* map = nullptr;
        if (newestWorker) {
            map = newestWorker->urlToScriptResourceMap();
        }

        // TODO (b): 9.23 If a) map is null or b) map[url]'s body is not
        // byte-for-byte identical with response’s body, set hasUpdatedResources
        // to true.
        if (map == nullptr) {
            hasUpdatedResources = true;
        } else {
            // 9.24 Else if newestWorker’s classic scripts imported flag is set
        }

        // Else, continue the rest of these steps after the algorithm’s
        // asynchronous completion, with `script` being the asynchronous
        // completion value.

        // NOTE: we currenlty use response's body for script.
        // TODO: create a struct, script resource (a script).
        String* script = String::emptyString;

        // 10. If hasUpdatedResources is false, then:
        if (hasUpdatedResources == false) {
            // 10.1. Invoke Resolve Job Promise with job and registration.
            m_jobHandler->resolveJobPromise(job, registration);
            // 10.2. Invoke Finish Job with job and abort these steps.
            m_jobHandler->finishJob(job);
            return;
        }

        // 11. Let worker be a new service worker.
        auto worker = new ServiceWorkerData();

        worker->registrationId = registration->id;

        // 12. Set 1) worker’s script url to job’s script url, 2) worker’s
        // script resource to script, 3) worker’s type to job’s worker type, and
        // 4) worker’s script resource map to updatedResourceMap.
        worker->scriptURL = job->data()->scriptURL;
        worker->setType(job->data()->workerType);
        worker->setUrlToScriptResourceMap(updatedResourceMap);
        worker->scriptResource().script = script;

        // 13. Append url to worker’s set of used scripts.
        // 14. Set worker’s script resource’s HTTPS state to httpsState.
        worker->scriptResource().httpsState = httpsState;
        // 15. Set worker’s script resource’s referrer policy to referrerPolicy.
        worker->scriptResource().referrerPolicy = referrerPolicy;

        // 16. Invoke Run Service Worker algorithm given worker
        m_jobHandler->runServiceWorker(worker);
        // TODO: 16.1 If evaluationStatus is an abrupt completion or
        // evaluationStatus.[[Value]] is empty, then:

        // 16.2 Else, invoke Install algorithm with job, worker, and
        // registration as its arguments.
        m_jobHandler->install(job, worker, registration);
    }

private:
    ServiceWorkerHostJobHandler* m_jobHandler;
    ServiceWorkerJob* m_job;
    Nullable<ServiceWorkerData*> m_serviveWorker;
    ServiceWorkerRegistrationData* m_registration;
};

ServiceWorkerHostJobHandler::ServiceWorkerHostJobHandler(
    MessageLoop* messageLoop, ServiceWorkerServerInterface* swserver)
    : m_messageLoop(messageLoop)
    , m_SWServer(swserver)
{
    STARFISH_ASSERT(messageLoop != nullptr);
    STARFISH_ASSERT(swserver != nullptr);
}

void ServiceWorkerHostJobHandler::scheduleJob(ServiceWorkerJob* job)
{
    TRACE_SCOPE(HOST);
    STARFISH_ASSERT(job != nullptr);
    TRACE(HOST, "0: type: %d", toUnderlyingType(job->data()->type));

    // https://w3c.github.io/ServiceWorker/#schedule-job-algorithm
    // 1. Let jobQueue be null.
    JobQueue* jobQueue = nullptr;

    // 2. Let jobScope be job’s scope url, serialized.
    auto jobScope = job->data()->scopeURL;

    // 3. If scope to job queue map[jobScope] does not exist, set scope to job
    // queue map[jobScope] to a new job queue.
    // 4. Set jobQueue to scope to job queue map[jobScope].
    auto scope = m_scopeToJobQueueMap.find(jobScope);
    if (scope == m_scopeToJobQueueMap.end()) {
        jobQueue = new JobQueue();
        STARFISH_ASSERT(jobQueue != nullptr);
        m_scopeToJobQueueMap.insert(std::make_pair(jobScope, jobQueue));
    } else {
        jobQueue = scope->second;
    }

    // 5. If jobQueue is empty, then:
    TRACE(HOST, "5: is jobQueue empty? (%s)",
          jobQueue->empty() ? "true" : "false");

    if (jobQueue->empty()) {
        // 5.1. Set job’s containing job queue to jobQueue, and enqueue job to
        // jobQueue.
        job->setContainingJobQueue(jobQueue);
        jobQueue->enqueueJob(job);

        // 5.2. Invoke Run Job with jobQueue.
        runJob(jobQueue);
    } else {
        STARFISH_UNIMPLEMENTED();

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
    TRACE_SCOPE(HOST);
    STARFISH_ASSERT(queriedScope != nullptr);

    // https://w3c.github.io/ServiceWorker/#get-registration-algorithm

    // NOTE: Using GCMap.find doesn't work well, so we use its iterator.
    // e.g) auto it = m_scopeToRegistrationMap.find(queriedScope);
    // if (it == m_scopeToRegistrationMap.end()) {
    // ...
    // }

    for (const auto& pair : m_scopeToRegistrationMap) {
        if (pair.first->equals(queriedScope)) {
            TRACE(HOST, "1: %s (Found)", CSTR(queriedScope));
            return pair.second;
        }
    }

    TRACE(HOST, "1: %s (Not Found)", CSTR(queriedScope));
    return nullptr;
}

NULLABLE ServiceWorkerRegistrationData*
ServiceWorkerHostJobHandler::getRegistration(
    ServiceWorkerRegistrationId registrationId)
{
    TRACE_SCOPE(HOST);
    for (const auto& pair : m_scopeToRegistrationMap) {
        auto registration = pair.second;
        if (registration->id == registrationId) {
            TRACE(HOST, "1: %s (Found)", registrationId.toString().c_str());
            return registration;
        }
    }

    TRACE(HOST, "1: %s (Not Found)", registrationId.toString().c_str());
    return nullptr;
}

void ServiceWorkerHostJobHandler::setRegistration(
    String* scope, ServiceWorkerUpdateViaCache updateViaCache)
{
    TRACE_SCOPE(HOST);
    STARFISH_ASSERT(scope != nullptr);

    TRACE(HOST, "0: %s", CSTR(scope));

    // https://w3c.github.io/ServiceWorker/#set-registration-algorithm

    // 3. Let registration be a new service worker registration whose scope url
    // is set to scope and update via cache mode is set to updateViaCache.
    auto registration = new ServiceWorkerRegistrationData();
    registration->id = ServiceWorkerRegistrationId::generate();
    registration->scope = scope;
    registration->updateViaCache = updateViaCache;

    auto iter = m_scopeToRegistrationMap.find(scope);
    if (iter == m_scopeToRegistrationMap.end()) {
        m_scopeToRegistrationMap.insert(std::make_pair(scope, registration));
    } else {
        iter->second = registration;
    }
}

void ServiceWorkerHostJobHandler::runJob(JobQueue* jobQueue)
{
    TRACE_SCOPE(HOST);
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
                self->m_SWServer->tryTerminate();
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
    TRACE_SCOPE(HOST);
    m_messageLoop->addIdler(nullptr, fn, data);
}

void ServiceWorkerHostJobHandler::registerServiceWorker(ServiceWorkerJob* job)
{
    TRACE_SCOPE(HOST);
    STARFISH_ASSERT(job != nullptr);

    // https://w3c.github.io/ServiceWorker/#register-algorithm

    // 4. Let registration be the result of running the Get Registration
    // algorithm passing job’s scope url as the argument.
    NULLABLE auto registration = getRegistration(job->data()->scopeURL);

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

Nullable<ServiceWorkerData*> ServiceWorkerHostJobHandler::getNewestWorker(
    ServiceWorkerRegistrationData* registration)
{
    TRACE_SCOPE(HOST);
    STARFISH_ASSERT(registration != nullptr);

    // https://w3c.github.io/ServiceWorker/#get-newest-worker

    // 1. Run the following steps atomically.
    // 2. Let newestWorker be null.
    ServiceWorkerData* newestWorker = nullptr;

    if (registration->installingWorker()) {
        // 3. If registration’s installing worker is not null, set newestWorker
        // to registration’s installing worker.
        newestWorker = registration->installingWorker().value();
    } else if (registration->waitingWorker) {
        // 4. Else if registration’s waiting worker is not null, set
        // newestWorker to registration’s waiting worker.
        newestWorker = registration->waitingWorker.value();
    } else if (registration->activeWorker) {
        // 5. Else if registration’s active worker is not null, set newestWorker
        // to registration’s active worker.
        newestWorker = registration->activeWorker.value();
    }

    // 6. Return newestWorker.
    return newestWorker;
}

void ServiceWorkerHostJobHandler::update(ServiceWorkerJob* job)
{
    TRACE_SCOPE(HOST);
    STARFISH_ASSERT(job != nullptr);

    // https://w3c.github.io/ServiceWorker/#update-algorithm
    // 1. Let registration be the result of running the Get Registration
    // algorithm passing job’s scope url as the argument.
    NULLABLE auto registration = getRegistration(job->data()->scopeURL);

    // 2. If registration is null or registration’s uninstalling flag is set,
    if (registration == nullptr) {
        //  2.1 Invoke Reject Job Promise with job and TypeError.
        rejectJobPromise(job, new ErrorData(ExceptionCode::SCRIPT_TYPE_ERR,
                                            "Cannot update a null/nonexistent "
                                            "service worker registration"));

        //  2.2 Invoke  Finish Job with job and abort these steps.
        finishJob(job);
        return;
    }

    // 3. Let newestWorker be the result of running `Get Newest Worker
    // algorithm` passing registration as the argument.
    auto newestWorker = getNewestWorker(registration);

    // 4. If job’s job type is update, and newestWorker is not null and its
    // script url does not equal job’s script url, then:
    if ((job->data()->type == ServiceWorkerJobType::Update) &&
        newestWorker.hasValue() &&
        (newestWorker->scriptURL->equals(job->data()->scriptURL) == false)) {
        // 4.1 Invoke Reject Job Promise with job and TypeError.
        rejectJobPromise(
            job, new ErrorData(
                     ExceptionCode::SCRIPT_TYPE_ERR,
                     "Cannot update a service worker with a requested script "
                     "URL whose newest worker has a different script URL"));
        // 4.2 Invoke Finish Job with job and abort these steps.
        finishJob(job);
        return;
    }

    // NOTE: 5-8 is handled in `continuePendingUpdateJob`

    // 9. Switching on job’s worker type, run these substeps with the
    // following options:

    // 9.6 Fetch request, and asynchronously wait to run the remaining
    // steps as part of fetch’s process response for the response response.

    RequestData* requestData = new RequestData();
    requestData->m_url = new ResourceURL(job->data()->scriptURL);
    requestData->m_destination = RequestDestination::Script;
    requestData->m_syncLevel = RequestSyncLevel::NeverSync;

    FetchClient* client =
        new FetchClient(this, job, newestWorker, registration);

    // NOTE: simulating `script fetch` is done with invoking onReadyStateChange.
    // TODOE: fetch a script url using ResourceRequest given the above
    // requestData.
    client->onReadyStateChange(nullptr, true);
}

void ServiceWorkerHostJobHandler::runServiceWorker(
    ServiceWorkerData* serviceWorker)
{
    TRACE_SCOPE(HOST);
    STARFISH_ASSERT(serviceWorker != nullptr);
    // https://w3c.github.io/ServiceWorker/#run-service-worker
    // TODO: 1. Let script be serviceWorker’s script resource.

    // 2. Assert: script is not null.

    // TODO: 3. If serviceWorker is already running, this algorithm must have
    // been invoked previously. If callbackSteps is provided, run them with the
    // same value as the previous time, and abort these steps.

    // 4. Create a separate parallel execution environment (i.e. a separate
    // thread or process or equivalent construct), and run the following
    // substeps in that context:

    auto startServiceWorker = [](size_t handle, void* data) {
        ServiceWorkerAgent::instance()->runServiceWorker(
            castTo<ServiceWorkerData*>(data));
    };

    m_messageLoop->addIdler(nullptr, startServiceWorker, serviceWorker);
}

void ServiceWorkerHostJobHandler::install(
    ServiceWorkerJob* job, ServiceWorkerData* worker,
    ServiceWorkerRegistrationData* registration)
{
    TRACE_SCOPE(HOST);
    STARFISH_ASSERT(job != nullptr);
    STARFISH_ASSERT(worker != nullptr);
    STARFISH_ASSERT(registration != nullptr);
    // https://w3c.github.io/ServiceWorker/#install

    // 1. Let installFailed be false.
    bool installFailed = false;

    // 3. Run the Update Registration State algorithm passing registration,
    // "installing" and worker as the arguments.
    updateRegistrationState(registration,
                            ServiceWorkerRegistrationState::Installing, worker);

    // 4. Run the Update Worker State algorithm passing registration’s
    // installing worker and installing as the arguments.
    updateWorkerState(worker, ServiceWorkerState::Installing);

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
    TRACE_SCOPE(HOST);
    STARFISH_ASSERT(job != nullptr);
    STARFISH_ASSERT(job->hostConnection() != nullptr);

    // https://w3c.github.io/ServiceWorker/#resolve-job-promise-algorithm
    // is implemented on ServiceWorkerContainer.
    job->hostConnection()->resolveJobPromise(job, registration);
}

bool ServiceWorkerHostJobHandler::tryClearRegistration(
    ServiceWorkerRegistrationData* registration)
{
    TRACE_SCOPE(HOST);
    STARFISH_ASSERT(registration != nullptr);
    // https://w3c.github.io/ServiceWorker/#try-clear-registration-algorithm

    // 1. Invoke `Clear Registration` with registration if no service worker
    // client is using registration and all of the following conditions are
    // true:
    for (const auto& pair : m_clientIdToRegistrationIdMap) {
        if (registration->id == pair.second) {
            return false;
        }
    }

    // 1.1 registration’s installing worker is null or the result of running
    // Service Worker Has No Pending Events with registration’s installing
    // worker is true.
    if ((registration->installingWorker() != nullptr) &&
        (registration->installingWorker()->hasPendingEvents() == true)) {
        return false;
    }

    // 1.2 registration’s waiting worker is null or the result of running
    // Service Worker Has No Pending Events with registration’s waiting
    // worker is true.
    if ((registration->waitingWorker != nullptr) &&
        (registration->waitingWorker->hasPendingEvents() == true)) {
        return false;
    }

    // 1.3 registration’s active worker is null or the result of running
    // Service Worker Has No Pending Events with registration’s active
    // worker is true.
    if ((registration->activeWorker != nullptr) &&
        (registration->activeWorker->hasPendingEvents() == true)) {
        return false;
    }

    clearRegistration(registration);

    return true;
}

void ServiceWorkerHostJobHandler::clearRegistration(
    ServiceWorkerRegistrationData* registration)
{
    TRACE_SCOPE(HOST);
    STARFISH_ASSERT(registration != nullptr);
    // https://w3c.github.io/ServiceWorker/#clear-registration

    // 1. Run the following steps atomically.
    // 2. If registration’s installing worker is not null, then:
    if (registration->installingWorker()) {
        // 2.1. Terminate registration’s installing worker.
        terminateServiceWorker(registration->installingWorker().value());

        // 2.2. Run the `Update Worker State` algorithm passing registration’s
        // installing worker and redundant as the arguments.
        updateWorkerState(registration->installingWorker().value(),
                          ServiceWorkerState::Redundant);

        // 2.3 Run the Update Registration State algorithm passing registration,
        // "installing" and null as the arguments.
        updateRegistrationState(
            registration, ServiceWorkerRegistrationState::Installing, nullptr);
    }

    // 3. If registration’s waiting worker is not null, then:
    if (registration->waitingWorker) {
        // 3.1 Terminate registration’s waiting worker.
        terminateServiceWorker(registration->waitingWorker.value());

        // 3.2 Run the Update Worker State algorithm passing registration’s
        // waiting worker and redundant as the arguments.
        updateWorkerState(registration->waitingWorker.value(),
                          ServiceWorkerState::Redundant);

        // 3.3 Run the Update Registration State algorithm passing registration,
        // "waiting" and null as the arguments.
        updateRegistrationState(
            registration, ServiceWorkerRegistrationState::Waiting, nullptr);
    }

    // 4. If registration’s active worker is not null, then:
    if (registration->activeWorker) {
        // 4.1 Terminate registration’s active worker.
        terminateServiceWorker(registration->activeWorker.value());

        // 4.2 Run the Update Worker State algorithm passing registration’s
        // active worker and redundant as the arguments.
        updateWorkerState(registration->activeWorker.value(),
                          ServiceWorkerState::Redundant);

        // 4.3 Run the Update Registration State algorithm passing registration,
        // "active" and null as the arguments.
        updateRegistrationState(
            registration, ServiceWorkerRegistrationState::Active, nullptr);
    }

    // 5. Let scopeString be registration’s serialized scope url.
    // 6. Remove scope to registration map[scopeString].
    m_scopeToRegistrationMap.erase(registration->scope);
}

void ServiceWorkerHostJobHandler::terminateServiceWorker(
    ServiceWorkerData* serviceWorker)
{
    TRACE_SCOPE(HOST);
    STARFISH_ASSERT(serviceWorker != nullptr);

    // https://w3c.github.io/ServiceWorker/#terminate-service-worker-algorithm

    // 1. If serviceWorker is not running, abort these steps.
    if (serviceWorker->runningState() != ServiceWorkerRunningState::Running) {
        return;
    }

    // 2. Let serviceWorkerGlobalScope be serviceWorker’s global object.
    // TODO: get global object using serviceWorkerGlobalScope Id

    // 3. Set serviceWorkerGlobalScope’s closing flag to true.

    // 4. Remove all the items from serviceWorker’s set of extended events.

    // 6. Abort the script currently running in serviceWorker.
    ServiceWorkerAgent::instance()->abortServiceWorkerScript(serviceWorker);
}

void ServiceWorkerHostJobHandler::rejectJobPromise(ServiceWorkerJob* job,
                                                   ErrorData* errorData)
{
    TRACE_SCOPE(HOST);
    STARFISH_ASSERT(job != nullptr);
    STARFISH_ASSERT(errorData != nullptr);
    // https://w3c.github.io/ServiceWorker/#reject-job-promise-algorithm
    // is implemented on ServiceWorkerContainer.
    job->hostConnection()->rejectJobPromise(job, errorData);
}

void ServiceWorkerHostJobHandler::updateRegistrationState(
    ServiceWorkerRegistrationData* registration,
    ServiceWorkerRegistrationState target, Nullable<ServiceWorkerData*> source)
{
    TRACE_SCOPE(HOST);
    STARFISH_ASSERT(registration != nullptr);
    // https://w3c.github.io/ServiceWorker/#update-registration-state-algorithm

    // 1. Let registrationObjects be an array containing all the
    // ServiceWorkerRegistration objects associated with registration.

    // 2. If target is "installing", then:
    switch (target) {
    case ServiceWorkerRegistrationState::Installing:
        // 2.1. Set registration’s installing worker to source.
        registration->setInstallingWorker(source);

        // 2.2 For each registrationObject in registrationObjects:
        // 2.2.1 Queue a task to set the installing attribute of
        // registrationObject to the ServiceWorker object that represents
        // registration’s installing worker, or null if registration’s
        // installing worker is null.
        break;
    case ServiceWorkerRegistrationState::Waiting:
    case ServiceWorkerRegistrationState::Active:
        STARFISH_UNIMPLEMENTED();
        break;
    default:
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        break;
    }
}

void ServiceWorkerHostJobHandler::updateWorkerState(ServiceWorkerData* worker,
                                                    ServiceWorkerState state)
{
    TRACE_SCOPE(HOST);
    STARFISH_ASSERT(worker != nullptr);

    // https://w3c.github.io/ServiceWorker/#update-worker-state

    // 1. Set worker’s state to state.
    worker->state = state;

    // 2. Let workerObjects be an array containing all the ServiceWorker objects
    // associated with worker.

    // NOTE: strategy : Here, we simply broadcast this changes to all the
    // connections. Clients ought to search ServiceWorkers which have a
    // registraion containing the matched Id, and then, update the state of the
    // ServiceWorkers.
    GCVector<IServiceWorkerClientConnection*> connections;

    m_SWServer->getConnections(connections);

    for (const auto& connection : connections) {
        connection->onUpdateWorkerState(worker->registrationId, state);
    }
}

void ServiceWorkerHostJobHandler::unregisterServiceWorker(ServiceWorkerJob* job)
{
    TRACE_SCOPE(HOST);
    STARFISH_ASSERT(job != nullptr);
    TRACE(HOST, "0: type: %d", toUnderlyingType(job->data()->type));

    // TODO: meet https://w3c.github.io/ServiceWorker/#unregister-algorithm

    // 1. If the origin of job’s scope url is not `job’s client's origin`, then:
    auto scopeURL = new ResourceURL(job->data()->scopeURL);
    auto clientOrigin = job->data()->clientOrigin;

    STARFISH_ASSERT(clientOrigin != nullptr);

    if (clientOrigin->equals(scopeURL->origin()) == false) {
        // 1.1 Invoke Reject Job Promise with job and "SecurityError"
        // DOMException.
        rejectJobPromise(job, new ErrorData(ExceptionCode::SECURITY_ERR,
                                            "Script origin does not match the "
                                            "registering client's origin"));

        // 1.2 Invoke Finish Job with job and abort these steps.
        return finishJob(job);
    }

    // 2. Let registration be the result of running
    // Get Registration algorithm passing job’s scope url as the argument.
    NULLABLE auto registration = getRegistration(job->data()->scopeURL);

    // 3. If registration is null, then:
    if (registration == nullptr) {
        // 3.1 Invoke Resolve Job Promise with job and false.
        // NOTE: we handle `false` as nullptr.
        resolveJobPromise(job, nullptr);

        // 3.2 Invoke Finish Job with job and abort these steps.
        return finishJob(job);
    }

    // 4. Set registration’s uninstalling flag.
    registration->setIsUninstalling(true);

    // 5. Invoke Resolve Job Promise with job and true.
    resolveJobPromise(job, registration);

    // 6. Invoke Try Clear Registration with registration.
    tryClearRegistration(registration);

    // Note: If `Try Clear Registration` does not trigger `Clear Registration`
    // here, `Clear Registration` is tried again when the last client using the
    // registration is unloaded or the extend lifetime promises for the
    // registration’s service workers settle.

    // 7. Invoke Finish Job with job.
    finishJob(job);
}

void ServiceWorkerHostJobHandler::finishJob(ServiceWorkerJob* job)
{
    TRACE_SCOPE(HOST);
    STARFISH_ASSERT(job != nullptr);
    TRACE(HOST, "0: type: %d", toUnderlyingType(job->data()->type));

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
    TRACE_SCOPE(HOST);
    STARFISH_ASSERT(request != nullptr);
    STARFISH_ASSERT(clientURLString != nullptr);

    TRACE(HOST, "0: %s", CSTR(clientURLString));

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

        TRACE(HOST, "4: %s", CSTR(selectedRegistrationKey));
        if (clientURLString->startsWith(selectedRegistrationKey, false) ==
            false) {
            continue;
        }

        // 5. Set matchingScopeString to the longest value in scopeStringSet
        // which the value of clientURLString starts with, if it exists.
        if (matchingScopeString->length() < selectedRegistrationKey->length()) {
            TRACE(HOST, "5: %s", CSTR(selectedRegistrationKey));
            matchingScopeString = selectedRegistrationKey;
        }
    }

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
        TRACE(HOST, "9: done");
        return nullptr;
    }

    // 10. Return registration.
    return registration;
}

void ServiceWorkerHostJobHandler::updateServiceWorkerClient(
    ContextRequestData* request)
{
    TRACE_SCOPE(HOST);
    STARFISH_ASSERT(request != nullptr);

    TRACE(HOST, "0: %s", request->contextId.toString().c_str());

    if (request->type == ServiceWorkerClientRequestType::Register) {
        if (request->registrationId.isValid()) {
            TRACE(HOST, "1: client is registered to regId: %s",
                  request->registrationId.toString().c_str());
            auto iter = m_clientIdToRegistrationIdMap.find(request->contextId);
            if (iter == m_clientIdToRegistrationIdMap.end()) {
                m_clientIdToRegistrationIdMap.insert(std::make_pair(
                    request->contextId, request->registrationId));
            } else {
                iter->second = request->registrationId;
            }
        }
    } else if (request->type == ServiceWorkerClientRequestType::Unregister) {
        TRACE(HOST, "1: client is unregistered");
        m_clientIdToRegistrationIdMap.erase(request->contextId);

    } else {
        STARFISH_ASSERT_NOT_REACHED();
    }
}

} // namespace Starfish

#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
