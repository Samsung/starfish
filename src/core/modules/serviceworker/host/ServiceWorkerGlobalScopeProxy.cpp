/*
 * Copyright (c) 2022-present Samsung Electronics Co., Ltd
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

#ifdef STARFISH_WEBWORKER_HOST

#include "StarfishConfig.h"
#include "core/modules/serviceworker/util/ParallelTask.h"
#include "core/modules/worker/host/WorkerGlobalScope.h"
#include "core/modules/serviceworker/ExtendableEvent.h"
#include "core/modules/serviceworker/host/ServiceWorkerGlobalScope.h"
#include "core/modules/serviceworker/host/ServiceWorkerGlobalScopeProxy.h"

namespace Starfish {

void ServiceWorkerGlobalScopeProxy::setGlobalScope(
    ServiceWorkerGlobalScope* scope)
{
    STARFISH_ASSERT(!m_globalScope);
    m_globalScope = scope;
}

void ServiceWorkerGlobalScopeProxy::runPendingEvent()
{
    STARFISH_ASSERT(m_globalScope);
    for (auto data : m_pendingFetchEventData) {
        m_globalScope->handleFetch(data);
    }

    m_pendingFetchEventData.clear();
}

void ServiceWorkerGlobalScopeProxy::handleFetch(RequestData* data)
{
    if (m_globalScope) {
        m_globalScope->handleFetch(data);
    } else {
        m_pendingFetchEventData.push_back(data);
    }
}

} // namespace Starfish

#endif
