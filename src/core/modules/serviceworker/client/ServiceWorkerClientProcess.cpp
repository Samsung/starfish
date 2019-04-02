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

#include <EscargotPublic.h>

#include "core/dom/ExecutionContext.h"
#include "core/modules/serviceworker/ServiceWorkerTypes.h"
#include "core/modules/serviceworker/ServiceWorkerProcessInterface.h"
#include "core/modules/serviceworker/client/ServiceWorkerClientProcess.h"

#include "core/modules/serviceworker/ServiceWorkerJob.h"
#include "core/modules/serviceworker/client/ServiceWorkerJobClient.h"

#include "core/modules/serviceworker/host/ServiceWorkerHostProcess.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/dom/ExecutionContext.h"

namespace Starfish {

ServiceWorkerClientProcess* ServiceWorkerClientProcess::m_instance = nullptr;

ServiceWorkerClientProcess* ServiceWorkerClientProcess::getInstance()
{
    if (!m_instance) {
        m_instance = new ServiceWorkerClientProcess();
    }
    return m_instance;
}

void ServiceWorkerClientProcess::destroy()
{
    delete m_instance;
    m_instance = nullptr;
    // NOTE: temporary destroy the host instance here
    ServiceWorkerHostProcess::getInstance()->destroy();
}

ServiceWorkerClientProcess::ServiceWorkerClientProcess()
    : m_jobClient(nullptr)
{
}

ServiceWorkerClientProcess::~ServiceWorkerClientProcess()
{
}

void ServiceWorkerClientProcess::init(ServiceWorkerJobClient* jobClient)
{
    m_jobClient = jobClient;
}

ServiceWorkerHostProcessInterface* ServiceWorkerClientProcess::host()
{
    return ServiceWorkerHostProcess::getInstance();
}

void ServiceWorkerClientProcess::resolveJobPromise(
    ServiceWorkerJob* job, ServiceWorkerRegistrationData* registration)
{
    m_jobClient->resolveJobPromise(job, registration);
}

} // namespace Starfish

#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
