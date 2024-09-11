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

#include "StarfishConfig.h"
#include "Starfish.h"
#include "StoragePathProvider.h"

#include "core/util/Id.h"
#include "core/util/Archivable.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/dom/DOMException.h"
#include "core/modules/threading/IRunnable.h"
#include "core/modules/resource_request/ResourceRequest.h"
#include "core/dom/ExecutionContext.h"
#include "platform/loader/ResourceURL.h"
#include "platform/network/http/HTTPStatus.h"
#include "core/modules/worker/WorkerConfig.h"
#include "core/modules/worker/PerProcess.h"
#include "core/modules/worker/WorkerGlobalScope.h"
#include "core/modules/worker/util/network/IORunnable.h"
#include "core/modules/worker/util/network/Connection.h"
#include "core/modules/serviceworker/ServiceWorkerTypes.h"
#include "core/modules/serviceworker/ExceptionData.h"
#include "core/modules/serviceworker/MessageServiceWorker.h"
#include "core/modules/serviceworker/JobQueue.h"
#include "core/modules/serviceworker/ServiceWorkerData.h"
#include "core/modules/serviceworker/ServiceWorkerJobData.h"
#include "core/modules/serviceworker/ServiceWorkerRegistrationData.h"
#include "core/modules/serviceworker/ServiceWorkerJob.h"
#include "core/modules/serviceworker/host/ServiceWorkerFetchJob.h"
#include "core/modules/serviceworker/ConnectionInterface.h"
#include "core/modules/serviceworker/FetchEventData.h"
#include "core/modules/serviceworker/RegistrationStore.h"
#include "core/modules/serviceworker/host/ExtendableEvent.h"
#include "core/modules/serviceworker/host/ServiceWorkerAgent.h"
#include "core/modules/serviceworker/host/ServiceWorkerGlobalScope.h"
#include "core/modules/serviceworker/host/ServiceWorkerServerInterface.h"
#include "core/modules/serviceworker/host/ServiceWorkerHostJobHandler.h"

namespace Starfish {

class FetchClient : public ResourceRequestClient {
public:
    FetchClient(ServiceWorkerHostJobHandler* jobHandler, ServiceWorkerJob* job,
                ServiceWorkerData* data,
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
        worker->clientContextId = m_job->data()->contextId;

        // 12. Set 1) worker’s script url to job’s script url, 2) worker’s
        // script resource to script, 3) worker’s type to job’s worker type, and
        // 4) worker’s script resource map to updatedResourceMap.
        worker->scriptURL = job->data()->scriptURL;
        worker->scopeURL = job->data()->scopeURL;
        worker->setType(job->data()->workerType);
        worker->setUrlToScriptResourceMap(updatedResourceMap);
        worker->scriptResource().script = script;

        // 13. Append url to worker’s set of used scripts.
        // 14. Set worker’s script resource’s HTTPS state to httpsState.
        worker->scriptResource().httpsState = httpsState;
        // 15. Set worker’s script resource’s referrer policy to referrerPolicy.
        worker->scriptResource().referrerPolicy = referrerPolicy;

        // 16. Invoke Run Service Worker algorithm given worker
        ServiceWorkerAgent::instance()->runServiceWorker(worker);
        // TODO: 16.1 If evaluationStatus is an abrupt completion or
        // evaluationStatus.[[Value]] is empty, then:

        // 16.2 Else, invoke Install algorithm with job, worker, and
        // registration as its arguments.
        m_jobHandler->install(job, worker, registration);
    }

private:
    ServiceWorkerHostJobHandler* m_jobHandler;
    ServiceWorkerJob* m_job;
    ServiceWorkerData* m_serviveWorker;
    ServiceWorkerRegistrationData* m_registration;
};

ServiceWorkerHostJobHandler::ServiceWorkerHostJobHandler(
    PerProcess* perProcess, ServiceWorkerServerInterface* swserver)
    : m_messageLoop(perProcess->messageLoop())
    , m_SWServer(swserver)
    , m_registrationStore(new RegistrationStoreLocalStorage(
          perProcess->starfish()
              ->storagePathProvider()
              .getServiceWorkerDataDirectoryPath()))
{
    STARFISH_ASSERT(swserver != nullptr);

    m_registrationStore->load(m_scopeToRegistrationMap);
}

void ServiceWorkerHostJobHandler::scheduleJob(ServiceWorkerJob* job)
{
    TRACE_SCOPE(HOST);
    STARFISH_ASSERT(job != nullptr);
    TRACE(HOST, "0: type: ", toUnderlyingType(job->data()->type));

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
    TRACE(HOST, "5: is jobQueue empty? ", jobQueue->empty() ? "true" : "false");

    if (jobQueue->empty()) {
        // 5.1. Set job’s containing job queue to jobQueue, and enqueue job to
        // jobQueue.
        job->setContainingJobQueue(jobQueue);
        jobQueue->enqueueJob(job);

        // 5.2. Invoke Run Job with jobQueue.
        runJob(jobQueue);
    } else {
        STARFISH_UNSUPPORTED(
            "ServiceWorker: schedule job when there is a job in the queue");

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
            TRACEF(HOST, "1: %s (Found)", CSTR(queriedScope));
            return pair.second;
        }
    }

