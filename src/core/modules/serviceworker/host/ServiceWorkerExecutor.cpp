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

#ifdef STARFISH_WEBWORKER_HOST

#include "StarfishConfig.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/serviceworker/ServiceWorkerAgent.h"
#include "core/modules/serviceworker/host/ServiceWorkerExecutor.h"

namespace Starfish {

void ServiceWorkerExecutor::initialize(Starfish* starfish)
{
    STARFISH_ASSERT(starfish != nullptr);

#ifdef PORT_NEEDS_THREADED_PUBLIC_API
    MessageLoop::runOnMainThreadSync([&]() -> size_t {
        STARFISH_ASSERT(starfish != nullptr);
        if (ServiceWorkerAgent::isCreated() == false) {
            ServiceWorkerAgent::create(starfish);
        }
        return 0;
    });
#else
    if (ServiceWorkerAgent::isCreated() == false) {
        ServiceWorkerAgent::create(starfish);
    }
#endif
}

void ServiceWorkerExecutor::runServiceWorker(const std::string& scriptURL)
{
    STARFISH_ASSERT(ServiceWorkerAgent::isCreated() == true);

#ifdef PORT_NEEDS_THREADED_PUBLIC_API
    MessageLoop::runOnMainThreadSync([scriptURL]() -> size_t {
        ServiceWorkerAgent::instance()->runServiceWorker(
            String::fromUTF8(scriptURL.data(), scriptURL.size()));
        return 0;
    });
#else
    ServiceWorkerAgent::instance()->runServiceWorker(
        String::fromUTF8(scriptURL.data(), scriptURL.size()));
#endif
}

void ServiceWorkerExecutor::registerOnStatusChangedHandler(
    ServiceWorkerAgentStateHandler cb)
{
    STARFISH_ASSERT(ServiceWorkerAgent::isCreated() == true);
    STARFISH_ASSERT(cb != nullptr);

#ifdef PORT_NEEDS_THREADED_PUBLIC_API
    MessageLoop::runOnMainThreadSync([&]() -> size_t {
        STARFISH_ASSERT(cb != nullptr);
        ServiceWorkerAgent::instance()->registerOnStatusChangedHandler(cb);
        return 0;
    });
#else
    ServiceWorkerAgent::registerOnStatusChangedHandler(cb);
#endif
}

void ServiceWorkerExecutor::finalize()
{
#ifdef PORT_NEEDS_THREADED_PUBLIC_API
    MessageLoop::runOnMainThreadSync([]() -> size_t {
        if (ServiceWorkerAgent::isCreated() == true) {
            ServiceWorkerAgent::instance()->destroy();
        }
        return 0;
    });
#else
    if (ServiceWorkerAgent::isCreated() == true) {
        ServiceWorkerAgent::instance()->destroy();
    }
#endif
}
}
#endif /* STARFISH_WEBWORKER_HOST */
