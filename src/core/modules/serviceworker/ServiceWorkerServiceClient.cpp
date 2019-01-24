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
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/serviceworker/ServiceWorkerJob.h"
#include "core/modules/serviceworker/ServiceWorkerServiceHost.h"
#include "core/modules/serviceworker/ServiceWorkerServiceClient.h"
#include "core/modules/serviceworker/ServiceWorkerServiceJobClient.h"

#include <EscargotPublic.h>

namespace Starfish {

ServiceWorkerServiceClient* ServiceWorkerServiceClient::m_instance = nullptr;

ServiceWorkerServiceClient* ServiceWorkerServiceClient::getInstance()
{
    if (!m_instance) {
        m_instance = new ServiceWorkerServiceClient();
    }
    return m_instance;
}

void ServiceWorkerServiceClient::destroy()
{
    delete m_instance;
    m_instance = nullptr;
    // NOTE: temporary destroy the host instance here
    ServiceWorkerServiceHost::getInstance()->destroy();
}

ServiceWorkerServiceClient::ServiceWorkerServiceClient()
    : m_jobClient(nullptr)
{
}

ServiceWorkerServiceClient::~ServiceWorkerServiceClient()
{
}

void ServiceWorkerServiceClient::init(ServiceWorkerServiceJobClient* jobClient)
{
    m_jobClient = jobClient;
}

ServiceWorkerServiceHostInterface* ServiceWorkerServiceClient::host()
{
    return ServiceWorkerServiceHost::getInstance();
}

void ServiceWorkerServiceClient::resolveJobPromise(ServiceWorkerJob* job)
{
    m_jobClient->resolveJobPromise(job);
}

} // namespace Starfish

#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