    TRACEF(HOST, "1: %s (Not Found)", CSTR(queriedScope));
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
            TRACEF(HOST, "1: %s (Found)", registrationId.toString());
            return registration;
        }
    }

    TRACEF(HOST, "1: %s (Not Found)", registrationId.toString());
    return nullptr;
}

void ServiceWorkerHostJobHandler::setRegistration(
    String* scope, ServiceWorkerUpdateViaCache updateViaCache)
{
    TRACE_SCOPE(HOST);
    STARFISH_ASSERT(scope != nullptr);

    TRACE(HOST, "0: ", CSTR(scope));

    // https://w3c.github.io/ServiceWorker/#set-registration-algorithm

    // 3. Let registration be a new service worker registration whose scope url
    // is set to scope and update via cache mode is set to updateViaCache.
    auto registration = new ServiceWorkerRegistrationData();
    registration->id = ServiceWorkerRegistrationId::generate();
    registration->scope = scope;
    registration->updateViaCache = updateViaCache;

    TRACE(HOST, "Create a registration", registration->id.toString());

    auto iter = m_scopeToRegistrationMap.find(scope);
    if (iter == m_scopeToRegistrationMap.end()) {
        m_scopeToRegistrationMap.insert(std::make_pair(scope, registration));
    } else {
        iter.value() = registration;
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
        auto newstWorker = getNewestWorker(registration);
        if (newstWorker &&
            job->data()->scriptURL->equals(newstWorker->scriptURL) &&
            job->data()->workerType == newstWorker->type() &&
            job->data()->updateViaCacheMode ==
                newstWorker->updateViaCacheMode()) {
            TRACE_SCOPE(HOST);

            resolveJobPromise(job, registration);
            finishJob(job);
            return;
        }
    } else {
        // 6. Invoke Set Registration algorithm with job’s scope url and job’s
        // update via cache mode.
        setRegistration(job->data()->scopeURL, job->data()->updateViaCacheMode);
    }

    // 7. Invoke Update algorithm passing job as the argument
    update(job);
}

