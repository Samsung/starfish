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

#include "core/modules/serviceworker/client/ServiceWorkerClientConnection.h"
#include "core/modules/serviceworker/util/Trace.h"
#include "core/modules/serviceworker/ServiceWorkerFetchTask.h"
#include "core/modules/serviceworker/FetchEventHandler.h"

namespace Starfish {

void FetchEventHandler::addFetch(FetchEventData* data)
{
    if (!m_isStarted) {
        TRACE_SCOPE(CLIENT);
        m_eventDatas.push_back(data);
        return;
    }

    TRACE_SCOPE(CLIENT);
    STARFISH_ASSERT(m_connection);
    STARFISH_ASSERT(m_scopeURL);
    sendEvent(data);
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

    for (auto data : m_eventDatas) {
        sendEvent(data);
    }

    m_eventDatas.clear();

    m_isStarted = true;
}

void FetchEventHandler::sendEvent(FetchEventData* data)
{
    data->scopeURL = m_scopeURL;

    m_connection->fetchEvent(data);
}

} // namespace Starfish

#endif
