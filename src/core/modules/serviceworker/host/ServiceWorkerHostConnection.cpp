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

#include "core/util/Id.h"
#include "core/util/Archiver.h"
#include "core/util/Archivable.h"
#include "core/modules/networking/Socket.h"
#include "core/modules/threading/IRunnable.h"
#include "core/modules/serviceworker/Task.h"
#include "core/modules/serviceworker/Message.h"
#include "platform/process/base/ProcessType.h"
#include "core/modules/worker/util/network/IORunnable.h"
#include "core/modules/worker/util/network/Connection.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/DOMException.h"

#include "core/modules/worker/WorkerConfig.h"
#include "core/modules/serviceworker/host/ServiceWorkerAgent.h"
#include "core/modules/serviceworker/ServiceWorkerTypes.h"
#include "core/modules/serviceworker/ExceptionData.h"
#include "core/modules/serviceworker/MessageServiceWorker.h"
#include "core/modules/serviceworker/ConnectionInterface.h"
#include "core/modules/serviceworker/FetchEventData.h"
#include "core/modules/serviceworker/ServiceWorkerData.h"
#include "core/modules/serviceworker/ServiceWorkerJobData.h"
#include "core/modules/serviceworker/ServiceWorkerRegistrationData.h"
#include "core/modules/serviceworker/ServiceWorkerJob.h"
#include "core/modules/serviceworker/ServiceWorkerRequest.h"
#include "core/modules/serviceworker/host/ServiceWorkerHostJobHandler.h"
#include "core/modules/serviceworker/host/ServiceWorkerServerInterface.h"
#include "core/modules/serviceworker/host/ServiceWorkerHostConnection.h"

namespace Starfish {

ServiceWorkerHostConnection::ServiceWorkerHostConnection(
    ServiceWorkerServerInterface* server)
    : m_SWServer(server)
{
    STARFISH_ASSERT(server != nullptr);
}

void ServiceWorkerHostConnection::resolveJobPromise(
    ServiceWorkerJob* job, NULLABLE ServiceWorkerRegistrationData* registration)
{
    TRACE_SCOPE(HOST);
    STARFISH_ASSERT(job != nullptr);

    JsonWriter writer;
    Message msg("resolveJobPromise");
    msg.addParam(job->data());
    msg.addParam(registration);
    msg.archive(writer);

    send(writer.GetString(), writer.GetSize() + 1);
}

void ServiceWorkerHostConnection::rejectJobPromise(ServiceWorkerJob* job,
                                                   ExceptionData* errorData)
{
    TRACE_SCOPE(HOST);
    STARFISH_ASSERT(job != nullptr);
    STARFISH_ASSERT(errorData != nullptr);

    JsonWriter writer;

    Message msg("rejectJobPromise");
    msg.addParam(job->data());
    msg.addParam(errorData);
    msg.archive(writer);

    send(writer.GetString(), writer.GetSize() + 1);
}

void ServiceWorkerHostConnection::resolveRequest(
    ServiceWorkerRequest* request, NULLABLE Archivable* archivable)
{
    TRACE_SCOPE(HOST);
    STARFISH_ASSERT(request != nullptr);

    JsonWriter writer;
    Message msg("resolveRequest");
    msg.addParam(request);
    msg.addParam(archivable);
    msg.archive(writer);

    send(writer.GetString(), writer.GetSize() + 1);
}

void ServiceWorkerHostConnection::onUpdateRegistrationState(
    ServiceWorkerRegistrationData* registration,
    ServiceWorkerRegistrationState target, ServiceWorkerData* source)
{
    TRACE_SCOPE(HOST);
    JsonWriter writer;
    Message msg("updateRegistrationState");

    auto param = new UpdateRegistrationState(registration, target, source);
    msg.addParam(param);
    msg.archive(writer);

    send(writer.GetString(), writer.GetSize() + 1);
}

void ServiceWorkerHostConnection::respondFetchEvent(
    FetchEventResponseData* data)
{
    TRACE_SCOPE(HOST);
    STARFISH_ASSERT(data);

    JsonWriter writer;
    Message msg("respondFetchEvent");
    msg.addParam(data);
    msg.archive(writer);

    send(writer.GetString(), writer.GetSize() + 1);
}

void ServiceWorkerHostConnection::onUpdateWorkerState(String* scriptURL,
                                                      ServiceWorkerState state)
{
    TRACE_SCOPE(HOST);
    JsonWriter writer;
    Message msg("updateWorkerState");

    msg.addParam(new UpdateWorkerStateData(scriptURL, state));
    msg.archive(writer);

    send(writer.GetString(), writer.GetSize() + 1);
}

void ServiceWorkerHostConnection::fireEventRequest(String* scriptURL,
                                                   String* eventName)
{
    TRACE_SCOPE(HOST);
    JsonWriter writer;
    Message msg("fireEventRequest");

    msg.addParam(new StringArchivable(TypeName::String, scriptURL));
    msg.addParam(new StringArchivable(TypeName::String, eventName));
    msg.archive(writer);

    send(writer.GetString(), writer.GetSize() + 1);
}

void ServiceWorkerHostConnection::onReceived(Socket* socket, const char* data,
                                             size_t len)
{
    TRACE_SCOPE(HOST);
    Connection::onReceived(socket, data, len);

    if (m_SWServer->isTerminating() == true) {
        // TODO: send request reject
        TRACE(HOST,
              "1. received data is ignored due to swserver is "
              "being terminated");
        return;
    }

    // unmarshalling
    JsonReader reader(data);
    Message msg;
    msg.archive(reader);

    auto msgName = msg.name();
    auto handler = m_SWServer->jobHandler();

    STARFISH_ASSERT(handler != nullptr);
    TRACE(HOST, "onReceived:", msgName.data());

    // NOTE: consider using a message map to invoke member functions registered.
    if (msgName == "scheduleJob") {
        auto job =
            new ServiceWorkerJob(downcast<ServiceWorkerJobData*>(msg.param(0)));
        job->setHostConnection(this);
        handler->scheduleJob(job);

    } else if (msgName == "matchRegistration") {
        auto request = downcast<ServiceWorkerRequest*>(msg.param(0));
        auto clientURL = downcast<StringArchivable*>(msg.param(1))->value();

        auto registration = handler->matchRegistration(request, clientURL);
        resolveRequest(request, registration);

    } else if (msgName == "updateServiceWorkerClient") {
        handler->updateServiceWorkerClient(
            downcast<ContextRequestData*>(msg.param(0)));
    } else if (msgName == "fetchEvent") {
        auto data = downcast<FetchEventRequestData*>(msg.param(0));
        handler->handleFetch(data, this);
    } else if (msgName == "startServiceWorkerContext") {
        auto data = downcast<ServiceWorkerData*>(msg.param(0));
        handler->startServiceWorkerContext(data);
    } else {
        STARFISH_LOG_ERROR("Unknown message is received: %s", msgName.c_str());
        STARFISH_ASSERT_NOT_REACHED();
    }
}

} // namespace Starfish
#endif // #ifdef STARFISH_WEBWORKER_HOST
