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

#include "StarfishConfig.h"

#include "core/util/Id.h"
#include "core/util/Archiver.h"
#include "core/util/Archivable.h"
#include "core/modules/networking/Socket.h"
#include "core/modules/threading/IRunnable.h"
#include "core/modules/serviceworker/Task.h"
#include "core/modules/serviceworker/Message.h"
#include "platform/process/base/ProcessType.h"
#include "core/modules/serviceworker/IORunnable.h"
#include "core/modules/serviceworker/Connection.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/DOMException.h"

#include "core/modules/serviceworker/ServiceWorkerTypes.h"
#include "core/modules/serviceworker/ErrorData.h"
#include "core/modules/serviceworker/ServiceWorkerProcessInterface.h"
#include "core/modules/serviceworker/ServiceWorkerData.h"
#include "core/modules/serviceworker/ServiceWorkerJobData.h"
#include "core/modules/serviceworker/ServiceWorkerRegistrationData.h"
#include "core/modules/serviceworker/ServiceWorkerJob.h"
#include "core/modules/serviceworker/ServiceWorkerRequest.h"
#include "core/modules/serviceworker/host/ServiceWorkerHostConnection.h"

#ifdef STARFISH_ENABLE_SERVICE_WORKER

namespace Starfish {

ServiceWorkerHostConnection::ServiceWorkerHostConnection(
    ServiceWorkerHostProcessInterface* client)
    : m_client(client)
{
}

void ServiceWorkerHostConnection::resolveJobPromise(
    ServiceWorkerJob* job, NULLABLE ServiceWorkerRegistrationData* registration)
{
    STARFISH_ASSERT(job != nullptr);

    JsonWriter writer;
    Message msg("resolveJobPromise");
    msg.addParam(job->data());
    msg.addParam(registration);
    msg.archive(writer);

    send(writer.GetString(), writer.GetSize() + 1);
}

void ServiceWorkerHostConnection::rejectJobPromise(ServiceWorkerJob* job,
                                                   ErrorData* errorData)
{
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
    STARFISH_ASSERT(request != nullptr);

    JsonWriter writer;
    Message msg("resolveRequest");
    msg.addParam(request);
    msg.addParam(archivable);
    msg.archive(writer);

    send(writer.GetString(), writer.GetSize() + 1);
}

void ServiceWorkerHostConnection::onReceived(Socket* socket, const char* data,
                                             size_t len)
{
    Connection::onReceived(socket, data, len);

    STARFISH_ASSERT(m_client != nullptr);

    // unmarshalling
    JsonReader reader(data);
    Message msg;
    msg.archive(reader);

    auto msgName = msg.name();

    if (msgName == "scheduleJob") {
        auto job =
            new ServiceWorkerJob(downcast<ServiceWorkerJobData*>(msg.param(0)));
        job->setHostConnection(this);
        m_client->scheduleJob(job);

    } else if (msgName == "matchRegistration") {
        m_client->matchRegistration(
            downcast<ServiceWorkerRequest*>(msg.param(0)),
            downcast<GenericArchivable<String*>*>(msg.param(1))->value());
    }
}

} // namespace Starfish
#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
