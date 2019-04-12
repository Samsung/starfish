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
#include "core/modules/serviceworker/client/ServiceWorkerRegistration.h"
#include "core/modules/serviceworker/client/ServiceWorker.h"
#include "core/dom/Document.h"

namespace Starfish {

ServiceWorkerRegistration::ServiceWorkerRegistration(
    ExecutionContext* executionContext)
    : EventTarget()
    , m_executionContext(executionContext)
    , m_installingWorker(nullptr)
    , m_waitingWorker(nullptr)
    , m_activeWorker(nullptr)
    , m_data(new ServiceWorkerRegistrationData)
{
}

ExecutionContext* ServiceWorkerRegistration::executionContext()
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
}

#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