ServiceWorkerData* ServiceWorkerHostJobHandler::getNewestWorker(
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
        newestWorker = registration->installingWorker();
    } else if (registration->waitingWorker()) {
        // 4. Else if registration’s waiting worker is not null, set
        // newestWorker to registration’s waiting worker.
        newestWorker = registration->waitingWorker();
    } else if (registration->activeWorker()) {
        // 5. Else if registration’s active worker is not null, set newestWorker
        // to registration’s active worker.
        newestWorker = registration->activeWorker();
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
        rejectJobPromise(job,
                         new ExceptionData(ExceptionCode::SCRIPT_TYPE_ERR,
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
        newestWorker != nullptr &&
        (newestWorker->scriptURL->equals(job->data()->scriptURL) == false)) {
        // 4.1 Invoke Reject Job Promise with job and TypeError.
        rejectJobPromise(
            job, new ExceptionData(
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

void ServiceWorkerHostJobHandler::startServiceWorkerContext(
    ServiceWorkerData* serviceWorkerData)
{
    TRACE(HOST, CSTR(serviceWorkerData->scopeURL));

    ServiceWorkerAgent::instance()->runServiceWorker(serviceWorkerData, true);
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

    // 2. Let newestWorker be the result of running Get Newest Worker algorithm
    //    passing registration as its argument.

    // 3. Set registration’s update via cache mode to job’s update via cache
    //    mode.
    registration->updateViaCache = job->data()->updateViaCacheMode;

    // 4. Run the Update Registration State algorithm passing registration,
    // "installing" and worker as the arguments.
    updateRegistrationState(registration,
                            ServiceWorkerRegistrationState::Installing, worker);

    // 5. Run the Update Worker State algorithm passing registration’s
    // installing worker and installing as the arguments.
    updateWorkerState(worker, ServiceWorkerState::Installing);

    // TODO: 6. Assert: job’s job promise is not null.
    // STARFISH_ASSERT(job->promise() != nullptr);

    // 7. Invoke Resolve Job Promise with job and registration.
    resolveJobPromise(job, registration);

    {
        TRACE_SCOPE(HOST);
        // 8. Let settingsObjects be all environment settings objects whose
        //    origin is registration’s scope url's origin.

        // 9. For each settingsObject of settingsObjects, queue a task on
        //    settingsObject’s responsible event loop in the DOM manipulation
        //    task source to run the following steps:

        // 9.1. Let registrationObjects be every ServiceWorkerRegistration
        // object in settingsObject’s realm, whose service worker registration
        // is registration.

        // 9.2. For each registrationObject of registrationObjects, fire an
        // event on registrationObject named `updatefound`.
        GCVector<IServiceWorkerClientConnection*> connections;
        m_SWServer->getConnections(connections);

        for (const auto& connection : connections) {
            connection->fireEventRequest(
                worker->scriptURL, String::createASCIIString("updatefound"));
        }
    }
    // 10. Let installingWorker be registration’s installing worker.
    auto installingWorker = registration->installingWorker();

    // 11. If the result of running the Should Skip Event algorithm with
    //     installingWorker and "install" is false, then:

    // 11.1. Let forceBypassCache be true if job’s force bypass cache flag is
    // set, and false otherwise.
    // TODO: update job to have forceBypassCache

    bool forceBypassCache = false;
    if (forceBypassCache) {
        // 11.2. If the result of running the Run Service Worker algorithm with
        // installingWorker and forceBypassCache is failure, then:
        // 11.2.1. Set installFailed to true.
    } else {
        // 11.3. Else:

        // 11.3.1. Queue a task task on installingWorker’s event loop using the
        // DOM manipulation task source to run the following steps:
        // 11.3.1.1. Let e be the result of creating an event with
        // ExtendableEvent.
        // 11.3.1.2. Initialize e’s type attribute to install.
        // 11.3.1.3. Dispatch e at installingWorker’s global object.
        auto event = new ExtendableEvent(
            installingWorker->globalObject()->executionContext(),
            String::fromUTF8("install"));
        installingWorker->globalObject()->dispatchEventByUA(event);
        // 11.3.1.4. WaitForAsynchronousExtensions: Run the following substeps
        // in parallel:
        // 11.3.1.4.1. Wait until e is not active.
        // 11.3.1.4.2. If e’s timed out flag is set, set installFailed to true.

        // 11.3.1.4.3. Let p be the result of getting a promise to wait for all
        // of e’s extend lifetime promises.

        // 11.3.1.4.4. Upon rejection of p, set installFailed to true.

        //         If task is discarded, set installFailed to true.

        // 11.3.2. Wait for task to have executed or been discarded.
        // 11.3.3. Wait for the step labeled WaitForAsynchronousExtensions to
        // complete.
    }

    // 17. Run the Update Registration State algorithm passing registration,
    // "waiting" and registration’s installing worker as the arguments.
    updateRegistrationState(registration,
                            ServiceWorkerRegistrationState::Waiting, worker);

    // 18. Run the Update Registration State algorithm passing registration,
    // "installing" and null as the arguments.
    updateRegistrationState(
        registration, ServiceWorkerRegistrationState::Installing, nullptr);

    // 19. Run the Update Worker State algorithm passing registration’s
    // waiting worker and "installed" as the arguments.
    updateWorkerState(worker, ServiceWorkerState::Installed);

    // 20. Invoke Finish Job with job.
    finishJob(job);

    // 21. Wait for all the tasks queued by Update Worker State invoked in this
    // algorithm to have executed.
    // TODO

    // 22. Invoke Try Activate with registration.
    //
    // Note: If Try Activate does not trigger Activate here, Activate is tried
    // again when the last client controlled by the existing active worker is
    // unloaded, `skipWaiting()` is asynchronously called, or the extend
    // lifetime promises for the existing active worker settle.

    // TODO: tryActivate(registration);
}

void ServiceWorkerHostJobHandler::tryActivate(
    ServiceWorkerRegistrationData* registration)
{
    TRACE_SCOPE(HOST);
    // https://w3c.github.io/ServiceWorker/#try-activate-algorithm

    // 1. If registration’s waiting worker is null, return.
    if (registration->waitingWorker() == nullptr) {
        return;
    }

    // 2. If registration’s active worker is not null and registration’s active
    // worker's state is "activating", return.
    if (registration->activeWorker() != nullptr &&
        registration->activeWorker()->state == ServiceWorkerState::Activating) {
        // Note: If the existing active worker is still in activating state, the
        // activation of the waiting worker is delayed.
        return;
    }

    // 3. Invoke `Activate` with registration if either of the following is
    // true:

    // Condition 1 - registration’s active worker is null.
    if (registration->activeWorker() == nullptr) {
        return activate(registration);
    }

    // Condition 2 - The result of running `Service Worker Has No Pending
    // Events` with registration’s `active worker` is true, and

    bool condition1 =
        serviceWorkerHasNoPendingEvents(registration->activeWorker());

    // TODO: no `service worker client` is using registration
    bool isNoServiceWorkerClientIsUsingRegistration = false;

    // or registration’s waiting worker's `skip waiting flag` is set.
    bool condition2 = (isNoServiceWorkerClientIsUsingRegistration ||
                       registration->waitingWorker()->skipWaiting());

    if (condition1 && condition2) {
        return activate(registration);
    }
}

void ServiceWorkerHostJobHandler::activate(
    ServiceWorkerRegistrationData* registration)
{
    TRACE_SCOPE(HOST);
    // TODO: https://www.w3.org/TR/service-workers/#activation-algorithm

    // 1. If registration’s waiting worker is null, abort these steps.
    if (!registration->waitingWorker()) {
        return;
    }
    // 2. If registration’s active worker is not null, then:
    if (registration->activeWorker()) {
        // 2.1. Terminate registration’s active worker.

        // 2.2. Run the Update Worker State algorithm passing registration’s
        // active worker and "redundant" as the arguments.
    }

    // 3. Run the Update Registration State algorithm passing registration,
    //    "active" and registration’s waiting worker as the arguments.
    updateRegistrationState(registration,
                            ServiceWorkerRegistrationState::Active,
                            registration->waitingWorker());

    // 4. Run the Update Registration State algorithm passing registration,
    //    "waiting" and null as the arguments.
    updateRegistrationState(registration,
                            ServiceWorkerRegistrationState::Waiting, nullptr);

    // 5. Run the Update Worker State algorithm passing registration’s active
    //    worker and "activating" as the arguments.
    updateWorkerState(registration->activeWorker(),
                      ServiceWorkerState::Activating);

    // Note: Once an active worker is activating, neither a runtime script error
    // nor a force termination of the active worker prevents the active worker
    // from getting activated.

    // Note: Make sure to design activation handlers to do non-essential work
    // (like cleanup). This is because activation handlers may not all run to
    // completion, especially in the case of browser termination during
    // activation. A Service Worker should be designed to function properly,
    // even if the activation handlers do not all complete successfully.

    // 6. Let matchedClients be a list of service worker clients whose creation
    //    URL matches registration’s storage key and registration’s scope url.

    // TODO: The step, 7, needs to be run on each clients.

    // 7. For each client of matchedClients, queue a task on client’s
    //    responsible event loop, using the DOM manipulation task source, to run
    //    the following substeps:
    {
        // 7.1. Let readyPromise be client’s global object's
        // ServiceWorkerContainer object’s ready promise.

        // 7.2. If readyPromise is null, then continue.

        // 7.3. If readyPromise is pending, resolve readyPromise with the the
        // result of getting the service worker registration object that
        // represents registration in readyPromise’s relevant settings object.
    }

    // 8. For each client of matchedClients:
    {
        // 8.1. If client is a window client, unassociate client’s responsible
        // document from its application cache, if it has one.

        // 8.2. Else if client is a shared worker client, unassociate client’s
        // global object from its application cache, if it has one.

        // Note: Resources will now use the service worker registration instead
        // of the existing application cache.
    }

    // 9. For each service worker client client who is using registration:
    {
        // 9.1. Set client’s active worker to registration’s active worker.

        // 9.2. Invoke Notify Controller Change algorithm with client as the
        // argument.
    }

    // 10. Let activeWorker be registration’s active worker.
    ServiceWorkerData* activeWorker = registration->activeWorker();

    // 11. If the result of running the "Should Skip Event" algorithm with
    //     activeWorker and "activate" is false, then:

    if (shouldSkipEvent("activate", activeWorker)) {
        // 11.1. If the result of running the Run Service Worker algorithm with
        // activeWorker is not failure, then:

        // 11.1.1. Queue a task task on activeWorker’s event loop using the DOM
        // manipulation task source to run the following steps:
        // 11.1.1.1. Let e be the result of creating an event with
        // ExtendableEvent.
        // 11.1.1.2. Initialize e’s type attribute to activate.
        // 11.1.1.3. Dispatch e at activeWorker’s global object.
        auto event = new ExtendableEvent(
            activeWorker->globalObject()->executionContext(),
            String::fromUTF8("activate"));
        activeWorker->globalObject()->dispatchEventByUA(event);

        // 11.1.1.4. WaitForAsynchronousExtensions: Wait, in parallel, until e
        // is not active.

        // 11.1.2. Wait for task to have executed or been discarded.

        // 11.1.3. Wait for the step labeled WaitForAsynchronousExtensions to
        // complete.
    }

    // 12. Run the Update Worker State algorithm passing registration’s active
    //     worker and "activated" as the arguments.
    updateWorkerState(activeWorker, ServiceWorkerState::Activated);
}

bool ServiceWorkerHostJobHandler::shouldSkipEvent(
    std::string eventName, ServiceWorkerData* serviceWorker)
{
    // Note: To avoid unnecessary delays, this specification permits skipping
    // event dispatch when no event listeners for the event have been
    // deterministically added in the service worker’s global during the very
    // first script execution.

    // If serviceWorker’s set of event types to handle does not contain
    // eventName, then the user agent may return true.

    // TODO: A service worker has an associated set of event types to handle.
    return true;

    // Return false.
}

bool ServiceWorkerHostJobHandler::serviceWorkerHasNoPendingEvents(
    ServiceWorkerData* serviceWorker)
{
    TRACE_SCOPE(HOST);
    // https://w3c.github.io/ServiceWorker/#service-worker-has-no-pending-events
    // 1. For each event of worker’s set of extended events:
    // 1.1 If event is active, return false.

    // 2. Return true.
    return true;
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
    if ((registration->waitingWorker() != nullptr) &&
        (registration->waitingWorker()->hasPendingEvents() == true)) {
        return false;
    }

    // 1.3 registration’s active worker is null or the result of running
    // Service Worker Has No Pending Events with registration’s active
    // worker is true.
    if ((registration->activeWorker() != nullptr) &&
        (registration->activeWorker()->hasPendingEvents() == true)) {
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
        terminateServiceWorker(registration->installingWorker());

        // 2.2. Run the `Update Worker State` algorithm passing registration’s
        // installing worker and redundant as the arguments.
        updateWorkerState(registration->installingWorker(),
                          ServiceWorkerState::Redundant);

        // 2.3 Run the Update Registration State algorithm passing registration,
        // "installing" and null as the arguments.
        updateRegistrationState(
            registration, ServiceWorkerRegistrationState::Installing, nullptr);
    }

    // 3. If registration’s waiting worker is not null, then:
    if (registration->waitingWorker()) {
        // 3.1 Terminate registration’s waiting worker.
        terminateServiceWorker(registration->waitingWorker());

        // 3.2 Run the Update Worker State algorithm passing registration’s
        // waiting worker and redundant as the arguments.
        updateWorkerState(registration->waitingWorker(),
                          ServiceWorkerState::Redundant);

        // 3.3 Run the Update Registration State algorithm passing registration,
        // "waiting" and null as the arguments.
        updateRegistrationState(
            registration, ServiceWorkerRegistrationState::Waiting, nullptr);
    }

    // 4. If registration’s active worker is not null, then:
    if (registration->activeWorker()) {
        // 4.1 Terminate registration’s active worker.
        terminateServiceWorker(registration->activeWorker());

        // 4.2 Run the Update Worker State algorithm passing registration’s
        // active worker and redundant as the arguments.
        updateWorkerState(registration->activeWorker(),
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

    ServiceWorkerAgent::instance()->removeGlobalScope(
        serviceWorker->clientContextId);
}

void ServiceWorkerHostJobHandler::rejectJobPromise(ServiceWorkerJob* job,
                                                   ExceptionData* errorData)
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
    ServiceWorkerRegistrationState target, ServiceWorkerData* source)
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
        {
            TRACE_SCOPE(HOST);
            GCVector<IServiceWorkerClientConnection*> connections;
            m_SWServer->getConnections(connections);

            for (const auto& connection : connections) {
                connection->onUpdateRegistrationState(registration, target,
                                                      source);
            }
        }
        break;
    case ServiceWorkerRegistrationState::Waiting:
        // 3.1 Set registration’s waiting worker to source.
        registration->setWaitingWorker(source);

        // 3.2 For each registrationObject in registrationObjects:
        // 3.2.1 Queue a task to set the waiting attribute of registrationObject
        // to null if registration’s waiting worker is null, or the result of
        // getting the service worker object that represents registration’s
        // waiting worker in registrationObject’s relevant settings object.
        {
            TRACE_SCOPE(HOST);
            GCVector<IServiceWorkerClientConnection*> connections;
            m_SWServer->getConnections(connections);

            for (const auto& connection : connections) {
                connection->onUpdateRegistrationState(registration, target,
                                                      source);
            }
        }
        break;
    case ServiceWorkerRegistrationState::Active:
        // 4.1 Set registration’s active worker to source.
        registration->setActiveWorker(source);

        // 4.2 For each registrationObject in registrationObjects:
        // 4.2.1 Queue a task to set the active attribute of registrationObject
        // to null if registration’s active worker is null, or the result of
        // getting the service worker object that represents registration’s
        // active worker in registrationObject’s relevant settings object.
        {
            TRACE_SCOPE(HOST);
            GCVector<IServiceWorkerClientConnection*> connections;
            m_SWServer->getConnections(connections);

            for (const auto& connection : connections) {
                connection->onUpdateRegistrationState(registration, target,
                                                      source);
            }
        }

        m_registrationStore->add(registration);

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
    // connections. Clients needs to search a ServiceWorker having a registraion
    // with the matched Id, and then, update the state of the ServiceWorkers.
    GCVector<IServiceWorkerClientConnection*> connections;

    m_SWServer->getConnections(connections);

    for (const auto& connection : connections) {
        connection->onUpdateWorkerState(worker->scriptURL, state);
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
        rejectJobPromise(job,
                         new ExceptionData(ExceptionCode::SECURITY_ERR,
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

    m_registrationStore->remove(registration);

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
    TRACE(HOST, "0: type: ", toUnderlyingType(job->data()->type));

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

    TRACE(HOST, "0: ", CSTR(clientURLString));

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

        TRACE(HOST, "4: ", CSTR(selectedRegistrationKey));
        if (clientURLString->startsWith(selectedRegistrationKey, false) ==
            false) {
            continue;
        }

        // 5. Set matchingScopeString to the longest value in scopeStringSet
        // which the value of clientURLString starts with, if it exists.
        if (matchingScopeString->length() < selectedRegistrationKey->length()) {
            TRACE(HOST, "5: ", CSTR(selectedRegistrationKey));
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

    TRACE(HOST, "0: ", request->contextId.toString());

    if (request->type == ServiceWorkerClientRequestType::Register) {
        if (request->registrationId.isValid()) {
            TRACE(HOST, "1: client is registered to regId: %s",
                  request->registrationId.toString().c_str());
            auto iter = m_clientIdToRegistrationIdMap.find(request->contextId);
            if (iter == m_clientIdToRegistrationIdMap.end()) {
                m_clientIdToRegistrationIdMap.insert(std::make_pair(
                    request->contextId, request->registrationId));
            } else {
                iter.value() = request->registrationId;
            }
        }
    } else if (request->type == ServiceWorkerClientRequestType::Unregister) {
        TRACE(HOST, "1: client is unregistered");
        m_clientIdToRegistrationIdMap.erase(request->contextId);

    } else {
        STARFISH_ASSERT_NOT_REACHED();
    }
}

void ServiceWorkerHostJobHandler::handleFetch(
    FetchEventRequestData* data, ServiceWorkerHostConnection* connection)
{
    TRACE(HOST);
    auto contextId = data->contextId;

    auto clientValue =
        ServiceWorkerAgent::instance()->findGlobalScopeByContextId(contextId);
    if (!clientValue.hasValue()) {
        STARFISH_LOG_WARN("Cannot find global scope!");
        return;
    }

    auto client = clientValue.getValue();
    auto fetchJob = new ServiceWorkerFetchJob(client, contextId, connection);
    fetchJob->handleFetch(data);
}

} // namespace Starfish

#endif // #ifdef STARFISH_WEBWORKER_HOST
