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
#include "core/util/Archiver.h"
#include "core/util/Archivable.h"
#include "core/modules/networking/Socket.h"
#include "core/modules/threading/IRunnable.h"
#include "core/modules/serviceworker/Task.h"
#include "core/modules/serviceworker/Message.h"
#include "platform/process/base/ProcessType.h"

#include "core/dom/ExecutionContext.h"
#include "core/dom/DOMException.h"
#include "core/page/GlobalScope.h"

#include "core/modules/serviceworker/IORunnable.h"
#include "core/modules/serviceworker/Connection.h"

#include "core/modules/serviceworker/ServiceWorker.h"
#include "core/modules/serviceworker/ServiceWorkerTypes.h"
#include "core/modules/serviceworker/ExceptionData.h"
#include "core/modules/serviceworker/MessageServiceWorker.h"
#include "core/modules/serviceworker/ConnectionInterface.h"
#include "core/modules/serviceworker/ServiceWorkerJobData.h"
#include "core/modules/serviceworker/ServiceWorkerRegistrationData.h"
#include "core/modules/serviceworker/ServiceWorkerRegistration.h"
#include "core/modules/serviceworker/ServiceWorkerJob.h"
#include "core/modules/serviceworker/ServiceWorkerRequest.h"
#include "core/modules/serviceworker/ServiceWorkerContainer.h"
#include "core/modules/serviceworker/client/ServiceWorkerProcessManager.h"
#include "core/modules/serviceworker/client/ServiceWorkerClientConnection.h"
#include "core/modules/serviceworker/client/FetchEventHandler.h"
#include "core/modules/serviceworker/FetchEventData.h"

#include "core/modules/worker/util/Trace.h"
#include "Starfish.h"
#include "core/dom/Event.h"
#include "core/dom/EventTarget.h"
#include "core/page/WebBase.h"
#include "core/modules/message_loop/MessageLoop.h"

#include "core/page/Navigator.h"
#include "core/dom/Document.h"
#include "core/page/Window.h"

