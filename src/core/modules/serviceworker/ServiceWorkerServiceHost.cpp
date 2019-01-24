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
#include "core/modules/serviceworker/ServiceWorkerServiceHost.h"
#include "core/modules/serviceworker/ServiceWorkerJob.h"
#include "core/modules/serviceworker/ServiceWorkerServiceHostJobQueue.h"
#include "core/modules/serviceworker/ServiceWorkerRegistration.h"
#include "core/modules/serviceworker/ServiceWorkerServiceClient.h"

namespace Starfish {

ServiceWorkerServiceHost* ServiceWorkerServiceHost::m_instance = nullptr;

ServiceWorkerServiceHost* ServiceWorkerServiceHost::getInstance()
{
    if (!m_instance) {
        m_instance = new ServiceWorkerServiceHost();
    }
    return m_instance;
}

void ServiceWorkerServiceHost::destroy()
{
    delete m_instance;
    m_instance = nullptr;
}

ServiceWorkerServiceHost::ServiceWorkerServiceHost()
    : m_messageLoop(nullptr)
    , m_jobQueueMap()
{
}

ServiceWorkerServiceHost::~ServiceWorkerServiceHost()
{
}

ServiceWorkerServiceClientInterface* ServiceWorkerServiceHost::client()
{
    // TODO: return a communication-interface
    return ServiceWorkerServiceClient::getInstance();
}

void ServiceWorkerServiceHost::init(MessageLoop* messageLoop)
{
    m_messageLoop = messageLoop;
}

ServiceWorkerRegistrationData* ServiceWorkerServiceHost::getRegistration(
    String* scope)
{
    // https://w3c.github.io/ServiceWorker/#get-registration-algorithm

    // 4. 5.
    ServiceWorkerRegistrationData* registration = m_registrationMap[scope];

    return registration;
}

void ServiceWorkerServiceHost::setRegistration(
    String* scope, ServiceWorkerUpdateViaCache updateViaCacheMode)
{
    // https://w3c.github.io/ServiceWorker/#set-registration-algorithm

    auto registration = new ServiceWorkerRegistrationData();
    m_registrationMap[scope] = registration;
}

void ServiceWorkerServiceHost::scheduleJob(ServiceWorkerJob* job)
{
    // https://w3c.github.io/ServiceWorker/#schedule-job-algorithm
    STARFISH_ASSERT(m_messageLoop);

    // 1. Let jobQueue be null.
    ServiceWorkerServiceHostJobQueue* jobQueue = nullptr;

    // 2. Let jobScope be job’s scope url, serialized.
    auto jobScope = job->scopeURL;

    // 3. If scope to job queue map[jobScope] does not exist, set scope to job
    // queue map[jobScope] to a new job queue.
    // 4. Set jobQueue to scope to job queue map[jobScope].
    auto scope = m_jobQueueMap.find(jobScope);
    if (scope == m_jobQueueMap.end()) {
        jobQueue = new ServiceWorkerServiceHostJobQueue(m_messageLoop);
        m_jobQueueMap.insert(std::make_pair(jobScope, jobQueue));
    } else {
        jobQueue = scope->second;
    }

    // 5. If jobQueue is empty, then:
    if (jobQueue->size() == 0) {
        // 5.1. Set job’s containing job queue to jobQueue, and enqueue job to
        // jobQueue.
        // TODO: containingJobQueue should be a certain identifier which can be
        // shared and unique b/t host and client.
        job->containingJobQueue = jobQueue;
        jobQueue->enqueueJob(job);

        // 5.2. Invoke Run Job with jobQueue.
        jobQueue->runJob();
    } else {
        // 6. Else:
    }
}

} // namespace Starfish

#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
