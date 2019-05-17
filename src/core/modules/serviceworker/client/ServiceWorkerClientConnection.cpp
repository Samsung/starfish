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

#include "core/modules/serviceworker/ServiceWorkerTypes.h"
#include "core/modules/serviceworker/ErrorData.h"
#include "core/modules/serviceworker/MessageServiceWorker.h"
#include "core/modules/serviceworker/ServiceWorkerProcessInterface.h"
#include "core/modules/serviceworker/ServiceWorkerJobData.h"
#include "core/modules/serviceworker/ServiceWorkerRegistrationData.h"
#include "core/modules/serviceworker/ServiceWorkerJob.h"
#include "core/modules/serviceworker/ServiceWorkerRequest.h"
#include "core/modules/serviceworker/ServiceWorkerContainer.h"
#include "core/modules/serviceworker/client/ServiceWorkerProcessManager.h"
#include "core/modules/serviceworker/client/ServiceWorkerClientConnection.h"

#if !defined(STARFISH_WEBWORKER_HOST)
#include "core/page/Navigator.h"
#include "core/dom/Document.h"
#include "core/page/Window.h"
#endif

namespace Starfish {

ServiceWorkerClientConnection::ServiceWorkerClientConnection()
{
}

void ServiceWorkerClientConnection::scheduleJob(ServiceWorkerJob* job)
{
    STARFISH_ASSERT(job != nullptr);

    sendMessage("scheduleJob", job->data());
}

void ServiceWorkerClientConnection::matchRegistration(
    ServiceWorkerRequest* request, String* clientURL)
{
    STARFISH_ASSERT(request != nullptr);
    STARFISH_ASSERT(clientURL != nullptr);

    sendMessage("matchRegistration", request,
                new StringArchivable(TypeName::String, clientURL));
}

void ServiceWorkerClientConnection::updateServiceWorkerClient(
    ContextRequestData* request)
{
    STARFISH_ASSERT(request != nullptr);

    sendMessage("updateServiceWorkerClient", request);
}

void ServiceWorkerClientConnection::sendMessage(const char* msgName,
                                                NULLABLE Archivable* param1,
                                                NULLABLE Archivable* param2)
{
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
    Connection::onReceived(socket, data, len);

    JsonReader reader(data);
    Message msg;
    msg.archive(reader);

    auto msgName = msg.name();

    if (msgName == "resolveJobPromise") {
        auto jobData = downcast<ServiceWorkerJobData*>(msg.param(0));
        auto job = new ServiceWorkerJob(jobData);

        NULLABLE auto registration =
            static_cast<ServiceWorkerRegistrationData*>(msg.param(1));
        resolveJobPromise(job, registration);

    } else if (msgName == "rejectJobPromise") {
        auto jobData = downcast<ServiceWorkerJobData*>(msg.param(0));
        auto job = new ServiceWorkerJob(jobData);

        auto errorData = downcast<ErrorData*>(msg.param(1));
        rejectJobPromise(job, errorData);

    } else if (msgName == "resolveRequest") {
        auto request = downcast<ServiceWorkerRequest*>(msg.param(0));
        auto serviceWorkerContainer =
            findServiceWorkerContainer(request->contextId);

        if (serviceWorkerContainer != nullptr) {
            NULLABLE Archivable* archivable = msg.param(1);

            auto requestMatched =
                serviceWorkerContainer->findRequest(request->id);

            if ((requestMatched != nullptr) && requestMatched->postTask()) {
                requestMatched->postTask()->run(requestMatched, { archivable });
            }
        }
    } else if (msgName == "updateWorkerState") {
        auto data = downcast<UpdateWorkerStateData*>(msg.param(0));
        updateWorkerState(data->registrationId, data->state);

    } else {
        STARFISH_LOG_ERROR("Unknown message is received: %s", msgName.c_str());
        STARFISH_ASSERT_NOT_REACHED();
    }
}

void ServiceWorkerClientConnection::resolveJobPromise(
    ServiceWorkerJob* job, NULLABLE ServiceWorkerRegistrationData* registration)
{
    STARFISH_ASSERT(job != nullptr);

    // find if this job owner context is still active.
    NULLABLE auto serviceWorkerContainer =
        findServiceWorkerContainer(job->data()->contextId);

    if (serviceWorkerContainer != nullptr) {
        // TODO: consider passing job data and move findjob into container
        NULLABLE auto jobMatched =
            serviceWorkerContainer->findJob(job->data()->id);
        if (jobMatched != nullptr) {
            serviceWorkerContainer->resolveJobPromise(jobMatched, registration);
        }
    }
}

void ServiceWorkerClientConnection::rejectJobPromise(ServiceWorkerJob* job,
                                                     ErrorData* errorData)
{
    STARFISH_ASSERT(job != nullptr);
    STARFISH_ASSERT(errorData != nullptr);

    // find if this job owner context is still active.
    NULLABLE auto serviceWorkerContainer =
        findServiceWorkerContainer(job->data()->contextId);

    if (serviceWorkerContainer != nullptr) {
        NULLABLE auto jobMatched =
            serviceWorkerContainer->findJob(job->data()->id);
        if (jobMatched != nullptr) {
            serviceWorkerContainer->rejectJobPromise(jobMatched, errorData);
        }
    }
}

void ServiceWorkerClientConnection::updateWorkerState(
    ServiceWorkerRegistrationId id, ServiceWorkerState target)
{
    // https://w3c.github.io/ServiceWorker/#update-worker-state
    // TODO: 3. For each workerObject in workerObjects:
}

NULLABLE ServiceWorkerContainer*
ServiceWorkerClientConnection::findServiceWorkerContainer(
    ServiceWorkerContextId id)
{
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
