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
#include "core/modules/serviceworker/host/ServiceWorkerHostConnection.h"

#include "core/modules/networking/Socket.h"
#include "core/util/Id.h"
#include "core/modules/serviceworker/ServiceWorkerTypes.h"
#include "core/modules/serviceworker/ServiceWorkerProcessInterface.h"

#include "platform/process/base/ProcessType.h"
#include "core/modules/threading/IRunnable.h"
#include "core/modules/serviceworker/client/ServiceWorkerProcessManager.h"

#include "core/modules/serviceworker/ServiceWorkerJob.h"
#include "core/modules/serviceworker/client/ServiceWorkerClientConnection.h"

#ifdef STARFISH_ENABLE_SERVICE_WORKER

namespace Starfish {

ServiceWorkerHostConnection::ServiceWorkerHostConnection(
    ServiceWorkerHostProcessInterface* client)
    : m_client(client)
{
}

void ServiceWorkerHostConnection::resolveJobPromise(
    ServiceWorkerJob* job, ServiceWorkerRegistrationData* registration)
{
    // TODO: use socket to communicate with the client.
    auto swpm = ServiceWorkerProcessManager::getInstance();
    auto connection = swpm->getConnection(CSTR(job->data()->origin));
    connection->resolveJobPromise(job, registration);
}

void ServiceWorkerHostConnection::onReceived(Socket* socket, const char* data)
{
}

} // namespace Starfish
#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
