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

#include "core/modules/serviceworker/WorkerConfig.h"
#include "core/modules/serviceworker/notification/NotificationJob.h"

#include "core/modules/serviceworker/ServiceWorkerRegistration.h"

namespace Starfish {

ServiceWorkerRegistration::ServiceWorkerRegistration(
    ExecutionContext* executionContext, ServiceWorkerJobClientInterface* client)
    : EventTarget()
    , m_executionContext(executionContext)
    , m_installingWorker(nullptr)
    , m_waitingWorker(nullptr)
    , m_activeWorker(nullptr)
    , m_data(new ServiceWorkerRegistrationData)
    , m_jobClient(client)
    , m_pushManager(new PushManager(executionContext, this))
{
    STARFISH_ASSERT(m_data != nullptr);
}

ExecutionContext* ServiceWorkerRegistration::executionContext() const
{
    return m_executionContext;
}

void ServiceWorkerRegistration::updateRegistrationState(
    ServiceWorkerRegistrationState state, ServiceWorker* serviceWorker)
{
    switch (state) {
    case ServiceWorkerRegistrationState::Installing:
        m_installingWorker = serviceWorker;
        break;
    case ServiceWorkerRegistrationState::Waiting:
        m_waitingWorker = serviceWorker;
        break;
    case ServiceWorkerRegistrationState::Active:
        m_activeWorker = serviceWorker;
        break;
    default:
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        break;
    }
}

String* ServiceWorkerRegistration::scope() const
{
    return m_data->scope;
}

String* ServiceWorkerRegistration::updateViaCache() const
{
    STARFISH_ASSERT(m_data != nullptr);

    switch (m_data->updateViaCache) {
    case ServiceWorkerUpdateViaCache::Imports:
        return String::createASCIIString("imports");
    case ServiceWorkerUpdateViaCache::All:
        return String::createASCIIString("all");
    case ServiceWorkerUpdateViaCache::None:
        return String::createASCIIString("none");
    }
    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
}

ServiceWorker* ServiceWorkerRegistration::installing() const
{
    return m_installingWorker;
}

ServiceWorker* ServiceWorkerRegistration::waiting() const
{
    return m_waitingWorker;
}

ServiceWorker* ServiceWorkerRegistration::active() const
{
    return m_activeWorker;
}

Promise* ServiceWorkerRegistration::unregister()
{
    STARFISH_ASSERT(m_jobClient != nullptr);
    WORKER_LOG_IF_ALLOWED(1, "0: called\n");

    // https://w3c.github.io/ServiceWorker/#navigator-service-worker-unregister

    // 1. Let promise be a promise.
    Promise* promise = new Promise(scriptBindingInstance());

    // 2. Let job be the result of running Create Job with unregister, the scope
    // url of the service worker registration, null, promise, and the context
    // object’s relevant settings object.
    auto job = m_jobClient->createJob(ServiceWorkerJobType::Unregister,
                                      data()->scope, nullptr, promise,
                                      m_jobClient->serviceWorkerEnvironment());

    // 3. Invoke Schedule Job with job.
    m_jobClient->scheduleJob(job);

    // 4. Return promise.
    return promise;
}

Promise* ServiceWorkerRegistration::showNotification(String* title)
{
    STARFISH_ASSERT(title != nullptr);
    NotificationOptions options;
    return showNotification(title, options);
}

Promise* ServiceWorkerRegistration::showNotification(
    String* title, NotificationOptions& options)
{
    STARFISH_ASSERT(title != nullptr);
    Promise* promise = new Promise(scriptBindingInstance());

    // TODO: If active worker is null,
    // then reject promise with TypeError and return promise

    NotificationJob* notification =
        new NotificationJob(executionContext(), title, options);

    notification->runNotification(promise);

    return promise;
}

} // namespace Starfish

#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