namespace Starfish {

ServiceWorkerClientConnection::ServiceWorkerClientConnection()
{
}

void ServiceWorkerClientConnection::scheduleJob(ServiceWorkerJob* job)
{
    TRACE_SCOPE(CLIENT);
    STARFISH_ASSERT(job != nullptr);

    sendMessage("scheduleJob", job->data());
}

void ServiceWorkerClientConnection::matchRegistration(
    ServiceWorkerRequest* request, String* clientURL)
{
    TRACE_SCOPE(CLIENT);
    STARFISH_ASSERT(request != nullptr);
    STARFISH_ASSERT(clientURL != nullptr);

    sendMessage("matchRegistration", request,
                new StringArchivable(TypeName::String, clientURL));
}

void ServiceWorkerClientConnection::updateServiceWorkerClient(
    ContextRequestData* request)
{
    TRACE_SCOPE(CLIENT);
    STARFISH_ASSERT(request != nullptr);

    sendMessage("updateServiceWorkerClient", request);
}

void ServiceWorkerClientConnection::fetchEvent(
    FetchEventRequestData* FetchEventRequestData)
{
    TRACE_SCOPE(CLIENT);
    STARFISH_ASSERT(FetchEventRequestData != nullptr);

    sendMessage("fetchEvent", FetchEventRequestData);
}

void ServiceWorkerClientConnection::startServiceWorkerContext(
    ServiceWorkerData* serviceWorkerData)
{
    TRACE_SCOPE(CLIENT);
    STARFISH_ASSERT(serviceWorkerData != nullptr);

    sendMessage("startServiceWorkerContext", serviceWorkerData);
}

void ServiceWorkerClientConnection::sendMessage(const char* msgName,
                                                NULLABLE Archivable* param1,
                                                NULLABLE Archivable* param2)
{
    TRACE_SCOPE(CLIENT);
    STARFISH_ASSERT(msgName != nullptr);

    JsonWriter writer;
    Message msg(msgName);

    // marshalling
    // NOTE: consider using variable arguments if needed
    if (param1 != nullptr) {
        msg.addParam(param1);
    }
    if (param2 != nullptr) {
        msg.addParam(param2);
    }

    msg.archive(writer);

    send(writer.GetString(), writer.GetSize() + 1);
}

void ServiceWorkerClientConnection::onReceived(Socket* socket, const char* data,
                                               size_t len)
{
    TRACE_SCOPE(CLIENT);
    Connection::onReceived(socket, data, len);

    JsonReader reader(data);
    Message msg;
    msg.archive(reader);

    auto msgName = msg.name();

    if (msgName == "resolveJobPromise") {
        auto jobData = reinterpret_cast<ServiceWorkerJobData*>(msg.param(0));
        auto job = new ServiceWorkerJob(jobData);

        NULLABLE auto registration =
            static_cast<ServiceWorkerRegistrationData*>(msg.param(1));
        resolveJobPromise(job, registration);

    } else if (msgName == "rejectJobPromise") {
        auto jobData = reinterpret_cast<ServiceWorkerJobData*>(msg.param(0));
        auto job = new ServiceWorkerJob(jobData);

        auto errorData = reinterpret_cast<ExceptionData*>(msg.param(1));
        rejectJobPromise(job, errorData);

    } else if (msgName == "resolveRequest") {
        auto request = reinterpret_cast<ServiceWorkerRequest*>(msg.param(0));
        auto serviceWorkerContainer =
            findServiceWorkerContainer(request->contextId);

        // Find a request matched.
        if (serviceWorkerContainer != nullptr) {
            NULLABLE Archivable* archivable = msg.param(1);

            auto requestMatched =
                serviceWorkerContainer->findRequest(request->id);

            if ((requestMatched != nullptr) && requestMatched->postTask()) {
                requestMatched->postTask()->run(requestMatched, { archivable });
            }
        }
    } else if (msgName == "updateRegistrationState") {
        auto data = reinterpret_cast<UpdateRegistrationState*>(msg.param(0));
        updateRegistrationState(data->registration, data->target, data->source);
    } else if (msgName == "updateWorkerState") {
        // This is like a notification from the host.
        auto data = reinterpret_cast<UpdateWorkerStateData*>(msg.param(0));
        updateWorkerState(data->scriptURL, data->state);

    } else if (msgName == "fireEventRequest") {
        auto scriptURL = reinterpret_cast<StringArchivable*>(msg.param(0));
        auto eventName = reinterpret_cast<StringArchivable*>(msg.param(1));
        fireEventRequest(scriptURL->value(), eventName->value());

    } else if (msgName == "respondFetchEvent") {
        auto data = reinterpret_cast<FetchEventResponseData*>(msg.param(0));
        respondFetchEvent(data);
    } else {
        STARFISH_LOG_ERROR("Unknown message is received: %s", msgName.c_str());
        STARFISH_ASSERT_NOT_REACHED();
    }
}

void ServiceWorkerClientConnection::resolveJobPromise(
    ServiceWorkerJob* job, NULLABLE ServiceWorkerRegistrationData* registration)
{
    TRACE_SCOPE(CLIENT);
    STARFISH_ASSERT(job != nullptr);

    // find if this job owner context is still active.
    auto serviceWorkerContainer =
        findServiceWorkerContainer(job->data()->contextId);
    if (!serviceWorkerContainer.hasValue()) {
        return;
    }

    // TODO: consider passing job data and move findjob into container
    auto jobMatched = serviceWorkerContainer->findJob(job->data()->id);
    if (jobMatched.hasValue()) {
        serviceWorkerContainer->resolveJobPromise(jobMatched.value(),
                                                  registration);
    }
}

void ServiceWorkerClientConnection::rejectJobPromise(ServiceWorkerJob* job,
                                                     ExceptionData* errorData)
{
    TRACE_SCOPE(CLIENT);
    STARFISH_ASSERT(job != nullptr);
    STARFISH_ASSERT(errorData != nullptr);

    // find if this job owner context is still active.
    auto serviceWorkerContainer =
        findServiceWorkerContainer(job->data()->contextId);
    if (!serviceWorkerContainer.hasValue()) {
        return;
    }

    auto jobMatched = serviceWorkerContainer->findJob(job->data()->id);
    if (jobMatched.hasValue()) {
        serviceWorkerContainer->rejectJobPromise(jobMatched.value(), errorData);
    }
}

void ServiceWorkerClientConnection::updateRegistrationState(
    ServiceWorkerRegistrationData* registration,
    ServiceWorkerRegistrationState target, ServiceWorkerData* source)
{
    // NOTE: This is related to the step below in
    // https://w3c.github.io/ServiceWorker/#update-registration-state-algorithm.

    TRACE_SCOPE(CLIENT);
    // Let settingsObjects be all environment settings objects whose origin is
    // worker’s script url's origin.

    auto swpm = ServiceWorkerProcessManager::instance();
    const GCVector<ServiceWorkerEnvironment*>& settingsObjects =
        swpm->getSettingsObjects(registration->scope);

    for (auto it = settingsObjects.begin(); it != settingsObjects.end(); it++) {
        // NOTE: Starfish isn't used with multiple execution contexts. So there
        // is only one settingsObject.

        ExecutionContext* executionContext =
            static_cast<ServiceWorkerEnvironment*>(*it);

        // TODO: Use a UpdateRegistrationState instance as an input param.
        struct Param : public gc {
            Param(ExecutionContext* settingsObject_,
                  UpdateRegistrationState* updateRegistrationState_)
            {
                settingsObject = settingsObject_;
                updateRegistrationState = updateRegistrationState_;
            }
            ServiceWorkerEnvironment* settingsObject;
            UpdateRegistrationState* updateRegistrationState;
        };

        // NOTE: Searching globalScope() with contextId is required if multiple
        // executionContexts (multiple WebViews) are supported.
        executionContext->webBase()->messageLoop()->addIdler(
            executionContext->globalScope(),
            [](size_t handle, void* data0) {
                TRACE_SCOPE(CLIENT);
                Param* param = static_cast<Param*>(data0);
                ServiceWorkerEnvironment* settingsObject =
                    param->settingsObject;
                UpdateRegistrationState* newState =
                    param->updateRegistrationState;

                // NOTE: This is related to the step 9 below in
                // https://w3c.github.io/ServiceWorker/#install.

                auto window = settingsObject->document()->window();
                auto serviceWorkerContainer =
                    window->navigator()->serviceWorker();
                auto registrationObjects =
                    serviceWorkerContainer->serviceWorkerRegistrations();

                // 9.2. For each registrationObject of registrationObjects, fire
                // an event on registrationObject named `updatefound`. Find
                // registrations

                auto serviceWorker = settingsObject->activeServiceWorker();

                for (const auto registrationObject : registrationObjects) {
                    auto registration = registrationObject->data();
                    if (registration->id == newState->registration->id) {
                        ServiceWorker* serviceWorker =
                            newState->source == nullptr
                                ? nullptr
                                : serviceWorkerContainer->controller();

                        switch (newState->target) {
                        case ServiceWorkerRegistrationState::Installing:
                            // 4.2 For each registrationObject in
                            // registrationObjects:

                            // 4.2.1 Queue a task to set the active attribute of
                            // registrationObject to null if registration’s
                            // active worker is null,

                            // or the result of getting the service worker
                            // object that represents registration’s active
                            // worker in registrationObject’s relevant settings
                            // object.
                            registration->setInstallingWorker(newState->source);
                            registrationObject->updateRegistrationState(
                                newState->target, serviceWorker);
                            break;
                        case ServiceWorkerRegistrationState::Waiting:
                            // 3.1 Set registration’s waiting worker to source.
                            registration->setWaitingWorker(newState->source);
                            registrationObject->updateRegistrationState(
                                newState->target, serviceWorker);
                            break;
                        case ServiceWorkerRegistrationState::Active:
                            // 4.1 Set registration’s active worker to source.
                            registration->setActiveWorker(newState->source);
                            registrationObject->updateRegistrationState(
                                newState->target, serviceWorker);
                            break;
                        default:
                            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
                            break;
                        }
                        break; // stop for-loop
                    }
                }

                delete param;
            },
            new Param(executionContext, new UpdateRegistrationState(
                                            registration, target, source)));
    }
}

void ServiceWorkerClientConnection::updateWorkerState(String* scriptURL,
                                                      ServiceWorkerState state)
{
    TRACE_SCOPE(CLIENT);
    // https://w3c.github.io/ServiceWorker/#update-worker-state

    // 3. Let settingsObjects be all environment settings objects whose origin
    //    is worker’s script url's origin.

    auto swpm = ServiceWorkerProcessManager::instance();
    const GCVector<ServiceWorkerEnvironment*>& settingsObjects =
        swpm->getSettingsObjects(scriptURL);

    // 4. For each settingsObject of settingsObjects, queue a task on
    //    settingsObject’s responsible event loop in the DOM manipulation task
    //    source to run the following steps:

    for (auto it = settingsObjects.begin(); it != settingsObjects.end(); it++) {
        // NOTE: Starfish isn't used with multiple execution contexts. So there
        // is only one settingsObject.

        ExecutionContext* executionContext =
            static_cast<ServiceWorkerEnvironment*>(*it);

        struct Param {
            Param(ExecutionContext* settingsObject_, ServiceWorkerState state_)
            {
                settingsObject = settingsObject_;
                state = state_;
            }
            ServiceWorkerEnvironment* settingsObject;
            ServiceWorkerState state;
        };

        executionContext->webBase()->messageLoop()->addIdler(
            executionContext->globalScope(),
            [](size_t handle, void* data0) {
                TRACE_SCOPE(CLIENT);
                Param* param = static_cast<Param*>(data0);
                ServiceWorkerEnvironment* settingsObject =
                    param->settingsObject;

                // 4.1. Let objectMap be settingsObject’s service worker object
                // map.

                // 4.2. If objectMap[worker] does not exist, then abort these
                // steps.

                // NOTE: In ExecutionContext.h, use activeServiceWorker instead
                // of objectMap[worker].
                auto serviceWorker = settingsObject->activeServiceWorker();
                if (serviceWorker == nullptr) {
                    return;
                }

                // 4.3. Let workerObj be objectMap[worker].
                auto workerObj = serviceWorker;

                // 4.4. Set workerObj’s state to state.
                TRACEF(CLIENT, "state: %d -> %d",
                       toUnderlyingType(workerObj->data()->state),
                       toUnderlyingType(param->state));
                workerObj->data()->state = param->state;

                // 4.5. Fire an event named statechange at workerObj.
                // dispatchEvent
                TRACE(CLIENT, "Fire 'statechange' event");
                String* eventName = settingsObject->starfish()
                                        ->staticStrings()
                                        ->m_statechange.localName();
                Event* e = new Event(settingsObject, eventName);
                workerObj->dispatchEventByUA(workerObj, e);

                delete param;
            },
            new Param(executionContext, state));
    }
}

void ServiceWorkerClientConnection::fireEventRequest(String* scriptURL,
                                                     String* eventName)
{
    TRACE_SCOPE(CLIENT);
    // Let settingsObjects be all environment settings objects whose origin is
    // worker’s script url's origin.

    auto swpm = ServiceWorkerProcessManager::instance();
    const GCVector<ServiceWorkerEnvironment*>& settingsObjects =
        swpm->getSettingsObjects(scriptURL);

    // For each settingsObject of settingsObjects, queue a task on
    // settingsObject’s responsible event loop in the DOM manipulation task
    // source to run the following steps:

    for (auto it = settingsObjects.begin(); it != settingsObjects.end(); it++) {
        // NOTE: Starfish isn't used with multiple execution contexts. So there
        // is only one settingsObject.

        ExecutionContext* executionContext =
            static_cast<ServiceWorkerEnvironment*>(*it);

        struct Param : public gc {
            Param(ExecutionContext* settingsObject_, String* eventName_)
            {
                settingsObject = settingsObject_;
                eventName = eventName_;
            }
            ServiceWorkerEnvironment* settingsObject;
            String* eventName;
        };

        // NOTE: Searching globalScope() with contextId is required if multiple
        // executionContexts (multiple WebViews) are supported.
        executionContext->webBase()->messageLoop()->addIdler(
            executionContext->globalScope(),
            [](size_t handle, void* data0) {
                TRACE_SCOPE(CLIENT);
                Param* param = static_cast<Param*>(data0);
                ServiceWorkerEnvironment* settingsObject =
                    param->settingsObject;

                // NOTE: This is related to the step 9 below in
                // https://w3c.github.io/ServiceWorker/#install.

                // 9.1. Let registrationObjects be every
                // ServiceWorkerRegistration object in settingsObject’s realm,
                // whose service worker registration is registration.

                auto window = settingsObject->document()->window();
                auto serviceWorkerContainer =
                    window->navigator()->serviceWorker();
                auto registrationObjects =
                    serviceWorkerContainer->serviceWorkerRegistrations();

                // 9.2. For each registrationObject of registrationObjects, fire
                // an event on registrationObject named `updatefound`. Find
                // registrations
                for (const auto registrationObject : registrationObjects) {
                    TRACEF(CLIENT, "Fire '%s' event", CSTR(param->eventName));
                    Event* e = new Event(settingsObject, param->eventName);
                    registrationObject->dispatchEventByUA(registrationObject,
                                                          e);
                }

                delete param;
            },
            new Param(executionContext, eventName));
    }
}

void ServiceWorkerClientConnection::respondFetchEvent(
    FetchEventResponseData* data)
{
    auto handler =
        ServiceWorkerProcessManager::instance()->findFetchEventHandler(
            data->contextId);
    if (!handler.hasValue()) {
        TRACE(CLIENT);
        return;
    }
    handler->respondFetchEvent(data);
}

Nullable<ServiceWorkerContainer*>
ServiceWorkerClientConnection::findServiceWorkerContainer(
    ServiceWorkerContextId id)
{
    TRACE_SCOPE(CLIENT);
    auto swpm = ServiceWorkerProcessManager::instance();
    auto globalScope = swpm->findGlobalScope(id);
#if !defined(STARFISH_WEBWORKER_HOST)
    if (globalScope != nullptr) {
        auto executionContext = globalScope->executionContext();
        if ((executionContext != nullptr) && executionContext->hasDocument()) {
            // TODO: use serviceworker bindings on window
            auto window = executionContext->document()->window();
            return window->navigator()->serviceWorker();
        }
    }
#endif
    return nullptr;
}

} // namespace Starfish
#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
