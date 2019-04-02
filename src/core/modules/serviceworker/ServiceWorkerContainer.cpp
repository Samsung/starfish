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

#include "core/modules/serviceworker/ServiceWorkerContainer.h"
#include "core/dom/ExecutionContext.h"
#include "core/modules/serviceworker/ServiceWorkerTypes.h"
#include "core/modules/serviceworker/ServiceWorkerJob.h"

#include "platform/process/base/ProcessType.h"
#include "core/modules/threading/IRunnable.h"
#include "core/modules/serviceworker/IORunnable.h"
#include "core/modules/serviceworker/Connection.h"

#include "core/modules/serviceworker/ServiceWorkerProcessInterface.h"
#include "core/modules/serviceworker/client/ServiceWorkerClientProcess.h"
#include "core/modules/serviceworker/ServiceWorkerRegistration.h"
#include "core/modules/serviceworker/RegistrationOptions.h"
#include "core/modules/serviceworker/ServiceWorker.h"

#include "core/dom/WebOrigin.h"
#include "core/dom/Document.h"
#include "core/dom/Event.h"
#include "core/page/WebBase.h"
#include "core/modules/message_loop/MessageLoop.h"

#include "core/modules/serviceworker/client/ServiceWorkerClientConnection.h"
#include "core/modules/serviceworker/client/ServiceWorkerProcessManager.h"
#include "core/modules/serviceworker/host/ServiceWorkerHostProcess.h"

#include <EscargotPublic.h>

