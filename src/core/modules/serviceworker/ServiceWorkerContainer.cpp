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
#include "binding/ScriptBindingInstance.h"

#include "core/util/Id.h"
#include "core/modules/serviceworker/Task.h"
#include "core/modules/serviceworker/ServiceWorkerContainer.h"
#include "core/dom/ExecutionContext.h"

#include "core/modules/serviceworker/WorkerConfig.h"

#include "platform/process/base/ProcessType.h"
#include "core/modules/threading/IRunnable.h"
#include "core/modules/serviceworker/IORunnable.h"
#include "core/modules/serviceworker/Connection.h"

#include "core/modules/serviceworker/MessageServiceWorker.h"
#include "core/modules/serviceworker/ConnectionInterface.h"
#include "core/modules/serviceworker/RegistrationOptions.h"
#include "core/modules/serviceworker/ServiceWorker.h"

#include "core/page/GlobalScope.h"
#include "core/dom/WebOrigin.h"
#include "core/dom/DOMException.h"
#include "core/page/WebBase.h"
#include "core/storage/StorageInternal.h"
#include "core/modules/message_loop/MessageLoop.h"

#include "core/modules/serviceworker/client/ServiceWorkerClientConnection.h"

#if defined(STARFISH_ENABLE_SERVICE_WORKER) && !defined(STARFISH_WEBWORKER_HOST)
#include "core/modules/serviceworker/client/ServiceWorkerProcessManager.h"
#endif

#include "core/modules/serviceworker/ServiceWorkerRequest.h"

