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

#include "core/dom/ExecutionContext.h"
#include "core/util/Id.h"
#include "core/modules/serviceworker/ServiceWorkerTypes.h"
#include "core/modules/serviceworker/ServiceWorkerProcessInterface.h"
#include "core/modules/serviceworker/host/ServiceWorkerHostProcess.h"

#include "core/util/Id.h"
#include "core/modules/serviceworker/ServiceWorkerJob.h"
#include "core/modules/serviceworker/ServiceWorkerRegistration.h"
#include "core/modules/serviceworker/JobQueue.h"
#include "core/modules/serviceworker/ServiceWorker.h"
#include "core/modules/serviceworker/host/ServiceWorkerHostJobHandler.h"
#include "core/modules/serviceworker/client/ServiceWorkerClientProcess.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/threading/AdaptedThread.h"
#include "core/modules/threading/ThreadPool.h"
#include "core/modules/threading/IRunnable.h"
#include "core/modules/networking/Socket.h"
#include "core/modules/serviceworker/IORunnable.h"
#include "core/modules/serviceworker/Connection.h"
#include "core/modules/serviceworker/host/ServiceWorkerHostConnection.h"

namespace Starfish {

ServiceWorkerHostProcess* ServiceWorkerHostProcess::m_instance = nullptr;

ServiceWorkerHostProcess* ServiceWorkerHostProcess::getInstance()
{
    if (!m_instance) {
        m_instance = new ServiceWorkerHostProcess();
    }
    return m_instance;
}

void ServiceWorkerHostProcess::destroy()
{
    delete m_instance;
    m_instance = nullptr;
}

ServiceWorkerHostProcess::ServiceWorkerHostProcess()
    : m_messageLoop(nullptr)
    , m_jobHandler(nullptr)
{
}

ServiceWorkerHostProcess::~ServiceWorkerHostProcess()
{
}

void ServiceWorkerHostProcess::init(ThreadPool* threadPool)
{
    m_threadPool = threadPool;
    m_messageLoop = m_threadPool->messageLoop();

    m_jobHandler = new ServiceWorkerHostJobHandler(m_messageLoop);
    m_ioRunnable = new IORunnable(m_messageLoop);
    m_ioThread = new AdaptedThread(m_threadPool);

    // create a connection
    m_connection = new ServiceWorkerHostConnection(this);

    std::string address = IPC_PROTOCOL;
    address.append(IPC_ADDRESS_PREFIX);
#ifndef SERVICE_WORKER_USE_MULTI_PROCESS
// TODO: make the address with the given argument from the process host
#endif
    m_connection->socket()->bind(address.c_str());
    m_ioRunnable->addClient(m_connection);

    // start I/O runner
    m_ioThread->start(m_ioRunnable);
}

ServiceWorkerRegistrationData* ServiceWorkerHostProcess::getRegistration(
    String* scope)
{
    // https://w3c.github.io/ServiceWorker/#get-registration-algorithm

    // 4. 5.
    ServiceWorkerRegistrationData* registration = m_registrationMap[scope];

    return registration;
}

void ServiceWorkerHostProcess::setRegistration(
    String* scope, ServiceWorkerUpdateViaCache updateViaCacheMode)
{
    // https://w3c.github.io/ServiceWorker/#set-registration-algorithm

    auto registration = new ServiceWorkerRegistrationData();
    m_registrationMap[scope] = registration;
}

void ServiceWorkerHostProcess::scheduleJob(ServiceWorkerJob* job)
{
    // https://w3c.github.io/ServiceWorker/#schedule-job-algorithm
    STARFISH_ASSERT(m_jobHandler);

    // TODO: move this into connection.
    job->setHostConnection(m_connection);

    // 1. Let jobQueue be null.
    JobQueue* jobQueue = nullptr;

    // 2. Let jobScope be job’s scope url, serialized.
    auto jobScope = job->data()->scopeURL;

    // 3. If scope to job queue map[jobScope] does not exist, set scope to job
    // queue map[jobScope] to a new job queue.
    // 4. Set jobQueue to scope to job queue map[jobScope].
    auto scope = m_jobQueueMap.find(jobScope);
    if (scope == m_jobQueueMap.end()) {
        jobQueue = new JobQueue();
        m_jobQueueMap.insert(std::make_pair(jobScope, jobQueue));
    } else {
        jobQueue = scope->second;
    }

    // 5. If jobQueue is empty, then:
    if (jobQueue->size() == 0) {
        // 5.1. Set job’s containing job queue to jobQueue, and enqueue job to
        // jobQueue.
        job->setContainingJobQueue(jobQueue);
        jobQueue->enqueueJob(job);

        // 5.2. Invoke Run Job with jobQueue.
        m_jobHandler->runJob(jobQueue);
    } else {
        // 6. Else:
    }
}

} // namespace Starfish

#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