namespace Starfish {

ScriptValue createException(ScriptBindingInstance* scriptBindingInstance,
                            Escargot::ErrorObjectRef::Code errorCode,
                            const char* message)
{
    Escargot::ContextRef* context = scriptBindingInstance->scriptContext();
    Escargot::ExecutionStateRef* state =
        Escargot::ExecutionStateRef::create(context);

    auto exception =
        Escargot::ValueRef::create(Escargot::ErrorObjectRef::create(
            state, errorCode, Escargot::StringRef::fromASCII(message)));

    return exception;
}

ServiceWorkerContainer::ServiceWorkerContainer(Document* document)
    : EventTarget(document)
    , m_refValueToMakeServiceWorkerJobId(0)
{
    ServiceWorkerClientProcess::getInstance()->init(this);
}

ServiceWorkerContainer::~ServiceWorkerContainer()
{
    ServiceWorkerClientProcess::getInstance()->destroy();
}

Promise* ServiceWorkerContainer::registerServiceWorker(
    String* rawScriptURL, RegistrationOptions* options /*= nullptr*/)
{
    // https://w3c.github.io/ServiceWorker/#navigator-service-worker-register

    // 1. Let p be a promise.
    Promise* p = new Promise(scriptBindingInstance());

    // 2. Let client be the context object’s service worker client.
    ExecutionContext* client = executionContext();

    // 3. Let scriptURL be the result of parsing scriptURL with the context
    // object’s relevant settings object’s API base URL.
    auto scriptURL =
        new ResourceURL(rawScriptURL, client->baseURL()->baseURI());

    // 4. Let scopeURL be null
    ResourceURL* scopeURL = nullptr;

    if (options) {
        // 5. If options.scope is present, set scopeURL to the result of parsing
        // options.scope with the context object’s relevant settings object’s
        // API base URL.
    }

    startRegister(scopeURL, scriptURL, p, client);

    return p;
}

void ServiceWorkerContainer::startRegister(ResourceURL* scopeURL,
                                           ResourceURL* scriptURL,
                                           Promise* promise,
                                           ExecutionContext* client)
{
    // https://w3c.github.io/ServiceWorker/#start-register

    // 1. If scriptURL is failure, reject promise with a TypeError and abort
    // these steps.
    if (scriptURL->urlString()->isEmpty()) {
        auto exception = createException(scriptBindingInstance(),
                                         Escargot::ErrorObjectRef::TypeError,
                                         "serviceWorker.register() cannot be "
                                         "called with an empty script URL");
        promise->reject(exception);
        return;
    }

    // 2. Set scriptURL’s fragment to null.
    auto scriptURLWithNoFragment = scriptURL->urlStringWithoutSearchPart();

    // 3. If scriptURL’s scheme is not one of "http" and "https", reject promise
    // with a TypeError and abort these steps.
    if (scriptURL->isHTTPFamilyURL() == false) {
        auto exception = createException(
            scriptBindingInstance(), Escargot::ErrorObjectRef::TypeError,
            "serviceWorker.register() must be called with a script URL whose "
            "protocol is either HTTP or HTTPS");
        promise->reject(exception);
        return;
    }

    // 4. If any of the strings in scriptURL’s path contains either ASCII
    // case-insensitive
    // "%2f" or ASCII case-insensitive "%5c", reject promise with a TypeError
    // and abort these steps.
    if (scriptURLWithNoFragment->contains("%2f", false) ||
        scriptURLWithNoFragment->contains("%5c", false)) {
        auto exception = createException(
            scriptBindingInstance(), Escargot::ErrorObjectRef::TypeError,
            "Scope URL provided to serviceWorker.register() cannot have a path "
            "that contains '%2f' or '%5c");
        promise->reject(exception);
        return;
    }

    // 5. If scopeURL is null, set scopeURL to the result of parsing the string
    // "./" with scriptURL.
    if (!scopeURL) {
        scopeURL = new ResourceURL(String::createASCIIString("./"),
                                   scriptURL->urlStringWithoutSearchPart());
    }

    // 8. If scopeURL’s scheme is not one of "http" and "https", reject promise
    // with a TypeError and abort these steps.
    if (scopeURL->isHTTPFamilyURL() == false) {
        auto exception = createException(
            scriptBindingInstance(), Escargot::ErrorObjectRef::TypeError,
            "serviceWorker.register() must be called with a scope URL whose "
            "protocol is either HTTP or HTTPS");
        promise->reject(exception);
        return;
    }

    // 9. If any of the strings in scopeURL path contains either ASCII
    // case-insensitive
    // "%2f" or ASCII case-insensitive "%5c", reject promise with a TypeError
    // and abort these steps.
    auto scopeURLWithNoFragment = scopeURL->urlStringWithoutSearchPart();
    if (scopeURLWithNoFragment->contains("%2f", false) ||
        scopeURLWithNoFragment->contains("%5c", false)) {
        auto exception = createException(
            scriptBindingInstance(), Escargot::ErrorObjectRef::TypeError,
            "Scope URL provided to serviceWorker.register() cannot have a path "
            "that contains '%2f' or '%5c");
        promise->reject(exception);
        return;
    }

    // 10. Let job be the result of running Create Job with register, scopeURL,
    // scriptURL, promise, and client.
    auto job = createJob(ServiceWorkerJobType::Register, scopeURLWithNoFragment,
                         scriptURLWithNoFragment, promise, client);

    // 11. Set job’s worker type to workerType.
    // 12. Set job’s update via cache mode to updateViaCache.

    // 13. Set job’s referrer to referrer.
    job->data()->referrerURL =
        job->client() ? job->client()->referrer() : nullptr;

    // 14. Invoke Schedule Job with job.
    scheduleJob(job);
}

// TODO: consider movig this to job handler
ServiceWorkerJob* ServiceWorkerContainer::createJob(ServiceWorkerJobType type,
                                                    String* scopeURL,
                                                    String* scriptURL,
                                                    Promise* promise,
                                                    ExecutionContext* client)
{
    // https://w3c.github.io/ServiceWorker/#create-job

    auto job = new ServiceWorkerJob();
    auto data = new ServiceWorkerJobData();

    data->id = m_refValueToMakeServiceWorkerJobId;
    data->type = type;
    data->scopeURL = scopeURL;
    data->scriptURL = scriptURL;

    job->setData(data);
    job->setPromise(promise);
    job->setClient(client);

    m_refValueToMakeServiceWorkerJobId++;

    return job;
}

void ServiceWorkerContainer::scheduleJob(ServiceWorkerJob* job)
{
    executionContext()->webBase()->messageLoop()->addIdler(
        executionContext(),
        [](size_t handle, void* data1, void* data2) {
            ServiceWorkerJob* job = static_cast<ServiceWorkerJob*>(data1);
            WebOrigin* webOrigin = static_cast<WebOrigin*>(data2);

            auto swConnection =
                ServiceWorkerProcessManager::getInstance()->getConnection(
                    CSTR(webOrigin->serialize()));

            swConnection->scheduleJob(job);
        },
        job, document()->webOrigin());

    m_jobMap.insert(std::make_pair(job->data()->id, job));
}

Promise* ServiceWorkerContainer::getRegistration(
    String* scriptURL /*= nullptr*/)
{
    Promise* promise = new Promise(scriptBindingInstance());

    // TODO: return registraton
    promise->fulfill(Escargot::ValueRef::createUndefined());
    return promise;
}

Promise* ServiceWorkerContainer::registerServiceWorker(
    String* url, RegistrationOptions& options)
{
    return registerServiceWorker(url, &options);
}

ServiceWorker* ServiceWorkerContainer::controller()
{
    // 1. Let client be the context object’s service worker client.
    ExecutionContext* context = executionContext();

    // 2. Return the ServiceWorker object that represents client’s active
    // service worker.
    auto client = context->activeServiceWorker();
    return client;
}

void ServiceWorkerContainer::resolveJobPromise(
    ServiceWorkerJob* job, ServiceWorkerRegistrationData* registration)
{
    // TODO: meet
    // https://w3c.github.io/ServiceWorker/#resolve-job-promise-algorithm

    // TODO: get matched registration

    // 2. If job’s client is not null, queue a task, on job’s client's
    // responsible event loop using the DOM manipulation task source, to run the
    // following substeps:
    auto context = job->client();

    if (context) {
        struct Params : public gc {
            ServiceWorkerJob* job;
            ServiceWorkerContainer* container;
        };

        auto params = new Params();
        params->job = job;
        params->container = this;

        context->webBase()->messageLoop()->addIdler(
            context,
            [](size_t handle, void* data) {
                Params* params = static_cast<Params*>(data);
                ServiceWorkerJob* job = params->job;
                ServiceWorkerContainer* container = params->container;

                // 1. Let convertedValue be null.
                auto convertedValue = Escargot::ValueRef::createNull();

                if (job->data()->type == ServiceWorkerJobType::Register ||
                    job->data()->type == ServiceWorkerJobType::Update) {
                    // TODO: 2. If job’s job type is either register or update,
                    // set convertedValue to the ServiceWorkerRegistration
                    // object that represents value, in job’s client's Realm.
                    auto registeration =
                        new ServiceWorkerRegistration(container->document());
                    auto serviceWorker =
                        new ServiceWorker(container->document());
                    serviceWorker->data()->setScriptURL(job->data()->scriptURL);
                    registeration->updateRegistrationState(
                        ServiceWorkerRegistrationState::Installing,
                        serviceWorker);

                    convertedValue = registeration->scriptValue();
                    job->promise()->fulfill(convertedValue);
                }
            },
            params);
    }
}
} // namespace Starfish

#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