namespace Starfish {

ServiceWorkerContainer::ServiceWorkerContainer(
    ExecutionContext* executionContext)
    : EventTarget()
    , m_executionContext(executionContext)
    , m_state(State::Started)
{
}

ServiceWorkerContainer::~ServiceWorkerContainer()
{
}

void ServiceWorkerContainer::dispose()
{
    m_state = State::Disposed;
}

ExecutionContext* ServiceWorkerContainer::executionContext() const
{
    return m_executionContext;
}

ServiceWorkerEnvironment* ServiceWorkerContainer::serviceWorkerEnvironment()
{
    return m_executionContext;
}

Promise* ServiceWorkerContainer::registerServiceWorker(
    String* rawScriptURL, RegistrationOptions& options)
{
    TRACE_SCOPE(SVCWORKER);
    STARFISH_ASSERT(rawScriptURL != nullptr);

    // https://w3c.github.io/ServiceWorker/#navigator-service-worker-register

    // 1. Let p be a promise.
    Promise* p = new Promise(scriptBindingInstance());

    STARFISH_ASSERT(p != nullptr);

    // 2. Let client be the context object’s service worker client.
    ServiceWorkerEnvironment* client = serviceWorkerEnvironment();

    // 3. Let scriptURL be the result of parsing scriptURL with the context
    // object’s relevant settings object’s API base URL.
    auto scriptURL =
        new ResourceURL(rawScriptURL, client->baseURL()->baseURI());

    STARFISH_ASSERT(scriptURL != nullptr);

    // 4. Let scopeURL be null
    ResourceURL* scopeURL = nullptr;

    if (!options.scope()->isEmpty()) {
        // 5. If options.scope is present, set scopeURL to the result of parsing
        // options.scope with the context object’s relevant settings object’s
        // API base URL.
        scopeURL =
            new ResourceURL(options.scope(), client->baseURL()->baseURI());
    }

    // 6. Invoke Start Register with scopeURL, scriptURL, p, client, client’s
    // creation URL, options["type"], and options["updateViaCache"].
    startRegister(scopeURL, scriptURL, p, client, options.m_type,
                  options.m_updateViaCache);

    return p;
}

void ServiceWorkerContainer::startRegister(
    Nullable<ResourceURL*> scopeURL, ResourceURL* scriptURL, Promise* promise,
    ServiceWorkerEnvironment* client, WorkerType type,
    ServiceWorkerUpdateViaCache updateViaCache)
{
    TRACE_SCOPE(SVCWORKER);
    STARFISH_ASSERT(scriptURL != nullptr);
    STARFISH_ASSERT(promise != nullptr);
    STARFISH_ASSERT(client != nullptr);

    // https://w3c.github.io/ServiceWorker/#start-register

    // 1. If scriptURL is failure, reject promise with a TypeError and abort
    // these steps.
    if (scriptURL->urlString()->isEmpty()) {
        auto exception = new DOMException(executionContext(),
                                          DOMException::Code::SCRIPT_TYPE_ERR,
                                          "serviceWorker.register() cannot be "
                                          "called with an empty script URL");
        promise->reject(exception->scriptValue());
        return;
    }

    // 2. Set scriptURL’s fragment to null.
    auto scriptURLWithNoFragment = scriptURL->urlStringWithoutSearchPart();

    STARFISH_ASSERT(scriptURLWithNoFragment != nullptr);

    // 3. If scriptURL’s scheme is not one of "http" and "https", reject promise
    // with a TypeError and abort these steps.
    if (scriptURL->isHTTPFamilyURL() == false) {
        auto exception = new DOMException(
            executionContext(), DOMException::Code::SCRIPT_TYPE_ERR,
            "serviceWorker.register() must be called with a script URL whose "
            "protocol is either HTTP or HTTPS");
        promise->reject(exception->scriptValue());
        return;
    }

    // 4. If any of the strings in scriptURL’s path contains either ASCII
    // case-insensitive
    // "%2f" or ASCII case-insensitive "%5c", reject promise with a TypeError
    // and abort these steps.
    if (scriptURLWithNoFragment->contains("%2f", false) ||
        scriptURLWithNoFragment->contains("%5c", false)) {
        auto exception = new DOMException(
            executionContext(), DOMException::Code::SCRIPT_TYPE_ERR,
            "Script URL provided to serviceWorker.register() cannot have a "
            "path that contains '%2f' or '%5c'");
        promise->reject(exception->scriptValue());
        return;
    }

    // 5. If scopeURL is null, set scopeURL to the result of parsing the string
    // "./" with scriptURL.
    if (!scopeURL.hasValue()) {
        scopeURL = new ResourceURL(scriptURL->urlString(),
                                   String::createASCIIString("./"));
        STARFISH_ASSERT(scopeURL != nullptr);
    }

    // 8. If scopeURL’s scheme is not one of "http" and "https", reject promise
    // with a TypeError and abort these steps.
    if (scopeURL->isHTTPFamilyURL() == false) {
        auto exception = new DOMException(
            executionContext(), DOMException::Code::SCRIPT_TYPE_ERR,
            "serviceWorker.register() must be called with a scope URL whose "
            "protocol is either HTTP or HTTPS");
        promise->reject(exception->scriptValue());
        return;
    }

    // 9. If any of the strings in scopeURL path contains either ASCII
    // case-insensitive
    // "%2f" or ASCII case-insensitive "%5c", reject promise with a TypeError
    // and abort these steps.
    auto scopeURLWithNoFragment = scopeURL->urlStringWithoutSearchPart();

    STARFISH_ASSERT(scopeURLWithNoFragment != nullptr);

    if (scopeURLWithNoFragment->contains("%2f", false) ||
        scopeURLWithNoFragment->contains("%5c", false)) {
        auto exception = new DOMException(
            executionContext(), DOMException::Code::SCRIPT_TYPE_ERR,
            "Scope URL provided to serviceWorker.register() cannot have a path "
            "that contains '%2f' or '%5c'");
        promise->reject(exception->scriptValue());
        return;
    }

    // 10. Let job be the result of running Create Job with register, scopeURL,
    // scriptURL, promise, and client.
    auto job = createJob(ServiceWorkerJobType::Register, scopeURLWithNoFragment,
                         scriptURLWithNoFragment, promise, client);

    // 11. Set job’s worker type to workerType.
    job->data()->workerType = type;

    // 12. Set job’s update via cache mode to updateViaCache.
    job->data()->updateViaCacheMode = updateViaCache;

    // 13. Set job’s referrer to referrer.
    job->data()->referrerURL =
        job->client() ? job->client()->referrer() : nullptr;

    // 14. Invoke Schedule Job with job.
    scheduleJob(job);
}

ServiceWorkerJob* ServiceWorkerContainer::createJob(
    ServiceWorkerJobType type, NULLABLE String* scopeURL,
    NULLABLE String* scriptURL, Promise* promise,
    NULLABLE ServiceWorkerEnvironment* client)
{
    TRACE_SCOPE(SVCWORKER);
    // https://w3c.github.io/ServiceWorker/#create-job

    auto job = new ServiceWorkerJob();

    STARFISH_ASSERT(job != nullptr);
    STARFISH_ASSERT(promise != nullptr);

    auto data = job->data();

    data->id = ServiceWorkerJobId::generate();
    data->contextId = executionContext()->globalScope()->uid();
    data->type = type;
    data->scopeURL = scopeURL;
    data->scriptURL = scriptURL;
    data->clientOrigin = executionContext()->webOrigin()->serialize();
    data->referrerURL = client ? client->referrer() : nullptr;

    job->setPromise(promise);
    job->setClient(client);

    return job;
}

void ServiceWorkerContainer::scheduleJob(ServiceWorkerJob* job)
{
    TRACE_SCOPE(SVCWORKER);
    STARFISH_ASSERT(job != nullptr);

    executionContext()->webBase()->messageLoop()->addIdler(
        executionContext()->globalScope(),
        [](size_t handle, void* data1, void* data2) {
            TRACE_SCOPE(SVCWORKER);
            ServiceWorkerJob* job = castTo<ServiceWorkerJob*>(data1);
            WebOrigin* webOrigin = castTo<WebOrigin*>(data2);
#if defined(STARFISH_ENABLE_SERVICE_WORKER) && !defined(STARFISH_WEBWORKER_HOST)
            auto swConnection =
                ServiceWorkerProcessManager::instance()->getConnection(
                    webOrigin->serialize());
            swConnection->scheduleJob(job);
#else
            STARFISH_UNIMPLEMENTED();
#endif
        },
        job, executionContext()->webOrigin());

    m_jobMap.insert(std::make_pair(job->data()->id, job));
}

Promise* ServiceWorkerContainer::getRegistration(NULLABLE String* rawClientURL)
{
    TRACE_SCOPE(SVCWORKER);
    TRACE(CLIENT, "0: ", CSTR(rawClientURL));

    // https://w3c.github.io/ServiceWorker/#navigator-service-worker-getRegistration

    // 1. Let client be the context object’s service worker client.
    ServiceWorkerEnvironment* client = serviceWorkerEnvironment();

    STARFISH_ASSERT(client != nullptr);
    STARFISH_ASSERT(client->baseURL() != nullptr);

    // 2. Let clientURL be the result of parsing clientURL with the context
    // object’s relevant settings object’s API base URL.
    auto clientURL =
        new ResourceURL(rawClientURL, client->baseURL()->baseURI());

    // 3. If clientURL is failure, return a promise rejected with a TypeError.
    if (clientURL->urlString()->isEmpty()) {
        auto exception = new DOMException(
            executionContext(), DOMException::Code::SCRIPT_TYPE_ERR,
            "serviceWorker.getRegistration() cannot be called with an empty "
            "script URL");
        Promise* promise = new Promise(scriptBindingInstance());
        promise->reject(exception->scriptValue());
        return promise;
    }

    // 4. Set clientURL’s fragment to null.
    auto clientURLWithNoFragment = clientURL->urlStringWithoutSearchPart();

    STARFISH_ASSERT(clientURLWithNoFragment != nullptr);

    // 5. If the origin of clientURL is not client’s origin, return a promise
    // rejected with a "SecurityError" DOMException.

    if (client->webOrigin()->isSameOrigin(
            WebOrigin::createDocumentOrigin(clientURL)) == false) {
        auto exception = new DOMException(
            executionContext(), DOMException::Code::SECURITY_ERR,
            "Origin of clientURL is not client's origin");
        Promise* promise = new Promise(scriptBindingInstance());
        promise->reject(exception->scriptValue());
        return promise;
    }

    // 6. Let promise be a new promise.
    Promise* promise = new Promise(scriptBindingInstance());

    STARFISH_ASSERT(promise != nullptr);

    // 7. Run the following substeps in parallel:

    auto matchRegistrationRequest = createRequest("matchRegistration", promise);

    matchRegistrationRequest->setPostTask(new RequestTask(
        [](ServiceWorkerRequest& req, TaskResult& results, TaskParam& params) {
            TRACE_SCOPE(SVCWORKER);
            auto request = castTo<ServiceWorkerRequest*>(params[0]);
            auto container = castTo<ServiceWorkerContainer*>(params[1]);

            STARFISH_ASSERT(req.id == request->id);

            // 7.1 Let registration be the result of running Match Service
            // Worker Registration algorithm with clientURL as its argument.
            auto registration =
                static_cast<ServiceWorkerRegistrationData*>(results[0]);

            // 7.2 If registration is not null, then:
            if (registration != nullptr) {
                // 7.2.1 Resolve promise with the ServiceWorkerRegistration
                // object which represents registration.
                TRACE(CLIENT, "7.2.1: ", CSTR(registration->scope));

                auto swRegistration = new ServiceWorkerRegistration(
                    container->executionContext(), container);
                swRegistration->setData(registration);

                STARFISH_ASSERT(request->promise());
                request->promise()->fulfill(swRegistration->scriptValue());

            } else {
                // 7.3 Else:
                // 7.3.1 Resolve promise with undefined.
                TRACE(CLIENT, "7.3.1: null");
                request->promise()->fulfill(scriptUndefined());
            }
        },
        { matchRegistrationRequest, this }));

    matchRegistration(matchRegistrationRequest, clientURL);

    // 8. Return promise.
    return promise;
}

// https://w3c.github.io/ServiceWorker/#navigator-service-worker-getRegistrations
Promise* ServiceWorkerContainer::getRegistrations()
{
    TRACE_SCOPE(SVCWORKER);
    // 1. Let client be this's service worker client.
    auto client = serviceWorkerEnvironment();

    // 2. Let client storage key be the result of running obtain a storage key
    // given client.
    auto storageKey = StorageInternal::getStorageKey(client);

    // 3. Let promise be a new promise.
    auto promise = new Promise(scriptBindingInstance());

    // TODO: Send and receive data from Job handler
    STARFISH_UNIMPLEMENTED();

    auto registrationsObjects =
        createScriptArrayBuffer(scriptBindingInstance(), 0);
    promise->fulfill(createScriptValue(registrationsObjects));

    // 5. Return promise.
    return promise;
}

ServiceWorkerRequest* ServiceWorkerContainer::createRequest(
    const char* requestName, Promise* promise)
{
    STARFISH_ASSERT(requestName != nullptr);
    STARFISH_ASSERT(promise != nullptr);

    auto request = new ServiceWorkerRequest();
    request->id = RequestId::generate();
    request->contextId = executionContext()->globalScope()->uid();
    request->name = String::createASCIIString(requestName, strlen(requestName));
    request->origin = executionContext()->webOrigin()->serialize();
    request->setPromise(promise);
    return request;
}

void ServiceWorkerContainer::matchRegistration(ServiceWorkerRequest* request,
                                               ResourceURL* clientURL)
{
    TRACE_SCOPE(SVCWORKER);
    STARFISH_ASSERT(request != nullptr);
    STARFISH_ASSERT(clientURL != nullptr);

    executionContext()->webBase()->messageLoop()->addIdler(
        executionContext()->globalScope(),
        [](size_t handle, void* data1, void* data2) {
            TRACE_SCOPE(SVCWORKER);
            auto swrequest = castTo<ServiceWorkerRequest*>(data1);
            auto urlString = castTo<String*>(data2);
#if defined(STARFISH_ENABLE_SERVICE_WORKER) && !defined(STARFISH_WEBWORKER_HOST)
            auto swConnection =
                ServiceWorkerProcessManager::instance()->getConnection(
                    swrequest->origin);

            swConnection->matchRegistration(swrequest, urlString);
#else
            STARFISH_UNIMPLEMENTED();
#endif
        },
        request, clientURL->urlString());

    m_requestMap.insert(std::make_pair(request->id, request));
}

Promise* ServiceWorkerContainer::registerServiceWorker(String* url)
{
    TRACE_SCOPE(SVCWORKER);
    STARFISH_ASSERT(url != nullptr);
    RegistrationOptions defaultOptions;
    return registerServiceWorker(url, defaultOptions);
}

ServiceWorker* ServiceWorkerContainer::controller()
{
    TRACE_SCOPE(SVCWORKER);
    // 1. Let client be the context object’s service worker client.
    ExecutionContext* context = executionContext();

    STARFISH_ASSERT(context != nullptr);

    // 2. Return the ServiceWorker object that represents client’s active
    // service worker.
    auto client = context->activeServiceWorker();
    return client;
}

void ServiceWorkerContainer::resolveJobPromise(
    ServiceWorkerJob* job, NULLABLE ServiceWorkerRegistrationData* registration)
{
    TRACE_SCOPE(SVCWORKER);
    STARFISH_ASSERT(job != nullptr);

    // https://w3c.github.io/ServiceWorker/#resolve-job-promise-algorithm
    // TODO: get matched registration

    // 1. If job’s client is not null, queue a task, on job's client's
    // responsible event loop using the DOM manipulation task source, to run the
    // following substeps:

    // TODO: consider moving resolveJobPromise to ServiceWorkerClientConnection
    auto context = job->client();

    if (context != nullptr) {
        context->webBase()->messageLoop()->addIdler(
            context->globalScope(),
            [](size_t handle, void* data, void* data1) {
                TRACE_SCOPE(SVCWORKER);
                ServiceWorkerJob* job = castTo<ServiceWorkerJob*>(data);
                ServiceWorkerContainer* container =
                    castTo<ServiceWorkerContainer*>(data1);

                // 1.1 Let convertedValue be null.
                auto convertedValue = scriptNull();

                if (job->data()->type == ServiceWorkerJobType::Register ||
                    job->data()->type == ServiceWorkerJobType::Update) {
                    // 1.2. If job's job type is either register or update, set
                    // convertedValue to the result of getting the service
                    // worker registration object that represents value in job's
                    // client.
                    //
                    // TODO: Implement `getting the service worker registration
                    // object`
                    // https://w3c.github.io/ServiceWorker/#get-the-service-worker-registration-object
                    auto registration = new ServiceWorkerRegistration(
                        container->executionContext(), container);
                    auto serviceWorker =
                        new ServiceWorker(container->executionContext());

                    registration->data()->scope = job->data()->scopeURL;
                    registration->data()->updateViaCache =
                        job->data()->updateViaCacheMode;
                    serviceWorker->data()->scriptURL = job->data()->scriptURL;
                    registration->updateRegistrationState(
                        ServiceWorkerRegistrationState::Installing,
                        serviceWorker);

                    convertedValue = registration->scriptValue();
                } else {
                    // 1.3 Else, set convertedValue to value, in job's client's
                    // Realm.
                }

                // 1.4 Resolve job's job promise with convertedValue.
                job->promise()->fulfill(convertedValue);

                container->finishJob(job);
            },
            job, this);
    } else {
        finishJob(job);
    }
}

void ServiceWorkerContainer::rejectJobPromise(ServiceWorkerJob* job,
                                              ErrorData* errorData)
{
    TRACE_SCOPE(SVCWORKER);
    STARFISH_ASSERT(job != nullptr);
    STARFISH_ASSERT(errorData != nullptr);
    // https://w3c.github.io/ServiceWorker/#reject-job-promise-algorithm

    // 1. If job’s client is not null, queue a task, on job’s client's
    // responsible event loop using the DOM manipulation task source, to reject
    // job’s job promise with a new exception with errorData and a user
    // agent-defined message, in job’s client's Realm.
    auto context = job->client();

    if (context != nullptr) {
        context->webBase()->messageLoop()->addIdler(
            context->globalScope(),
            [](size_t handle, void* data, void* data1, void* data2) {
                ServiceWorkerJob* job = castTo<ServiceWorkerJob*>(data);
                ServiceWorkerContainer* container =
                    castTo<ServiceWorkerContainer*>(data1);
                ErrorData* errorData = castTo<ErrorData*>(data2);

                // TODO: consider generating an error message here.
                auto exception =
                    new DOMException(container->executionContext(),
                                     errorData->code, CSTR(errorData->message));

                job->promise()->reject(exception->scriptValue());
            },
            job, this, errorData);
    }

    // 2. For each equivalentJob in job’s list of equivalent jobs:
    // 2.1. If equivalentJob’s client is null, continue.
    // 2.2 Queue a task, on equivalentJob’s client's responsible event loop
    // using the DOM manipulation task source, to reject equivalentJob’s job
    // promise with a new exception with errorData and a user agent-defined
    // message, in equivalentJob’s client's Realm.
}

void ServiceWorkerContainer::finishJob(ServiceWorkerJob* job)
{
    TRACE_SCOPE(SVCWORKER);
    STARFISH_ASSERT(job != nullptr);
    m_jobMap.erase(job->data()->id);
}

Nullable<ServiceWorkerJob*> ServiceWorkerContainer::findJob(
    Id<ServiceWorkerJob> id)
{
    TRACE_SCOPE(SVCWORKER);
    auto it = m_jobMap.find(id);
    if (it == m_jobMap.end()) {
        return nullptr;
    }
    return it->second;
}

NULLABLE ServiceWorkerRequest* ServiceWorkerContainer::findRequest(
    Id<ServiceWorkerRequest> id)
{
    TRACE_SCOPE(SVCWORKER);
    auto it = m_requestMap.find(id);
    if (it == m_requestMap.end()) {
        return nullptr;
    }
    return it->second;
}

void ServiceWorkerContainer::finishRequest(ServiceWorkerRequest* request)
{
    TRACE_SCOPE(SVCWORKER);
    STARFISH_ASSERT(request != nullptr);
    m_requestMap.erase(request->id);
}

} // namespace Starfish

#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
