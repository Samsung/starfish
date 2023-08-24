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

#ifdef STARFISH_ENABLE_SERVICE_WORKER

#include "StarfishConfig.h"

#include "core/modules/resource_request/ResourceRequest.h"
#include "core/modules/worker/util/Trace.h"
#include "core/modules/serviceworker/client/ServiceWorkerClientConnection.h"
#include "core/modules/serviceworker/client/ServiceWorkerFetchTask.h"
#include "core/modules/serviceworker/client/FetchEventHandler.h"
#include "core/modules/serviceworker/FetchEventData.h"

namespace Starfish {

void FetchEventHandler::addFetch(ServiceWorkerFetchTask* task)
{
    m_fetchTaskMap.insert(std::make_pair(task->id(), task));

    if (!m_isStarted) {
        TRACE_SCOPE(CLIENT);
        m_pendingTasks.push_back(task);
        return;
    }

    TRACE_SCOPE(CLIENT, CSTR(task->resourceRequest()->url()->urlString()));
    STARFISH_ASSERT(m_connection);
    STARFISH_ASSERT(m_scopeURL);
    sendEvent(task);
}

void FetchEventHandler::start(ServiceWorkerClientConnection* connection,
                              String* scopeURL)
{
    if (m_isStarted) {
        return;
    }

    TRACE_SCOPE(CLIENT);
    STARFISH_ASSERT(connection);
    STARFISH_ASSERT(scopeURL);
    m_connection = connection;
    m_scopeURL = scopeURL;

    for (auto task : m_pendingTasks) {
        sendEvent(task);
    }

    m_pendingTasks.clear();

    m_isStarted = true;
}

void FetchEventHandler::respondFetchEvent(FetchEventResponseData* data)
{
    TRACE(CLIENT);

    auto it = m_fetchTaskMap.find(data->fetchTaskId);
    if (it == m_fetchTaskMap.end()) {
        STARFISH_LOG_WARN("Cannot find fetch task!");
        return;
    }

    auto task = it->second;
    STARFISH_ASSERT(
        task->resourceRequest()->url()->urlString()->equals(data->url));

    task->onResponse(data);

    m_fetchTaskMap.erase(it);
}

void FetchEventHandler::sendEvent(ServiceWorkerFetchTask* task)
{
    auto data = FetchEventRequestData::createFetchEventRequestData(
        task->resourceRequest());
    data->scopeURL = m_scopeURL;
    data->fetchTaskId = task->id();

    TRACE(CLIENT, CSTR(data->url));
    m_connection->fetchEvent(data);
}

ServiceWorkerFetchKey FetchEventHandler::fetchTaskId()
{
    return m_fetchTaskId++;
}

} // namespace Starfish

#endif
