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
#include "Starfish.h"
#include "core/modules/serviceworker/client/ServiceWorker.h"
#include "core/page/Window.h"
#include "core/dom/Document.h"

namespace Starfish {

DEFINE_EVENT_LISTENER(ServiceWorker, statechange);

String* ServiceWorker::scriptURL() const
{
    return m_data->scriptURL;
}

String* ServiceWorker::state() const
{
    switch (m_data->state) {
    case ServiceWorkerState::Installing:
        return String::createASCIIString("installing");
    case ServiceWorkerState::Installed:
        return String::createASCIIString("installed");
    case ServiceWorkerState::Activating:
        return String::createASCIIString("activating");
    case ServiceWorkerState::Activated:
        return String::createASCIIString("activated");
    case ServiceWorkerState::Redundant:
        return String::createASCIIString("redundant");
    }
    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
}

ExecutionContext* ServiceWorker::executionContext()
{
    return m_executionContext;
}
} // namespace Starfish

#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
