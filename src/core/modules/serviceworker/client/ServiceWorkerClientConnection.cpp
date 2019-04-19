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

#include "core/modules/serviceworker/ServiceWorkerProcessInterface.h"
#include "core/modules/threading/IRunnable.h"
#include "core/modules/serviceworker/IORunnable.h"
#include "core/modules/serviceworker/Connection.h"
#include "core/modules/serviceworker/client/ServiceWorkerClientConnection.h"

#include "core/modules/networking/Socket.h"
#include "core/util/Id.h"
#include "core/modules/serviceworker/ServiceWorkerTypes.h"
#include "core/modules/serviceworker/ServiceWorkerProcessInterface.h"
#include "core/modules/serviceworker/host/ServiceWorkerHostProcess.h"

#include "core/modules/serviceworker/ServiceWorkerTypes.h"
#include "core/util/Archivable.h"
#include "core/modules/serviceworker/ServiceWorkerJobData.h"
#include "core/modules/serviceworker/ServiceWorkerJob.h"

#include "platform/process/base/ProcessType.h"
#include "core/modules/serviceworker/client/ServiceWorkerProcessManager.h"

#include "core/page/Navigator.h"
#include "core/page/Window.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/Document.h"
#include "core/modules/serviceworker/client/ServiceWorkerContainer.h"
#include "core/modules/serviceworker/host/ServiceWorkerHostConnection.h"

#include "core/util/Archiver.h"
#include "core/modules/serviceworker/Message.h"
#include "core/modules/serviceworker/ServiceWorkerRequest.h"
#include "core/modules/serviceworker/ServiceWorkerRegistrationData.h"

#ifdef STARFISH_ENABLE_SERVICE_WORKER

namespace Starfish {

ServiceWorkerClientConnection::ServiceWorkerClientConnection()
{
}

void ServiceWorkerClientConnection::scheduleJob(ServiceWorkerJob* job)
{
    STARFISH_ASSERT(job != nullptr);

    job->setClientConnection(this);

    // marshalling
    JsonWriter writer;
    Message msg("scheduleJob");
    msg.addParam(job->data());
    msg.archive(writer);

    m_socket->send(writer.GetString(), writer.GetSize(), SCK_DONTWAIT);
}

void ServiceWorkerClientConnection::matchRegistration(
    ServiceWorkerRequest* request, String* clientURL)
{
    STARFISH_ASSERT(request != nullptr);
    STARFISH_ASSERT(clientURL != nullptr);

    // marshalling
    JsonWriter writer;
    Message msg("matchRegistration");
    msg.addParam(request);
    msg.addParam(new GenericArchivable<String*>(TypeName::String, clientURL));
    msg.archive(writer);

    m_socket->send(writer.GetString(), writer.GetSize(), SCK_DONTWAIT);
}

void ServiceWorkerClientConnection::onReceived(Socket* socket, const char* data)
{
    STARFISH_ASSERT(data != nullptr);

    JsonReader reader(data);
    Message msg;
    msg.archive(reader);

    auto msgName = msg.name();

    if (msgName == "resolveJobPromise") {
        auto jobData = downcast<ServiceWorkerJobData>(msg.param(0));
        auto job = new ServiceWorkerJob(jobData);

        STARFISH_ASSERT(job != nullptr);

        auto registration =
            downcast<ServiceWorkerRegistrationData>(msg.param(1));
        resolveJobPromise(job, registration);
    }
}

void ServiceWorkerClientConnection::resolveJobPromise(
    ServiceWorkerJob* job, ServiceWorkerRegistrationData* registration)
{
    STARFISH_ASSERT(job != nullptr);
    STARFISH_ASSERT(registration != nullptr);

    // find if this job owner context is still active.
    auto swpm = ServiceWorkerProcessManager::getInstance();
    auto globalScope = swpm->find(job->data()->contextId);

    if (globalScope) {
        auto executionContext = globalScope->executionContext();
        if (executionContext->hasDocument()) {
            auto window = executionContext->document()->window();
            // TODO: use serviceworker bindings on window
            auto serviceWorkerContainer = window->navigator()->serviceWorker();
            // TODO: consider passing job data and move findjob into container
            auto jobMatched = serviceWorkerContainer->findJob(job->data()->id);
            if (jobMatched) {
                serviceWorkerContainer->resolveJobPromise(jobMatched,
                                                          registration);
            }
        }
    }
}

} // namespace Starfish
#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
